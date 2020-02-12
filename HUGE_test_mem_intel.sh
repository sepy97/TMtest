#!/bin/bash

#g++ -std=c++11 -O -mrtm tsxtest.cpp -o mem.out -lpthread

for i in 1 2 4 8 16 32 64 128 256 512 1024 2048
do
LD_PRELOAD=libhugetlbfs.so HUGETLB_MORECORE=yes ./mem.out $i >> HUGE_mem_report.txt
done

exit 0

