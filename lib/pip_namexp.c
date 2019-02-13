/*
  * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
  * 	  System Software Devlopment Team. All rights researved$
  * $PIP_VERSION: Version 1.0$
  * $PIP_license: <Simplified BSD License>
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  * The views and conclusions contained in the software and documentation
  * are those of the authors and should not be interpreted as representing
  * official policies, either expressed or implied, of the PiP project.$
*/
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2018
 */

#define _GNU_SOURCE
#include <stdarg.h>
#include <ctype.h>
#include <sched.h>
#include <stdio.h>

//#define DEBUG

#include <pip.h>
#include <pip_blt.h>
#include <pip_internal.h>
#include <pip_util.h>

#define PIP_HASHTAB_SZ	(1024)	/* must be power of 2 */

typedef uint64_t 	pip_hash_t;

typedef struct {
  pip_list_t			list; /* hash collision list */
  pip_hash_t			hashval;
  char				*name;
  void				*address;
  int				flag_exported;
  volatile  int			flag_canceled;
  pip_task_queue_t		queue;
} pip_namexp_entry_t;

typedef struct {
  pip_spinlock_t		lock;
  pip_list_t			list;
} pip_namexp_list_t;

typedef struct {
  int				pipid;
  pip_namexp_list_t		hash_table[PIP_HASHTAB_SZ];
} pip_named_exptab_t;


static void pip_namexp_lock( pip_spinlock_t *lock ) {
  DBGF( "LOCK %p", lock );
  pip_spin_lock( lock );
}

static void pip_namexp_unlock( pip_spinlock_t *lock ) {
  DBGF( "UNLOCK %p", lock );
  pip_spin_unlock( lock );
}

static void
pip_add_namexp( pip_namexp_list_t *head, pip_namexp_entry_t *entry ) {
  PIP_LIST_ADD( (pip_list_t*) &head->list, (pip_list_t*) entry );
}

static void pip_del_namexp( pip_namexp_entry_t *entry ) {
  PIP_LIST_DEL( (pip_list_t*) entry );
}

int pip_named_export_init_( pip_task_internal_t *taski ) {
  pip_named_exptab_t *namexp;
  int	i;

  namexp = (pip_named_exptab_t*) malloc( sizeof( pip_named_exptab_t ) );
  if( namexp == NULL ) RETURN( ENOMEM );
  memset( namexp, 0, sizeof( pip_named_exptab_t ) );

  namexp->pipid = taski->pipid;
  for( i=0; i<PIP_HASHTAB_SZ; i++ ) {
    pip_spin_init( &namexp->hash_table[i].lock );
    PIP_LIST_INIT( &namexp->hash_table[i].list );
  }
  taski->annex->named_exptab = (void*) namexp;
  return 0;
}

static pip_namexp_list_t*
pip_lock_hashtab_head( pip_named_exptab_t *hashtab, pip_hash_t hash ) {
  int			idx = hash & ( PIP_HASHTAB_SZ - 1 );
  pip_namexp_list_t	*head = &hashtab->hash_table[idx];
  pip_namexp_lock( &head->lock );
  return head;
}

static void pip_unlock_hashtab_head( pip_namexp_list_t *head ) {
  pip_namexp_unlock( &head->lock );
}

static pip_hash_t pip_name_hash( const char *name ) {
  pip_hash_t hash = 0;
  int i;

  //DBGF( "%s", name );
  for( i=0; name[i]!='\0'; i++ ) {
    if( !isalnum( name[i] ) ) continue;
    hash <<= 1;
    hash ^= name[i];
  }
  hash += i;
  return hash;
}

static pip_namexp_entry_t*
pip_find_namexp( pip_namexp_list_t *head, pip_hash_t hash, char *name ) {
  pip_list_t		*entry;
  pip_namexp_entry_t	*name_entry;

  PIP_LIST_FOREACH( (pip_list_t*) &head->list, entry ) {
    name_entry = (pip_namexp_entry_t*) entry;
    //DBGF( "entry:%p  hash:0x%lx/0x%lx  name:%s/%s",
    //name_entry, name_entry->hashval, hash, name_entry->name, name );
    if( name_entry->hashval == hash &&
	strcmp( name_entry->name, name ) == 0 ) {
      DBGF( "FOUND -- head:%p  name='%s'", head, name );
      return name_entry;
    }
  }
  DBGF( "NOT found -- head:%p  name='%s'", head, name );
  return NULL;
}

