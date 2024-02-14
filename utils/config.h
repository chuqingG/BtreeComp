#pragma once

// Compilation options
#define THREAD_DEBUG
// #define MULTICOL_COMPRESSION
// #define MYDEBUG
// #define MULTICOL_LHKEY_SAVE_SPACE //disable means store full lhkey
#define MEMDEBUG
#define CHARALL
#define SINGLE_DEBUG
#define VERBOSE_PRINT // enable to write some intermediate results to output file (if set -o)
// #define WT_OPTIM   // enable for prefixstart
// #define WTCACHE
// #define DUPKEY
// #define TOFIX  // enable to fix some skipped problem

// Setup
#define SPLIT_STRATEGY_SPACE
#define MAX_SIZE_IN_BYTES 1024 // 512 // 8192 // 1024 //(4096 + 512) // 512 // 2048
#define DB2_PFX_MAX_SIZE 256   // 128  //(1024 + 512)  // 128  // 256
#define SPLIT_LIMIT 32         // suggest to be >= 32 for safety on db2
#define TAIL_SPLIT_WIDTH (1.0 / 5)
#define RANGE_SCOPE (1.0 / 1000000)
#define DEFAULT_DATASET_SIZE 100
#define APPROX_KEY_SIZE 32
#define WT_CACHE_KEY_NUM 1000

#define PKB_LEN 5
// #define TRACK_DISTANCE // distance to decompress for delta compression
