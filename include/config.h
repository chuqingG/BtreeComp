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
#define MAX_SIZE_IN_BYTES 512 // 2048
#define DB2_PFX_MAX_SIZE 64   // 256
#define SPLIT_LIMIT 16        // suggest to be >= 32 for safety on db2
#define TAIL_SPLIT_WIDTH (1 / 6)
#define DEFAULT_DATASET_SIZE 100000
#define APPROX_KEY_SIZE 32

#define PKB_LEN 2
// Be careful when key length is variable,
// this number leave a little gap for safety by our split strategy
constexpr int kNumberBound = MAX_SIZE_IN_BYTES / APPROX_KEY_SIZE;