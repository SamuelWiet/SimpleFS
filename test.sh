#!/bin/bash
set -e

P=sfs
# include sfs_util.c (contains FAT/Directory/File helper functions required at link time)
C='sfs_test.c disk_emu.c sfs_api.c sfs_util.c'
# enable -g to allow gdb debugging, keep optimizations off for tests
FLAGS='-O2 -Wall -g'

echo Compiling...
gcc $FLAGS $C -o $P

if [ "${1}" = "debug" ]
then
	echo Debugging...
	gdb ./$P
else
	echo ${1}; echo Running...
	./$P
	# remove the disk only after a normal run
	rm -f disk.sfs
fi
