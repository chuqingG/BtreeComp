# B-tree compression benchmark


## Usage
```bash
# compile and build
mkdir build && cd build 
cmake -DCMAKE_BUILD_TYPE=Release .. && make 

# run the benchmark

./Benchmark -b [insert/insert,search/insert,search,range] -n [key_num] -i [iter_num] -d [dataset_name] -t [thread_num] -o [path_to_results_file] -r [range_query_num] -l [max_key_length]
# multiple benchmarks can be run at once
# -o can be omitted, or you can try -o auto
# e.g. ./Benchmark -b insert,search -n 100000 -i 5
# e.g. ./Benchmark -b insert,search,range -d ../datasets/test_dataset.txt -i 5
# e.g. ./Benchmark -b insert,search,range,backward -d ../datasets/test_dataset.txt -i 5
# e.g. ./Benchmark -b insert,search,range -d ../datasets/test_dataset.txt -i 5 -o ../results
```


## TODO
1. WT - check if locking can be avoided in search
2. Enforce checks to avoid benchmark without insert
