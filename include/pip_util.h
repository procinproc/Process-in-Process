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
  * Written by Atsushi HORI <ahori@riken.jp>
*/

#ifndef _pip_util_h_
#define _pip_util_h_

/************************************************************/
/* The following functions are just utilities for debugging */
/************************************************************/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <stdio.h>
#include <pip.h>

#ifdef PIP_EVAL
#define PIP_ACCUM(V,F)		\
  do { double __st=pip_gettime(); if(F) (V) += pip_gettime() - __st; } while(0)
#define PIP_REPORT(V)	 	printf( "%s: %g\n", #V, V );
#else
#define PIP_ACCUM(V,F)		if(F)
#define PIP_REPORT(V)
#endif

#define PIP_MALLOC(S)		malloc(S)
#define PIP_FREE(P)		do{ DBGF("FREE:%p",(P)); free(P); } while(0)

#ifdef __cplusplus
extern "C" {
#endif

  pid_t pip_gettid( void );
  int   pip_tgkill( int, int, int );
  int   pip_tkill( int, int );

  int  pip_check_pie( const char *path, int );

  void pip_info_fmesg( FILE *fp, const char *format, ... )
    __attribute__((format (printf, 2, 3)));
  void pip_info_mesg( const char *format, ... )
    __attribute__((format (printf, 1, 2)));
  void pip_err_mesg( const char *format, ... )
    __attribute__((format (printf, 1, 2)));
  void pip_warn_mesg( const char *format, ... )
    __attribute__((format (printf, 1, 2)));

  /* the following pip_pring_*() functions will be deprecated */
  void pip_print_maps( void );
  void pip_print_fd( int fd );
  void pip_print_fds( void );
  void pip_print_loaded_solibs( FILE *file );
  void pip_print_dsos( void );

  void pip_fprint_maps( FILE *fp );
  void pip_fprint_fd( FILE *fp, int fd );
  void pip_fprint_fds( FILE *fp );
  void pip_fprint_loaded_solibs( FILE *file );
  void pip_fprint_dsos( FILE *fp );

  void pip_check_addr( char *tag, void *addr );
  double pip_gettime( void );

  void pip_backtrace_fd( int depth, int fd );

#ifdef __cplusplus
}
#endif

#endif

#endif
