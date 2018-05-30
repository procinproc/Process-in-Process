/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
 */

#define _GNU_SOURCE

#include <dlfcn.h>
#include <elf.h>

#include <pip.h>
#include <pip_ulp.h>
#include <pip_internal.h>
#include <pip_util.h>

extern int pip_is_coefd( int );
extern int pip_get_dso( int pipid, void **loaded );
extern int pip_is_root_( void );

int pip_check_pie( char *path ) {
  Elf64_Ehdr elfh;
  int fd;
  int err = 0;

  if( ( fd = open( path, O_RDONLY ) ) < 0 ) {
    err = errno;
  } else {
    if( read( fd, &elfh, sizeof( elfh ) ) != sizeof( elfh ) ) {
      pip_warn_mesg( "Unable to read '%s'", path );
      err = EUNATCH;
    } else if( elfh.e_ident[EI_MAG0] != ELFMAG0 ||
	       elfh.e_ident[EI_MAG1] != ELFMAG1 ||
	       elfh.e_ident[EI_MAG2] != ELFMAG2 ||
	       elfh.e_ident[EI_MAG3] != ELFMAG3 ) {
      pip_warn_mesg( "'%s' is not an ELF file", path );
      err = EUNATCH;
    } else if( elfh.e_type != ET_DYN ) {
      pip_warn_mesg( "'%s' is not DYNAMIC (PIE)", path );
      err = ELIBEXEC;
    }
    (void) close( fd );
  }
  return err;
}

char *pip_pipidstr( char *buf ) {
  char *idstr;

  switch( pip_task->pipid ) {
  case PIP_PIPID_ROOT:
    idstr = "?root?";
    break;
  case PIP_PIPID_MYSELF:
    idstr = "?myself?";
    break;
  case PIP_PIPID_ANY:
    idstr = "?myself?";
    break;
  case PIP_PIPID_NULL:
    idstr = "?null?";
    break;
  default:
    (void) sprintf( buf, "%d", pip_task->pipid );
    idstr = buf;
    break;
  }
  return idstr;
}

char * pip_type_str( void ) {
  char *typestr = NULL;
  if( pip_task != NULL ) {
    switch( pip_task->type ) {
    case PIP_TYPE_ROOT:
      typestr= "root";
      break;
    case PIP_TYPE_TASK:
      typestr = "task";
      break;
    case PIP_TYPE_ULP:
      typestr = "ulp";
      break;
    }
  }
  return typestr;
}

int pip_idstr( char *buf, size_t sz ) {
  pid_t	pid = getpid();
  char *pre  = "<";
  char *post = ">";
  char *idstr, idnum[64];
  int	n = 0;

  if( pip_task == NULL ) {
    n = snprintf( buf, sz, "%snotask:(%d)%s", pre, pid, post );
  } else {
    switch( pip_task->type ) {
    case PIP_TYPE_ROOT:
      n = snprintf( buf, sz, "%sROOT:(%d)%s", pre, pid, post );
      break;
    case PIP_TYPE_TASK:
      idstr = pip_pipidstr( idnum );
      n = snprintf( buf, sz, "%sTSK:%s(%d)%s", pre, idstr, pid, post );
      break;
    case PIP_TYPE_ULP:
      idstr = pip_pipidstr( idnum );
      n = snprintf( buf, sz, "%sULP:%s(%d)%s", pre, idstr, pid, post );
      break;
    case PIP_TYPE_NONE:
      n = snprintf( buf, sz, "%s\?\?\?\?(%d)%s", pre, pid, post );
      break;
    default:
      n = snprintf( buf, sz, "%sType:0x%x(%d)%s ",
		    pre, pip_task->type, pid, post );
      break;
    }
  }
  return n;
}

static void pip_message( char *tagf, char *format, va_list ap ) {
#define PIP_MESGLEN		(512)
  char mesg[PIP_MESGLEN];
  char idstr[PIP_MIDLEN];
  int len;

  len = pip_idstr( idstr, PIP_MIDLEN );
  len = snprintf( &mesg[0], PIP_MESGLEN-len, tagf, idstr );
  vsnprintf( &mesg[len], PIP_MESGLEN-len, format, ap );
  fprintf( stderr, "%s\n", mesg );
}

