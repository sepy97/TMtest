#!/bin/bash

#g++ -std=c++11 -O -mrtm tsxtest.cpp -o freq.out -lpthread

for i in 1 2 3 4 5 6 7 8 9 10 20 30 40 50 100
do
LD_PRELOAD=libhugetlbfs.so HUGETLB_MORECORE=yes ./freq.out $i >> HUGE_freq_report.txt
done

exit 0
