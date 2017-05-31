/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

/** \addtogroup pmap pmap
 *
 * \brief command to print PiP memory map
 *
 * \section synopsis SYNOPSIS
 *
 *	\b pmap
 *
 *
 * \section description DESCRIPTION
 *
 * \b This command prints memory map of current task
 *
 * \section environment ENVIRONMENT
 *
 */

#include <pip.h>

int main() {
  pip_print_maps();
  return 0;
}
