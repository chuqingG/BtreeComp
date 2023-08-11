#!/bin/bash

# ./Benchmark -b insert,search -d ../../../datasets/tpch/lineitem_101112.txt -i 1 -o auto >> ../screenout/line10to12.txt
# ./Benchmark -b insert,search -d ../../../datasets/tpch/*50.txt -i 1 -o auto >> ../screenout/tpchpartname.txt
./Benchmark -b insert,search -d ../../../datasets/webspam/*25M.txt -i 1 -o auto >> ../screenout/webspam.txt
# ./Benchmark -b insert,search -d ../../../datasets/entropy/*12*36_1M.txt -i 1 -o auto >> ../screenout/entropy12_36_1M.txt