static pip_namexp_entry_t *pip_new_entry( char *name, pip_hash_t hash ) {
  pip_namexp_entry_t *entry;

  entry = (pip_namexp_entry_t*) malloc( sizeof( pip_namexp_entry_t ) );
  DBGF( "entry:%p  %s@0x%lx", entry, name, hash );
  if( entry != NULL ) {
    PIP_LIST_INIT( &entry->list );
    entry->hashval       = hash;
    entry->name          = strdup( name );
    entry->address       = NULL;
    entry->flag_exported = 0;
    entry->flag_canceled = 0;
    pip_task_queue_init( &entry->queue, NULL, NULL );

    if( entry->name == NULL ) {
      free( entry );
      entry = NULL;
    }
  }
  return entry;
}

static pip_namexp_entry_t*
pip_new_export_entry( void *address, char *name, pip_hash_t hash ) {
  pip_namexp_entry_t *entry = pip_new_entry( name, hash );
  if( entry != NULL ) {
    entry->address       = address;
    entry->flag_exported = 1;
  }
  return entry;
}

int pip_named_export( void *exp, const char *format, ... ) {
  pip_named_exptab_t *namexp;
  pip_namexp_entry_t *entry, *new;
  pip_namexp_list_t  *head;
  va_list 	ap;
  pip_hash_t 	hash;
  char 		*name = NULL;
  int 		n, err = 0;

  ENTER;
  va_start( ap, format );
  if( format == NULL ) {
    err = EINVAL;
    goto error;
  }
  if( vasprintf( &name, format, ap ) < 0 || name == NULL ) {
    err = ENOMEM;
    goto error;
  }
  hash = pip_name_hash( name );
  DBGF( "pipid:%d  name:%s", pip_task->pipid, name );

  namexp = (pip_named_exptab_t*) pip_task->annex->named_exptab;
  head = pip_lock_hashtab_head( namexp, hash );
  {
    if( ( entry = pip_find_namexp( head, hash, name ) ) == NULL ) {
      /* no entry yet */
      if( ( entry = pip_new_export_entry( exp, name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	pip_add_namexp( head, entry );
      }
    } else if( !entry->flag_exported ) {
      /* the entry is for query (i.e. this is the first export) */
      entry->address = exp;
      if( ( new = pip_new_export_entry( exp, name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	n = PIP_TASK_ALL;
	err = pip_dequeue_and_resume_N_nolock( &new->queue, NULL, &n );
	if( err ) goto error;
	pip_del_namexp( entry );
	pip_add_namexp( head, new );
	/* note: we cannot free this entry since it might be created by */
	/* another PiP task and this PiP task must free it in this case */
      }
    } else {
      /* already exists */
      err = EBUSY;
    }
  }
  pip_unlock_hashtab_head( head );
  pip_yield();
  free( name );
 error:
  va_end( ap );
  RETURN( err );
}

static int pip_named_import_( int pipid,
			      void **expp,
			      int flag_nblk,
			      const char *format,
			      va_list ap ) {
  void unlock_hashtab( void *arg ) {
    pip_namexp_list_t  	*head = (pip_namexp_list_t*) arg;
    pip_unlock_hashtab_head( head );
  }
  pip_task_internal_t	*taski;
  pip_named_exptab_t 	*namexp;
  pip_namexp_entry_t 	*entry, *new = NULL;
  pip_namexp_list_t  	*head;
  pip_hash_t 		hash;
  void			*address = NULL;
  char 			*name = NULL;
  int 			err = 0;

  ENTER;
  if( format == NULL ) RETURN( EINVAL );
  if( ( err = pip_check_pipid_( &pipid ) ) != 0 ) RETURN( err );
  if( !pip_is_alive( pipid ) ) RETURN( EACCES );
  taski = pip_get_task_( pipid );

  namexp = (pip_named_exptab_t*) taski->annex->named_exptab;
  if( namexp == NULL ) RETURN( EACCES );
  if( vasprintf( &name, format, ap ) < 0 || name == NULL ) RETURN( ENOMEM );
  hash = pip_name_hash( name );

  DBGF( ">> pipid:%d  name:%s", pipid, name );
 retry:
  head = pip_lock_hashtab_head( namexp, hash );
  {
    if( ( entry = pip_find_namexp( head, hash, name ) ) != NULL ) {
      if( entry->flag_exported ) { /* exported already */
	address = entry->address;
      } else {
	/* already queried, but not yet exported */
	if( flag_nblk ) {
	  err = ENOENT;
	} else {
	  pip_suspend_and_enqueue_nolock( &entry->queue,
					  (void*) unlock_hashtab,
					  (void*) head );
	  /* we must retry because the query entry was replaced */
	  if( !new->flag_canceled ) goto retry;
	  err = ECANCELED;
	  goto nounlock;
	}
      }
    } else {			/* not yet exported */
      if( flag_nblk ) {
	err = ENOENT;
      } else if( ( new = pip_new_entry( name, hash ) ) == NULL ) {
	err = ENOMEM;
      } else {
	pip_add_namexp( head, new );
	pip_suspend_and_enqueue_nolock( &new->queue,
					(void*) unlock_hashtab,
					(void*) head );
	/* this entry was created by myself and it can be freed by myself */
	free( new->name );
	free( new );
	/* we must retry because the query entry was replaced */
	if( !new->flag_canceled ) goto retry;
	err = ECANCELED;
	goto nounlock;
      }
    }
  }
  pip_unlock_hashtab_head( head );
 nounlock:
  DBGF( "<< pipid:%d  name:%s", pipid, name );
  free( name );
  if( !err ) *expp = address;
  RETURN( err );
}

int pip_named_import( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  int err;
  va_start( ap, format );
  err = pip_named_import_( pipid, expp, 0, format, ap );
  va_end( ap );
  RETURN( err );
}

int pip_named_tryimport( int pipid, void **expp, const char *format, ... ) {
  va_list ap;
  int err;
  va_start( ap, format );
  err = pip_named_import_( pipid, expp, 1, format, ap );
  va_end( ap );
  RETURN( err );
}

void pip_named_export_fin_( pip_task_internal_t *taski ) {
  pip_named_exptab_t	*namexp;
  pip_namexp_list_t	*head;
  pip_namexp_entry_t	*entry;
  pip_list_t		*list, *next;
  int 			n, i;

  ENTER;
  DBGF( "PIPID:%d", taski->pipid );
  namexp = (pip_named_exptab_t*) taski->annex->named_exptab;
  if( namexp != NULL ) {
    ASSERT( namexp->pipid != taski->pipid );
    for( i=0; i<PIP_HASHTAB_SZ; i++ ) {
      head = &namexp->hash_table[i];
      pip_spin_lock( &head->lock );
      PIP_LIST_FOREACH_SAFE( (pip_list_t*) &head->list, list, next ) {
	PIP_LIST_DEL( list );
	entry = (pip_namexp_entry_t*) list;
	pip_del_namexp( entry );
	if( entry->flag_exported ) {
	  free( entry->name );
	  free( entry );
	} else {
	  int err;
	  /* the is a query entry, it must be free()ed by the query task */
	  entry->flag_canceled = 1;
	  n = PIP_TASK_ALL;
	  err = pip_dequeue_and_resume_N_nolock(&entry->queue, NULL, &n);
	  ASSERT( err );
	}
      }
      pip_spin_unlock( &head->lock );
    }
  }
  RETURNV;
}

void pip_named_export_fin_all_( void ) {
  int i;

  DBGF( "pip_root->ntasks:%d", pip_root->ntasks );
  for( i=0; i<pip_root->ntasks; i++ ) {
    DBGF( "PiP task: %d", i );
    free( pip_root->tasks[i].annex->named_exptab );
    pip_root->tasks[i].annex->named_exptab = NULL;
  }
  DBGF( "PiP root" );
  (void) pip_named_export_fin_( pip_root->task_root );
  free( pip_root->task_root->annex->named_exptab );
  pip_root->task_root->annex->named_exptab = NULL;
  DBG;
}
