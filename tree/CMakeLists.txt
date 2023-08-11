
add_library(btree_std STATIC btree_std.h btree_std.cpp)
add_library(btree_db2 STATIC btree_db2.h btree_db2.cpp)
add_library(btree_myisam STATIC btree_myisam.h btree_myisam.cpp)
add_library(btree_wt STATIC btree_wt.h btree_wt.cpp)
add_library(btree_pkb STATIC btree_pkb.h btree_pkb.cpp)

add_library(btree INTERFACE)

target_link_libraries(btree INTERFACE btree_std btree_db2 btree_myisam btree_wt btree_pkb)