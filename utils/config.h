#pragma once

// Setup
#define SPLIT_STRATEGY_SPACE
#define MAX_SIZE_IN_BYTES 1024
#define DB2_PFX_MAX_SIZE 256
#define SPLIT_LIMIT 32 // suggest to be >= 32 for safety on db2
#define TAIL_SPLIT_WIDTH (1.0 / 6)
#define RANGE_SCOPE (1.0 / 2)
#define DEFAULT_DATASET_SIZE 100000
#define APPROX_KEY_SIZE 32
#define WT_CACHE_KEY_NUM 1000

#define PKB_LEN 2
