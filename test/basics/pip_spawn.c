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

#include <test.h>

int main( int argc, char **argv ) {
  int pipid, ntasks;

  /* before calling pip_init(), this must fail */
  pipid = PIP_PIPID_ANY;
  TESTIVAL( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		       NULL, NULL, NULL ),
	    EPERM,
	    return(EXIT_FAIL) );

  ntasks = NTASKS;
  TESTINT( pip_init( NULL, &ntasks, NULL, 0 ), return(EXIT_FAIL) );

  /* after calling pip_init() */
  if( pip_isa_task() ) {
    pipid = PIP_PIPID_ANY;
    TESTIVAL( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			 NULL, NULL, NULL ),
	      EPERM,
	      return(EXIT_FAIL) );
  } else {
    int coreno, status = 0, extval = 0;

    TESTIVAL( pip_spawn( NULL, NULL, NULL, PIP_CPUCORE_ASIS, &pipid,
			 NULL, NULL, NULL ),
	      EINVAL,
	      return(EXIT_FAIL) );

    coreno = -1;
    TESTIVAL( pip_spawn( argv[0], argv, NULL, coreno, &pipid,
			 NULL, NULL, NULL ),
	      EINVAL,
	      return(EXIT_FAIL) );

    coreno = 100000;
    TESTIVAL( pip_spawn( argv[0], argv, NULL, coreno, &pipid,
			 NULL, NULL, NULL ),
	      EINVAL,
	      return(EXIT_FAIL) );

    pipid = 100000;
    TESTIVAL( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			 NULL, NULL, NULL ),
	      EINVAL,
	      return(EXIT_FAIL) );

    pipid = -1;
    TESTIVAL( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			 NULL, NULL, NULL ),
	      EINVAL,
	      return(EXIT_FAIL) );

    pipid = PIP_PIPID_MYSELF;
    TESTIVAL( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			 NULL, NULL, NULL ),
	      EINVAL,
	      return(EXIT_FAIL) );

    pipid = PIP_PIPID_ANY;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			 NULL, NULL, NULL ),
	     return(EXIT_FAIL) );

    TESTIVAL( pip_wait( -123, &status ), EINVAL, return(EXIT_UNTESTED) );
    TESTINT(  pip_wait( pipid, &status ), return(EXIT_UNTESTED) );

    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
    } else {
      extval = EXIT_UNRESOLVED;
    }
    return extval;
  }
  TESTINT( pip_fin(), return(EXIT_FAIL) );
  return EXIT_PASS;
}
