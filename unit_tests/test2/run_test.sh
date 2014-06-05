#/bin/bash

#export AUT_DUMP_MM=TRUE
rm -rf AUT*.dump
# Using LLNL job handler (srun)
LD_PRELOAD=../../../install/lib/libstracker.so srun -n 16 -ppdebug ./test
d=`diff A*.dump gold.dump`
if [$d == '']
then 
	echo "PASSED"
else
	echo "FAILED"
fi