void pip_info_mesg( char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( "PiP-INFO%s ", format, ap );
}

void pip_warn_mesg( char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( "PiP-WARN%s ", format, ap );
}

void pip_err_mesg( char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( "PiP-ERROR%s ", format, ap );
}

int pip_check_pipid( int* );
pip_task_t *pip_get_task_( int );

void pip_ulp_describe( FILE *fp, const char *tag, pip_ulp_t *ulp, int flags ) {
  if( ulp != NULL ) {
    pip_task_t *task = PIP_TASK( ulp );
    if( tag == NULL ) {
      pip_info_mesg( "ULP[%d](ctx=%p)@%p",
		     task->pipid,
		     task->ctx_suspend,
		     task );
    } else {
      pip_info_mesg( "%s:ULP[%d](ctx=%p)@%p",
		     tag,
		     task->pipid,
		     task->ctx_suspend,
		     task );
    }
  } else {
    if( tag == NULL ) {
      pip_info_mesg( "ULP:(nil)" );
    } else {
      pip_info_mesg( "%s: ULP:(nil)", tag );
    }
  }
}

void pip_task_describe( FILE *fp, const char *tag, int pipid, int flags ) {
  pip_task_t *task;

  DBG;
  if( pip_check_pipid( &pipid ) == 0 ) {
    task = pip_get_task_( pipid );
    if( tag == NULL ) {
      pip_info_mesg( "%p (sched:%p,ctx:%p)",
		     task,
		     task->task_sched,
		     task->ctx_suspend );
    } else {
      pip_info_mesg( "%s: %p (sched:%p,ctx:%p)",
		     tag,
		     task,
		     task->task_sched,
		     task->ctx_suspend );
    }
  } else {
    if( tag == NULL ) {
      pip_info_mesg( "TASK:(pipid:%d is invlaid)", pipid );
    } else {
      pip_info_mesg( "%s: TASK:(pipid:%d is invlaid)", tag, pipid );
    }
  }
}

void pip_ulp_queue_describe( FILE *fp, const char *tag, pip_ulp_t *queue ) {
  DBG;
  if( queue != NULL ) {
    pip_task_t 	*t;
    pip_ulp_t 	*u;
    int i;

    if( tag == NULL ) {
      pip_info_mesg( "QUEUE:%p (next:%p prev:%p)",
		     queue, queue->next, queue->prev );
    } else {
      pip_info_mesg( "QUEUE:%p (next:%p prev:%p)",
		     queue, queue->next, queue->prev );
    }
    i = 0;
    PIP_ULP_FOREACH( queue, u ) {
      t = PIP_TASK( u );
      if( tag == NULL ) {
	pip_info_mesg( "(%d) pipid:%d "
		       "(ctx=%p):%p  next:%p  prev=%p",
		       i,
		       t->pipid,
		       t->ctx_suspend,
		       t,
		       u->next,
		       u->prev );
      } else {
	pip_info_mesg( "(%d) pipid:%d %s: "
		       "(ctx=%p):%p  next:%p  prev=%p",
		       i,
		       t->pipid,
		       tag,
		       t->ctx_suspend,
		       t,
		       u->next,
		       u->prev );
      }
      i++;
    }
  } else {
    if( tag == NULL ) {
      pip_info_mesg( "TASK:(nil)" );
    } else {
      pip_info_mesg( "%s: TASK:(nil)", tag );
    }
  }
}

/* the following function(s) are for debugging */

#define PIP_DEBUG_BUFSZ		(4096)

void pip_print_maps( void ) {
  int fd = open( "/proc/self/maps", O_RDONLY );
  char buf[PIP_DEBUG_BUFSZ];

  while( 1 ) {
    ssize_t rc, wc;
    char *p;

    if( ( rc = read( fd, buf, PIP_DEBUG_BUFSZ ) ) <= 0 ) break;
    p = buf;
    do {
      if( ( wc = write( 1, p, rc ) ) < 0 ) { /* STDOUT */
	fprintf( stderr, "write error\n" );
	goto error;
      }
      p  += wc;
      rc -= wc;
    } while( rc > 0 );
  }
 error:
  close( fd );
}

