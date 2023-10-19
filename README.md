# B-tree Compression Benchmark

We implemented a benchmark of different compression techniques of B-trees from scratch.

## Code Structure

The rough code structure is displayed below, omitting some auxiliary files (e.g. cmake files).
```bash
├── benchmark                      # Folder for benchmark codes
├── compression                    # Folder for compression handling codes
│   ├── compression_[method].cpp   
│   ├── compression_[method].h
├── README.md
├── tree                           # Folder for trees' data structures 
│   ├── btree_[method].cpp         # Definitions of the tree using [method]
│   ├── btree_[method].h           # Declarations of the tree using [method]
│   ├── node.cpp                   # Definitions of different types of nodes
│   ├── node.h                     # Nodes' declarations and definitions of headers
│   └── node_inline.h              # Macros and inline functions for accessing and updating metadata 
├── tree_disk                      # Folder for disk-based trees, similar as tree
│   ├── btree_disk_[method].cpp
│   ├── btree_disk_[method].h
│   ├── dsk_manager.cpp            # Disk management
│   ├── node_disk.cpp
│   ├── node_disk.h
│   └── node_disk_inline.h              
└── utils                          # Folder for utility functions
    ├── compare.cpp                # Utility for comparisons
    ├── config.h                   # Configurations
    ├── item.hpp                   # Definition of the data unit
    └── util.h                     # Utility for data processing and generate summary      
```

## Usage
```bash
# compile and build
mkdir build && cd build 
cmake -DCMAKE_BUILD_TYPE=Release .. && make 

# run the benchmark

./Benchmark -b [insert/insert,search/insert,search,range] \  # type of benchmark
            -n [key_num] \              # number of keys, enable to truncate the dataset 
            -i [iter_num] \             # number of iterations, default = 5
            -d [dataset_name] \         # path of the dataset 
            -o [path_to_results_file] \ # path to a summary of the results (optional)
            -r [range_query_num] \      # number of the range query
            -l [max_key_length]         # max key length, enable to truncate each key
```
