/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _pip_h_
#define _pip_h_

/** \mainpage pip Overview of Process-in-Process (PiP)
 *
 * \section overview Overview
 *
 * PiP is a user-level library which allows a process to
 * create sub-processes into the same virtual address space where the
 * parent process runs. The parent process and sub-processes share the
 * same address space, however, each process has its own
 * variables. So, each process runs independently from the other
 * process. If some or all processes agreed, then data own by a
 * process can be accessed by the other processes.
 *
 * Those processes share the same address space, just like pthreads,
 * and each process has its own variables like a process. The parent
 * process is called \e PiP \e process and sub-processes are called
 * \e PiP \e task since it has the best of the both worlds of
 * processes and pthreads.
 *
 * PiP root spawns PiP tasks and the PiP root and PiP tasks shared the
 * same address space. To load multiple instances of a program in the
 * same address space, the executiable of the PiP task must be
 * compiled and linked as PIE (Postion Independent Executable).
 *
 * When a PiP root or PiP task wants to be accessed the its own data
 * by the other(s), firstly a memory region where the data to be
 * accessed are located must be \e exported. Then the exported memory
 * region is \e imported so that the exported and imported data can be
 * accessed. The PiP library supports the functions to export and
 * import the memory region to be accessible.
 *
 * \section execution-mode Execution mode
 *
 * There are several PiP implementations which can be selected at the
 * runtime. These implementations can be categorized into two
 * according to the behaviour of PiP tasks,
 *
 * - \c Pthread, and
 * - \c Process.
 *
 * In the pthread mode, although each PiP task has its own variables
 * unlike thread, PiP task behaves more like Pthread, having no PID,
 * having the same file descriptor space, having the same signal
 * delivery semantics as Pthread does, and so on.
 *
 * In the process mode, PiP task beahve more like a process, having
 * a PID, having an independent file descriptor space, having the same
 * signal delivery semantics as Linux process does, and so on.
 *
 * When the \c PIP_MODE environment variable set to \"thread\" then
 * the PiP library runs based on the pthread mode, and it is set to
 * \"process\" then it runs with the process mode.
 *
 * \section limitation Limitation
 *
 * PiP allows PiP root and PiP tasks to share the data, so the
 * function pointer can be passed to the others. However, jumping into
 * the code owned by the other will not work properly for some
 * reasons.
 *
 * \section compile-and-link Compile and Link User programs
 *
 * The PiP root ust be linked with the PiP library and libpthread. The
 * programs able to run as a PiP task must be compiled with the
 * \"-fpie\" compile option and the \"-pie -rdynamic\" link options.
 *
 * \section glibc-issues GLIBC issues
 *
 * The PiP library is implemented at the user-level, i.e. no need of
 * kernel patches nor kernel modules. Due to the novel usage
 * combiningf \c dlmopn() GLIBC function and \c clone() systemcall,
 * there are some issues found in the GLIBC. To avoid GLIBC issues,
 * PiP users must have the pacthed GLIBC provided by the PiP
 * development team. Otherwise the PiP library will not run properly.
 *
 * \section gdb-issue GDB issue
 *
 * Currently gdb debugger only works with the PiP root. PiP-aware GDB
 * is already scheduled to develop.
 *
 * \section author Author
 *  Atsushi Hori (RIKEN, Japan) ahori@riken.jp
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define PIP_OPTS_NONE			(0x0)
#define PIP_OPTS_ANY			PIP_OPTS_NONE

#define PIP_MODE_PTHREAD		(0x100)
#define PIP_MODE_PROCESS		(0x200)
/* the following two modes are a submode of PIP_MODE_PROCESS */
#define PIP_MODE_PROCESS_PRELOAD	(0x210)
#define PIP_MODE_PROCESS_PIPCLONE	(0x220)
#define PIP_MODE_MASK			(0xFF0)

#define PIP_ENV_MODE			"PIP_MODE"
#define PIP_ENV_MODE_THREAD		"thread"
#define PIP_ENV_MODE_PTHREAD		"pthread"
#define PIP_ENV_MODE_PROCESS		"process"
#define PIP_ENV_MODE_PROCESS_PRELOAD	"process:preload"
#define PIP_ENV_MODE_PROCESS_PIPCLONE	"process:pipclone"

