/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
 */

#define _GNU_SOURCE

#include <sys/syscall.h>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>
#include <elf.h>

#include <pip.h>
#include <pip_ulp.h>
#include <pip_internal.h>
#include <pip_util.h>

extern int pip_is_coefd( int );
extern int pip_get_dso( int pipid, void **loaded );

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

static char *pip_type_str_( int type ) {
  char *typestr;

  switch( type ) {
  case PIP_TYPE_ROOT:
    typestr= "root";
    break;
  case PIP_TYPE_TASK:
    typestr = "task";
    break;
  case PIP_TYPE_ULP:
    typestr = "ulp";
    break;
  default:
    typestr = "(unknown)";
  }
  return typestr;
}

char * pip_type_str( void ) {
  char *typestr;

  if( pip_task == NULL ) {
    typestr = "(NULL)";
  } else {
    typestr = pip_type_str_( pip_task->type );
  }
  return typestr;
}

pid_t pip_gettid( void ) {
  return (pid_t) syscall( (long int) SYS_gettid );
}

int pip_idstr( char *buf, size_t sz ) {
  pid_t	pid = pip_gettid();
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

static void pip_message( FILE *fp, char *tagf, char *format, va_list ap ) {
#define PIP_MESGLEN		(512)
  char mesg[PIP_MESGLEN];
  char idstr[PIP_MIDLEN];
  int len;

  len = pip_idstr( idstr, PIP_MIDLEN );
  len = snprintf( &mesg[0], PIP_MESGLEN-len, tagf, idstr );
  vsnprintf( &mesg[len], PIP_MESGLEN-len, format, ap );
  fprintf( stderr, "%s\n", mesg );
}

void pip_info_fmesg( FILE *fp, char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  if( fp == NULL ) fp = stderr;
  pip_message( fp, "PiP-INFO%s ", format, ap );
}

void pip_info_mesg( char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "PiP-INFO%s ", format, ap );
}

void pip_warn_mesg( char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "PiP-WARN%s ", format, ap );
}

void pip_err_mesg( char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "PiP-ERROR%s ", format, ap );
}

int pip_check_pipid( int* );
pip_task_t *pip_get_task_( int );

void pip_ulp_describe( FILE *fp, const char *tag, pip_ulp_t *ulp ) {
  if( ulp != NULL ) {
    pip_task_t *task = PIP_TASK( ulp );
    char *typestr = pip_type_str_( task->type );
    if( tag == NULL ) {
      pip_info_fmesg( fp,
		      "%s[%d](sched:%p,ctx=%p)@%p",
		      typestr,
		      task->pipid,
		      task->task_sched,
		      task->ctx_suspend,
		      task );
    } else {
      pip_info_fmesg( fp,
		      "%s: %s[%d](sched:%p,ctx=%p)@%p",
		      tag,
		      typestr,
		      task->pipid,
		      task->task_sched,
		      task->ctx_suspend,
		      task );
    }
  } else {
    if( tag == NULL ) {
      pip_info_fmesg( fp, "ULP:(nil)" );
    } else {
      pip_info_fmesg( fp, "%s: ULP:(nil)", tag );
    }
  }
}

void pip_task_describe( FILE *fp, const char *tag, int pipid ) {
  if( pip_check_pipid( &pipid ) == 0 ) {
    pip_task_t *task = pip_get_task_( pipid );
    char *typestr = pip_type_str_( task->type );
    if( tag == NULL ) {
      pip_info_fmesg( fp,
		      "%s[%d]@%p (sched:%p,ctx:%p)",
		      typestr,
		      pipid,
		      task,
		      task->task_sched,
		      task->ctx_suspend );
    } else {
      pip_info_fmesg( fp,
		      "%s: %s[%d]@%p (sched:%p,ctx:%p)",
		      tag,
		      typestr,
		      pipid,
		      task,
		      task->task_sched,
		      task->ctx_suspend );
    }
  } else {
    if( tag == NULL ) {
      pip_info_fmesg( fp, "TASK:(pipid:%d is invlaid)", pipid );
    } else {
      pip_info_fmesg( fp, "%s: TASK:(pipid:%d is invlaid)", tag, pipid );
    }
  }
}