void pip_print_fd( int fd ) {
  char idstr[64];
  char fdpath[512];
  char fdname[256];
  ssize_t sz;

  pip_idstr( idstr, 64 );
  sprintf( fdpath, "/proc/self/fd/%d", fd );
  if( ( sz = readlink( fdpath, fdname, 256 ) ) > 0 ) {
    fdname[sz] = '\0';
    fprintf( stderr, "%s %d -> %s", idstr, fd, fdname );
  }
}

void pip_print_fds( void ) {
  DIR *dir = opendir( "/proc/self/fd" );
  struct dirent *de;
  char idstr[64];
  char fdpath[512];
  char fdname[256];
  char coe = ' ';
  ssize_t sz;

  pip_idstr( idstr, 64 );
  if( dir != NULL ) {
    int fd_dir = dirfd( dir );
    int fd;

    while( ( de = readdir( dir ) ) != NULL ) {
      sprintf( fdpath, "/proc/self/fd/%s", de->d_name );
      if( ( sz = readlink( fdpath, fdname, 256 ) ) > 0 ) {
	fdname[sz] = '\0';
	if( ( fd = atoi( de->d_name ) ) != fd_dir ) {
	  if( pip_is_coefd ( fd ) ) coe = '*';
	  fprintf( stderr, "%s %s -> %s %c", idstr, fdpath, fdname, coe );
	} else {
	  fprintf( stderr, "%s %s -> %s  opendir(\"/proc/self/fd\")",
		   idstr, fdpath, fdname );
	}
      }
    }
    closedir( dir );
  }
}

void pip_check_addr( char *tag, void *addr ) {
  FILE *maps = fopen( "/proc/self/maps", "r" );
  char idstr[64];
  char *line = NULL;
  size_t sz  = 0;
  int retval;

  if( tag == NULL ) {
    pip_idstr( idstr, 64 );
    tag = &idstr[0];
  }
  while( maps != NULL ) {
    void *start, *end;

    if( ( retval = getline( &line, &sz, maps ) ) < 0 ) {
      fprintf( stderr, "getline()=%d\n", errno );
      break;
    } else if( retval == 0 ) {
      continue;
    }
    line[retval] = '\0';
    if( sscanf( line, "%p-%p", &start, &end ) == 2 ) {
      if( (intptr_t) addr >= (intptr_t) start &&
	  (intptr_t) addr <  (intptr_t) end ) {
	fprintf( stderr, "%s %p: %s", tag, addr, line );
	goto found;
      }
    }
  }
  fprintf( stderr, "%s %p (not found)\n", tag, addr );
 found:
  if( line != NULL ) free( line );
  fclose( maps );
}

double pip_gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

void pip_print_loaded_solibs( FILE *file ) {
  void *handle = NULL;
  char idstr[PIP_MIDLEN];
  int err;

  /* pip_init() must be called in advance */
  (void) pip_idstr( idstr, PIP_MIDLEN );
  if( file == NULL ) file = stderr;

  if( ( err = pip_get_dso( PIP_PIPID_MYSELF, &handle ) ) != 0 ) {
    fprintf( file, "%s (no solibs found: %d)\n", idstr, err );
  } else {
    struct link_map *map = (struct link_map*) handle;
    for( ; map!=NULL; map=map->l_next ) {
      char *fname;
      if( *map->l_name == '\0' ) {
	fname = "(noname)";
      } else {
	fname = map->l_name;
      }
      fprintf( file, "%s %s at %p\n", idstr, fname, (void*)map->l_addr );
    }
  }
  if( pip_is_root_() && handle != NULL ) dlclose( handle );
}

static int
pip_print_dsos_cb_( struct dl_phdr_info *info, size_t size, void *data ) {
  int i;

  printf( "name=%s (%d segments)\n", info->dlpi_name, info->dlpi_phnum);

  for ( i=0; i<info->dlpi_phnum; i++ ) {
    printf( "\t\t header %2d: address=%10p\n", i,
	    (void *) (info->dlpi_addr + info->dlpi_phdr[i].p_vaddr ) );
  }
  return 0;
}

void pip_print_dsos( void ) {
  dl_iterate_phdr( pip_print_dsos_cb_, NULL );
}
