
add_library(btree_std_mt STATIC btree_std_mt.h btree_std_mt.cpp)
# add_library(btree_db2_mt STATIC btree_db2_mt.h btree_db2_mt.cpp)
# add_library(btree_myisam_mt STATIC btree_myisam_mt.h btree_myisam_mt.cpp)
# add_library(btree_wt_mt STATIC btree_wt_mt.h btree_wt_mt.cpp)
# add_library(btree_pkb_mt STATIC btree_pkb_mt.h btree_pkb_mt.cpp)

add_library(btree_mt INTERFACE)

# target_link_libraries(btree_mt INTERFACE btree_std_mt btree_db2_mt btree_myisam_mt btree_wt_mt btree_pkb_mt)
target_link_libraries(btree_mt INTERFACE btree_std_mt)