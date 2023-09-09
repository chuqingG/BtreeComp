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
// #define WTCACHE
// #define DUPKEY
// #define TOFIX  // enable to fix some skipped problem

// Setup
#define SPLIT_STRATEGY_SPACE
#define MAX_SIZE_IN_BYTES 160 // 2048
#define SPLIT_LIMIT 16        // can set to 0 when test fixed-length data
#define DEFAULT_DATASET_SIZE 100000
#define APPROX_KEY_SIZE 16
#define MAX_TAIL_COMP_RATIO 16
#define MAX_HEAD_COMP_RATIO 2
#define DB2_COMP_RATIO 2
#define TAIL_COMP_RELAX (MAX_TAIL_COMP_RATIO - 1)
#define HEAD_COMP_RELAX (MAX_HEAD_COMP_RATIO - 1)
// Be careful when key length is variable,
// this number leave a little gap for safety by our split strategy
constexpr int kNumberBound = MAX_SIZE_IN_BYTES / APPROX_KEY_SIZE;