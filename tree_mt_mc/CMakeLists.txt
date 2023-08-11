
add_library(btree_std_mt_mc STATIC btree_std_mt_mc.h btree_std_mt_mc.cpp)
add_library(btree_db2_mt_mc STATIC btree_db2_mt_mc.h btree_db2_mt_mc.cpp)
add_library(btree_myisam_mt_mc STATIC btree_myisam_mt_mc.h btree_myisam_mt_mc.cpp)
add_library(btree_wt_mt_mc STATIC btree_wt_mt_mc.h btree_wt_mt_mc.cpp)
add_library(btree_pkb_mt_mc STATIC btree_pkb_mt_mc.h btree_pkb_mt_mc.cpp)

add_library(btree_mt_mc INTERFACE)

target_link_libraries(btree_mt_mc INTERFACE btree_std_mt_mc btree_db2_mt_mc btree_myisam_mt_mc btree_wt_mt_mc btree_pkb_mt_mc)