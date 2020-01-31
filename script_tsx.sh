#!/bin/bash
rm report.txt


g++ -std=c++11 -O -mrtm tsxtest.cpp -lpthread

for i in 1 2 3 4 5 6 7 8 9 10 20 30 40 50 100
do
./a.out $i >> fast_freq_report.txt
done

exit 0
