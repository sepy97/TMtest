#!/bin/bash
rm report.txt


#g++ -std=c++11 -O -mrtm tsxtest.cpp -lpthread

for i in 1 2 3 4 5 6 7 8 9 10 20 30 40 50 100
do
./freq.out $i >> freq_report.txt
done

for i in 1 2 4 8 16 32 64 128 256 512 1024 2048
do
./mem.out $i >> mem_report.txt
done

exit 0
