#!/bin/sh
#PJM -L rscgrp=regular-cache
#PJM -L node=1
#PJM -L elapse=4:00:00
#PJM -g gg10
#PJM -j

## LD_LIBRARY_PATH=/work/0/gg10/g10013/work/PIP/import/mpich-install/lib/:/work/0/gg10/g10013/work/PIP/import/install/lib:$LD_LIBRARY_PATH /work/0/gg10/g10013/work/PIP/import/mpich-install/bin/mpiexec -n 16 ./stencil_pip 4096 1 1000

. ./eval.sh.inc

PROGS="stencil_piponly"
SIZE=4096
#SIZE=8192
#SIZE=16384
ENERGY=1
NITERS=1000

NCPUS=`grep CPU /proc/cpuinfo | wc -l`
MODES="process"
PIPRUN=`pwd`/../../bin/piprun

##ITER_NUM="1 2"

doeval() {
    if [ $1 == stencil_piponly ]
    then
	export PIPMODE=process;
	export LD_LIBRARY_PATH=$PIPMPILIB:$PIPLIB:$LIBS;
#	echo $MPIRUN_PIP;
#	echo $LD_LIBRARY_PATH;
	export LD_PRELOAD=`pwd`/../../preload/pip_preload.so
	for ITER in $ITER_NUM
	do
	    echo -n "["$ITER"]" $1:$PIPMODE $2 " ";
	    $PIPRUN -n $2 ./$1 $SIZE $ENERGY $NITERS;
	done
    else
	unset PIP_MODE;
	unset LD_PRELOAD;
	export LD_LIBRARY_PATH=$MPIMPILIB:$LIBS
#	echo $MPIRUN_MPI;
#	echo $LD_LIBRARY_PATH;
	for ITER in $ITER_NUM
	do
	    echo -n "["$ITER"]" $1 $2 " "
	    $MPIRUN_MPI -np $2 ./$1 $SIZE $ENERGY $NITERS;
	done
    fi
    echo
}

csv_begin

for NTASKS in 2 4 8 16 32 64 128 256
do
    if [ $NTASKS -gt $NCPUS ]
    then
	 break
    fi
    if [ $NTASKS -gt $CORENUM ]
    then
	break
    fi
    for PROG in ${PROGS}
    do
	doeval ${PROG} ${NTASKS}
    done
done

csv_end
