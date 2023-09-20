#!/bin/bash

# ./Benchmark -b insert,search -d ../../../datasets/random/*32B* -i 3 -n 100000 -o auto >> out.txt
# ./Benchmark -b insert,search -d ../../../datasets/random/*32B* -i 3 -n 1000000 -o auto >> out.txt
# ./Benchmark -b insert,search -d ../../../datasets/random/*32B* -i 3 -n 10000000 -o auto >> out.txt
# ./Benchmark -b insert,search -d ../../../datasets/random/*32B* -i 1 -n 100000000 -o auto >> out.

./Benchmark -b insert,search -d ../../../datasets/random/*32B_100M* -i 1 -n 100000000 -o auto -l 8 >> out.txt
./Benchmark -b insert,search -d ../../../datasets/random/*32B_100M* -i 1 -n 100000000 -o auto -l 16 >> out.txt
# ./Benchmark -b insert,search -d ../../../datasets/random/*64B* -i 1 -n 100000000 -o auto >> out.txt
# ./Benchmark -b insert,search -d ../../../datasets/randomstr/only_upper_32* -i 1 -n 100000000 -o auto >> out.txt
# ./Benchmark -b insert,search -d ../../../datasets/randomstr/num_and_lower_32B* -i 2 -n 100000000 -o auto >> out.txt