void pip_ulp_queue_describe( FILE *fp, const char *tag, pip_ulp_t *queue ) {
  if( queue == NULL ) {
    if( tag == NULL ) {
      pip_info_fmesg( fp, "QUEUE:(nil)" );
    } else {
      pip_info_fmesg( fp, "%s: QUEUE:(nil)", tag );
    }
  } else if( PIP_ULP_ISEMPTY( queue ) ) {
    if( tag == NULL ) {
      pip_info_fmesg( fp, "QHEAD:%p  EMPTY", queue );
    } else {
      pip_info_fmesg( fp, "%s: QHEAD:%p  EMPTY", tag, queue );
    }
  } else {
    pip_task_t 	*t;
    pip_ulp_t 	*u;
    int 	i = 0;

    if( tag == NULL ) {
      pip_info_fmesg( fp, "QHEAD:%p (next:%p prev:%p)",
		      queue, queue->next, queue->prev );
    } else {
      pip_info_fmesg( fp, "%s: QHEAD:%p (next:%p prev:%p)",
		      tag, queue, queue->next, queue->prev );
    }
    PIP_ULP_FOREACH( queue, u ) {
      t = PIP_TASK( u );
      if( tag == NULL ) {
	pip_info_fmesg( fp,
			"QUEUE[%d] pipid:%d "
			"(sched:%p,ctx=%p)@%p next:%p prev=%p",
			i,
			t->pipid,
			t->task_sched,
			t->ctx_suspend,
			t,
			u->next,
			u->prev );
      } else {
	pip_info_fmesg( fp,
			"%s: QUEUE[%d] pipid:%d "
			"(sched:%p,ctx=%p)@%p next:%p prev=%p",
			tag,
			i,
			t->pipid,
			t->task_sched,
			t->ctx_suspend,
			t,
			u->next,
			u->prev );
      }
      if( u->next == u->prev ) {
	if( tag == NULL ) {
	  pip_info_fmesg( fp, "QUEUE looped list !!!!!!!!!!!!!" );
	} else {
	  pip_info_fmesg( fp, "%s: QUEUE looped list !!!!!!!!!!!!!", tag );
	}
	break;
      }
      i++;
    }
  }
}

/* the following function(s) are for debugging */

#define PIP_DEBUG_BUFSZ		(4096)

void pip_fprint_maps( FILE *fp ) {
  int fd = open( "/proc/self/maps", O_RDONLY );
  char buf[PIP_DEBUG_BUFSZ];

  while( 1 ) {
    ssize_t rc;
    size_t  wc;
    char *p;

    if( ( rc = read( fd, buf, PIP_DEBUG_BUFSZ ) ) <= 0 ) break;
    p = buf;
    do {
      wc = fwrite( p, 1, rc, fp );
      p  += wc;
      rc -= wc;
    } while( rc > 0 );
  }
  close( fd );
}

void pip_print_maps( void ) { pip_fprint_maps( stdout ); }

void pip_fprint_fd( FILE *fp, int fd ) {
  char idstr[64];
  char fdpath[512];
  char fdname[256];
  ssize_t sz;

  pip_idstr( idstr, 64 );
  sprintf( fdpath, "/proc/self/fd/%d", fd );
  if( ( sz = readlink( fdpath, fdname, 256 ) ) > 0 ) {
    fdname[sz] = '\0';
    fprintf( fp, "%s %d -> %s", idstr, fd, fdname );
  }
}

void pip_print_fd( int fd ) { pip_fprint_fd( stderr, fd ); }

void pip_fprint_fds( FILE *fp ) {
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
	  fprintf( fp, "%s %s -> %s %c", idstr, fdpath, fdname, coe );
	} else {
	  fprintf( fp, "%s %s -> %s  opendir(\"/proc/self/fd\")",
		   idstr, fdpath, fdname );
	}
      }
    }
    closedir( dir );
  }
}

void pip_print_fds( void ) { pip_fprint_fds( stderr ); }

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

void pip_fprint_loaded_solibs( FILE *fp ) {
  void *handle = NULL;
  char idstr[PIP_MIDLEN];
  int err;

  /* pip_init() must be called in advance */
  (void) pip_idstr( idstr, PIP_MIDLEN );

  if( ( err = pip_get_dso( PIP_PIPID_MYSELF, &handle ) ) != 0 ) {
    pip_info_fmesg( fp, "%s (no solibs found: %d)\n", idstr, err );
  } else {
    struct link_map *map = (struct link_map*) handle;
    for( ; map!=NULL; map=map->l_next ) {
      char *fname;
      if( *map->l_name == '\0' ) {
	fname = "(noname)";
      } else {
	fname = map->l_name;
      }
      pip_info_fmesg( fp, "%s %s at %p\n", idstr, fname, (void*)map->l_addr );
    }
  }
  if( pip_is_root() && handle != NULL ) dlclose( handle );
}

void pip_print_loaded_solibs( FILE *file ) {
  pip_fprint_loaded_solibs( file );
}

static int pip_dsos_cb_( struct dl_phdr_info *info, size_t size, void *data ) {
  FILE *fp = (FILE*) data;
  int i;

  fprintf( fp, "name=%s (%d segments)\n", info->dlpi_name, info->dlpi_phnum);

  for ( i=0; i<info->dlpi_phnum; i++ ) {
    fprintf( fp, "\t\t header %2d: address=%10p\n", i,
	     (void *) (info->dlpi_addr + info->dlpi_phdr[i].p_vaddr ) );
  }
  return 0;
}

void pip_fprint_dsos( FILE *fp ) {
  dl_iterate_phdr( pip_dsos_cb_, (void*) fp );
}

void pip_print_dsos( void ) {
  FILE *fp = stderr;
  dl_iterate_phdr( pip_dsos_cb_, (void*) fp );
}