#define PIP_OPT_FORCEEXIT		(0x1)

#define PIP_ENV_OPTS			"PIP_OPTS"
#define PIP_ENV_OPTS_FORCEEXIT		"forceexit"

#define PIP_VALID_OPTS	\
  ( PIP_MODE_PTHREAD | PIP_MODE_PROCESS_PRELOAD | PIP_MODE_PROCESS_PIPCLONE | \
    PIP_OPT_FORCEEXIT )

#define PIP_ENV_STACKSZ		"PIP_STACKSZ"

#define PIP_PIPID_ROOT		(-1)
#define PIP_PIPID_ANY		(-2)
#define PIP_PIPID_MYSELF	(-3)

#define PIP_NTASKS_MAX		(100)

#define PIP_CPUCORE_ASIS 	(-1)

typedef int(*pip_spawnhook_t)(void*);

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <link.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @addtogroup libpip libpip
 * \brief the PiP library
 * @{
 * @file
 * @{
 */

  /**
   *
   * \brief Initialize the PiP library.
   *  @{
   * \param[out] pipidp When this is called by the PiP root
   *  process, then this returns PIP_PIPID_ROOT, otherwise it returns
   *  the PIPID of the calling PiP task.
   * \param[in,out] ntasks When called by the PiP root, it specifies
   *  the maxmum number of PiP tasks. When called by a PiP task, then
   *  it returns the number specified by the PiP root.
   * \param[in,out] root_expp If the root PiP is ready to export a
   *  memory region to any PiP task(s), then this parameter points to
   *  the variable holding the exporting address of the root PiP. If
   *  the PiP root is not ready to export or has nothing to export
   *  then this variable can be NULL. When called by a PiP task, it
   *  returns the exporting address of the PiP root, if any.
   * \param[in] opts This must be zero at the point of this writing.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * This function initialize the PiP library. The PiP root process
   * must call this. A PiP task is not required to call this function
   * unless the PiP task calls any PiP functions.
   *
   * Is is NOT guaranteed that users can spawn tasks up to the number
   * specified by the \a ntasks argument. There are some limitations
   * come from outside of the PiP library.
   *
   * \sa pip_export(3), pip_fin(3)
   */
  int pip_init( int *pipidp, int *ntasks, void **root_expp, int opts );
  /** @}*/

  /**
   * \brief finalize the PiP library.
   *  @{
   * \return Return 0 on success. Return an error code on error.
   *
   * This function finalize the PiP library.
   *
   * \sa pip_init(3)
   */
  int pip_fin( void );
  /** @}*/

  /**
   * \brief spawn a PiP task
   *  @{
   * \param[in] filename The executable to run as a PiP task
   * \param[in] argv Argument(s) for the spawned PiP task
   * \param[in] envv Environment variables for the spawned PiP task
   * \param[in] coreno Core number for the PiP task to be bound to. If
   *  PIP_CPUCORE_ASIS is specified, then the core binding will not
   *  take place.
   * \param[in,out] pipidp Specify PIPID of the spanwed PiP task. If
   *  \c PIP_PIPID_ANY is specified, then the PIPID of the spawned PiP
   *  task is up to the PiP library and the assigned PIPID will be
   *  returned.
   * \param[in] before Just before the executing of the spanwed PiP
   *  task, this function is called so that file descriptors inherited
   *  from the PiP root, for example, can deal with. This is only
   *  effective with the PiP process mode. This function is called
   *  with the argument \a hookarg described below.
   * \param[in] after This function is called when the PiP task
   *  terminates for the cleanup puropose. This function is called
   *  with the argument \a hookarg described below.
   * \param[in] hookarg The argument for the \a before and \a after
   *  function call.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * This function behave like the Linux \c execve() function to spanw
   * a PiP task. These functions are introduced to follow the
   * programming style of conventional \c fork and \c
   * exec. \a before function does the prologue found between the
   * \c fork and \c exec. \a after function is to free the argument if
   * it is \c malloc()ed. Note that the \a before and \a after
   * functions are called in the different \e context from the spawned
   * PiP task. More specifically, the variables defined in the spawned
   * PiP task cannot be accessible from the \a before and \a after
   * functions.
   *
   * \note In the current implementation, the spawned PiP task cannot
   * be freed even if the spawned PiP task terminates. To fix this,
   * hack on GLIBC (ld-linud.so) is required.
   *
   * \note In theory, there is no reason to restrict for a PiP task to
   * spawn another PiP task. However, the current implementation fails
   * to do so.
   */
  int pip_spawn( char *filename, char **argv, char **envv,
		 int coreno, int *pipidp,
		 pip_spawnhook_t before, pip_spawnhook_t after, void *hookarg);
  /** @}*/

  /**
   * \brief export a memory region of the calling PiP root or a PiP task to
   * the others.
   *  @{
   * \param[in] exp Starting address of a memory region of the calling
   *  process or task to the others.
   *  function call.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * The PiP root or a PiP task can export a memory region only
   * once. If the PiP task calls the \c pip_init() with the non-null
   * value of the \a expp parameter, then the function call by the
   * root PiP will fail.
   *
   * \note There is no size parameter to specify the length of the
   * exported region because there is no way to restrict the access
   * outside of the exported region.
   *
   * \sa pip_import(3)
   */
  int pip_export( void *exp );
  /** @}*/

  /**
   * \brief import the exposed memory region of the other.
   *  @{
   * \param[in] pipid The PIPID to import the exposed address
   * \param[out] expp The starting address of the exposed region of
   *  the PiP task specified by the \a pipid.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note It is the users' responsiblity to synchronize. When the
   * target exported region is not ready, then this function returns
   * NULL. If the root exports its region by the \c pip_init function
   * call, then it is guaranteed to be imported by PiP tasks at any
   * time.
   *
   * \sa pip_export(3)
   */
  int pip_import( int pipid, void **expp );
  /** @}*/

  /**
   * \brief get PIPID
   *  @{
   * \param[out] pipidp This parameter points to the variable which
   *  will be set to the PIPID of the calling process.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_get_pipid( int *pipidp );
  /** @}*/

  /**
   * \brief get the maxmum number of the PiP tasks
   *  @{
   * \param[out] ntasksp This parameter points to the variable which
   *  will be set to the maxmum number of the PiP tasks.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_get_ntasks( int *ntasksp );
  /** @}*/

  /**
   * \brief check if the calling task is a PiP task or not
   *  @{
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note Unlike most of the other PiP functions, this can be called
   * BEFORE calling the \c pip_init() function.
   */
  int pip_isa_piptask( void );
  /** @}*/

  /**
   * \brief get the PiP execution mode
   *  @{
   * \param[out] modep This parameter points to the variable which
   *  will be set to the PiP execution mode
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_get_mode( int *modep );
  /** @}*/

  /**
   * \brief terminate PiP task
   *  @{
   * \param[in] retval Terminate PiP task with the exit number
   * specified with this parameter.
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_exit( int retval );
  /** @}*/

  /**
   * \brief wait for the termination of a PiP task
   *  @{
   * \param[in] pipid PIPID to wait for.
   * \param[out] retval Exit value of the terminated PiP task
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_wait( int pipid, int *retval );
  /** @}*/

  /**
   * \brief wait for the termination of a PiP task in a non-blocking way
   *  @{
   * \param[in] pipid PIPID to wait for.
   * \param[out] retval Exit value of the terminated PiP task
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_trywait( int pipid, int *retval );
  /** @}*/

  /**
   * \brief deliver a signal to a PiP task
   *  @{
   * \param[out] pipid PIPID of a target PiP task
   * \param[out] signal signal number to be delivered
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_kill( int pipid, int signal );
  /** @}*/

  /**
   * \brief deliver a process or thread ID
   *  @{
   * \param[out] pipid PIPID of a target PiP task
   * \param[out] idp a pointer to store the ID value
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   *
   */
  int pip_get_id( int pipid, intptr_t *idp );
  /** @}*/

  /**
   * \brief get a string of the current execution mode
   *  @{
   *
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return the name string of the current execution mode
   *
   */
  const char *pip_get_mode_str( void );
  /** @}*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef PIP_INTERNAL_FUNCS

  /* the following functions depends its implementation deeply */

  int pip_get_thread( int pipid, pthread_t *threadp );
  int pip_if_pthread( int *flagp );
  int pip_if_shared_fd( int *flagp );
  int pip_if_shared_sighand( int *flagp );

  char **pip_copy_vec( char *addition, char **vecsrc );

#endif /* PIP_INTERNAL_FUNCS */

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/********************************************************/
/* The llowing functions are just for utility for debug */
/********************************************************/

#define PIP_DEBUG_BUFSZ		(4096)

inline static void pip_print_maps( void ) __attribute__ ((unused));
inline static void pip_print_maps( void ) {
  int fd = open( "/proc/self/maps", O_RDONLY );
  char buf[PIP_DEBUG_BUFSZ];

  while( 1 ) {
    ssize_t rc, wc;
    char *p;

    if( ( rc = read( fd, buf, PIP_DEBUG_BUFSZ ) ) <= 0 ) break;
    p = buf;
    do {
      if( ( wc = write( 1, p, rc ) ) < 0 ) break; /* STDOUT */
      p  += wc;
      rc -= wc;
    } while( rc > 0 );
  }
  close( fd );
}

#define PIP_MTAG	"PiP:"

inline static void pip_print_fd( int ) __attribute__ ((unused));
inline static void pip_print_fd( int fd ) {
  char fdpath[512];
  char fdname[256];
  ssize_t sz;

  sprintf( fdpath, "/proc/self/fd/%d", fd );
  if( ( sz = readlink( fdpath, fdname, 256 ) ) > 0 ) {
    fdname[sz] = '\0';
    fprintf( stderr, "%s %d -> %s", PIP_MTAG, fd, fdname );
  }
}

inline static void pip_print_fds( void ) __attribute__ ((unused));
inline static void pip_print_fds( void ) {
  DIR *dir = opendir( "/proc/self/fd" );
  struct dirent *de;
  char fdpath[512];
  char fdname[256];
  ssize_t sz;

  if( dir != NULL ) {
    int   fd = dirfd( dir );

    while( ( de = readdir( dir ) ) != NULL ) {
      sprintf( fdpath, "/proc/self/fd/%s", de->d_name );
      if( ( sz = readlink( fdpath, fdname, 256 ) ) > 0 ) {
	fdname[sz] = '\0';
	if( atoi( de->d_name ) != fd ) {
	  fprintf( stderr, "%s %s -> %s", PIP_MTAG, fdpath, fdname );
	} else {
	  fprintf( stderr, "%s %s -> %s  opendir(\"/proc/self/fd\")",
		   PIP_MTAG, fdpath, fdname );
	}
      }
    }
    closedir( dir );
  }
}

#define LINSZ	(1024)
inline static void pip_check_addr( char*, void* ) __attribute__ ((unused));
inline static void pip_check_addr( char *tag, void *addr ) {
  FILE *maps = fopen( "/proc/self/maps", "r" );
  char *line = NULL;
  size_t sz  = LINSZ;
  int retval;

  if( tag == NULL ) tag = PIP_MTAG;
  while( maps != NULL ) {
    void *start, *end;

    if( ( retval = getline( &line, &sz, maps ) ) < 0 ) {
      fprintf( stderr, "getline()=%d\n", errno );
      break;
    } else if( retval == 0 ) {
      continue;
    }
    if( sscanf( line, "%p-%p", &start, &end ) == 2 ) {
      if( (intptr_t) addr >= (intptr_t) start &&
	  (intptr_t) addr <  (intptr_t) end ) {
	fprintf( stderr, ">>%s>> %p: %s", tag, addr, line );
	goto found;
      } else {
	//fprintf( stderr, ">>%s>> %p: %p--%p\n", tag, addr, start, end );
      }
    }
  }
  fprintf( stderr, ">>%s>> (not found)\n", tag );
 found:
  fclose( maps );
  if( line != NULL ) free( line );
  return;
}

void pip_print_loaded_solibs( FILE *file );

inline static double pip_gettime( void ) __attribute__ ((unused));
inline static double pip_gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

#endif	/* DOXYGEN_SHOULD_SKIP_THIS */
#endif	/* _pip_h_ */
