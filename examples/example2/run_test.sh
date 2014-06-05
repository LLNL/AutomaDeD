#/bin/bash


# Using LLNL job handler (srun)
LD_PRELOAD=../../../install/lib/libstracker.so srun -n 16 -ppdebug ./test
