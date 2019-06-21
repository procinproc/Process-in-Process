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

#define DEBUG

#include <libgen.h>
#include <test.h>

int main( int argc, char **argv ) {
  char *dir;
  char *nargv[2] = { NULL, NULL };
  int status, extval;

  dir = dirname( argv[0] );
  chdir( dir );

  TESTINT( pip_init( NULL, NULL, NULL, 0 ), return(EXIT_FAIL) );

  nargv[0] = "./prog-nopie";
  TESTIVAL( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		      NULL, NULL, NULL ),
	    ELIBEXEC,
	    return(EXIT_FAIL) );

  nargv[0] = "./prog-nordynamic";
  TESTIVAL( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		      NULL, NULL, NULL ),
	    ENOEXEC,
	    return(EXIT_FAIL) );

  nargv[0] = "prog-pie";	/* not a path (no slash) */
  TESTIVAL( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		       NULL, NULL, NULL ),
	    EINVAL,
	    return(EXIT_FAIL) );

  nargv[0] = "./prog-pie";	/* correct one */
  TESTINT( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		       NULL, NULL, NULL ),
	   return(EXIT_FAIL) );

  TESTINT(  pip_wait_any( NULL, &status ), return(EXIT_UNTESTED) );

  if( WIFEXITED( status ) ) {
    if( ( extval = WEXITSTATUS( status ) ) != 0 ) {
      return EXIT_FAIL;
    }
  } else {
    extval = EXIT_UNRESOLVED;
  }

  TESTINT( pip_fin(), return(EXIT_FAIL) );
  return EXIT_PASS;
}
