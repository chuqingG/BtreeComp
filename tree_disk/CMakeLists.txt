
add_library(btree_disk_std STATIC btree_disk_std.h btree_disk_std.cpp)
# add_library(btree_disk_db2 STATIC btree_db2.h btree_db2.cpp)
# add_library(btree_disk_myisam STATIC btree_myisam.h btree_myisam.cpp)
# add_library(btree_disk_wt STATIC btree_wt.h btree_wt.cpp)
# add_library(btree_disk_pkb STATIC btree_pkb.h btree_pkb.cpp)

add_library(btree_disk INTERFACE)

target_link_libraries(btree_disk INTERFACE btree_disk_std)
# target_link_libraries(btree_disk INTERFACE btree_disk_std btree_disk_db2 btree_disk_myisam btree_disk_wt btree_disk_pkb)