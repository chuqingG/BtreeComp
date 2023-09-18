#!/bin/bash

./Benchmark -b insert,search -d ../../../datasets/random/*32B* -i 3 -n 100000 -o auto >> out.txt
./Benchmark -b insert,search -d ../../../datasets/random/*32B* -i 3 -n 1000000 -o auto >> out.txt
./Benchmark -b insert,search -d ../../../datasets/random/*32B* -i 3 -n 10000000 -o auto >> out.txt
./Benchmark -b insert,search -d ../../../datasets/random/*32B* -i 1 -n 100000000 -o auto >> out.txt