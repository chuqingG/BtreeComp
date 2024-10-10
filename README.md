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

## Result Analysis

In addition to the optional output file (-o), we also provide a summary on the screen output to get a quick glance of the results of each run, an example output is shown as follows.

```bash
Finish [insert,search]: size=1000000, iter=3, 0.9 minutes.
=================================================================================================================
                                        PERFORMANCE BENCHMARK RESULTS - insert
=================================================================================================================
Index Structure |      Min      |      Max      |      Avg      |      Med      | M Ops/s (Avg) | M Ops/s (Med)|
-----------------------------------------------------------------------------------------------------------------
Btree-Std       |      0.8383s  |      0.8552s  |      0.8443s  |      0.8394s  |      1.1845   |      1.1845  |
Btree-Head      |      0.8240s  |      0.8262s  |      0.8252s  |      0.8253s  |      1.2119   |      1.2119  |
Btree-Tail      |      0.8302s  |      0.8434s  |      0.8346s  |      0.8303s  |      1.1982   |      1.1982  |
Btree-He+Tail   |      0.7242s  |      0.7649s  |      0.7379s  |      0.7247s  |      1.3551   |      1.3551  |
Btree-WT        |      2.7890s  |      2.8618s  |      2.8137s  |      2.7902s  |      0.3554   |      0.3554  |
Btree-My        |      1.1489s  |      1.1836s  |      1.1649s  |      1.1623s  |      0.8584   |      0.8584  |
Btree-PkB       |      1.2770s  |      1.2951s  |      1.2830s  |      1.2771s  |      0.7794   |      0.7794  |
Btree-DB2       |      1.0329s  |      1.0456s  |      1.0376s  |      1.0341s  |      0.9638   |      0.9638  |
=================================================================================================================
                                        PERFORMANCE BENCHMARK RESULTS - search
=================================================================================================================
Index Structure |      Min      |      Max      |      Avg      |      Med      | M Ops/s (Avg) | M Ops/s (Med)|
-----------------------------------------------------------------------------------------------------------------
Btree-Std       |      0.7633s  |      0.7675s  |      0.7652s  |      0.7647s  |      1.3069   |      1.3069  |
Btree-Head      |      0.7283s  |      0.7454s  |      0.7356s  |      0.7330s  |      1.3595   |      1.3595  |
Btree-Tail      |      0.6832s  |      0.6905s  |      0.6878s  |      0.6897s  |      1.4539   |      1.4539  |
Btree-He+Tail   |      0.6241s  |      0.6264s  |      0.6252s  |      0.6251s  |      1.5994   |      1.5994  |
Btree-WT        |      2.4631s  |      2.4943s  |      2.4750s  |      2.4677s  |      0.4040   |      0.4040  |
Btree-My        |      0.7180s  |      0.7353s  |      0.7239s  |      0.7183s  |      1.3815   |      1.3815  |
Btree-PkB       |      0.0050s  |      0.0051s  |      0.0051s  |      0.0051s  |    197.2763   |    197.2763  |
Btree-DB2       |      0.8444s  |      0.8493s  |      0.8474s  |      0.8485s  |      1.1801   |      1.1801  |
=================================================================================================================
                                        TREE STATISTICS BENCHMARK RESULTS
=================================================================================================================
Index Structure |      Height   | Avg Key Size  |  Prefix Size  |  Avg Fanout   | Total Nodes   | Non-leaf #   |
-----------------------------------------------------------------------------------------------------------------
Btree-Std       |      6.0000   |     20.0000   |      0.0000   |     14.9937   |  76379.0000   |   5094.0000  |
Btree-Head      |      5.0000   |     16.2299   |      0.3537   |     16.6031   |  61283.0000   |   3691.0000  |
Btree-Tail      |      5.0000   |     19.2780   |      0.0000   |     30.9104   |  74897.0000   |   2423.0000  |
Btree-He+Tail   |      4.0000   |     15.3739   |      0.3616   |     32.7147   |  59509.0000   |   1819.0000  |
Btree-WT        |      5.0000   |     16.5533   |      2.0000   |     31.2422   |  65922.0000   |   2110.0000  |
Btree-My        |      5.0000   |     16.8257   |      2.0000   |     15.3043   |  67600.0000   |   4417.0000  |
Btree-PkB       |      6.0000   |     26.0000   |      5.8370   |     12.0491   |  96695.0000   |   8025.0000  |
Btree-DB2       |      5.0000   |     17.1164   |      1.5145   |     18.2874   |  57259.0000   |   3131.0000  |
```