#pragma once

// Compilation options
#define THREAD_DEBUG
// #define MULTICOL_COMPRESSION
// #define MYDEBUG
// #define MULTICOL_LHKEY_SAVE_SPACE //disable means store full lhkey
#define MEMDEBUG
#define CHARALL
#define SINGLE_DEBUG
// #define TIME_DEBUG
// #define DUPKEY
// #define TOFIX  // enable to fix some skipped problem



//Setup
#define SPLIT_STRATEGY_SPACE
#define MAX_SIZE_IN_BYTES 160 // 2048
#define SPLIT_LIMIT 16
#define DEFAULT_DATASET_SIZE 100000
#define APPROX_KEY_SIZE 32

// Be careful when key length is variable, 
// this number leave a little gap for safety by our split strategy
constexpr int kNumberBound = MAX_SIZE_IN_BYTES / APPROX_KEY_SIZE; 