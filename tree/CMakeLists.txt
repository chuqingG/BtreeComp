
add_library(btree_std STATIC btree_std.h btree_std.cpp)
add_library(btree_db2 STATIC btree_db2.h btree_db2.cpp)
add_library(btree_myisam STATIC btree_myisam.h btree_myisam.cpp)
add_library(btree_wt STATIC btree_wt.h btree_wt.cpp)
add_library(btree_pkb STATIC btree_pkb.h btree_pkb.cpp)



# Define the assembly output directory
set(ASM_OUTPUT_DIR "${CMAKE_BINARY_DIR}/asm")

# Create the directory for assembly output
file(MAKE_DIRECTORY ${ASM_OUTPUT_DIR})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -S -fverbose-asm -save-temps")
get_filename_component(source_filename btree_std.cpp NAME_WE)
add_custom_command(
    OUTPUT ${ASM_OUTPUT_DIR}/${source_filename}.s
    COMMAND ${CMAKE_CXX_COMPILER} -S ${CMAKE_CURRENT_SOURCE_DIR}/btree_std.cpp ${CMAKE_CURRENT_SOURCE_DIR}/btree_std.h -o ${ASM_OUTPUT_DIR}/${source_filename}.s
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/btree_std.cpp
    COMMENT "Generating assembly for btree_std.cpp"
)
add_custom_target(btree_std_asm ALL DEPENDS ${ASM_OUTPUT_DIR}/${source_filename}.s)

# if(UNIX AND NOT APPLE)
#     add_custom_command(TARGET btree_std POST_BUILD
#                        COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:btree_std>)
# endif()

add_library(btree INTERFACE)

# target_link_libraries(btree INTERFACE btree_std btree_myisam btree_wt btree_pkb)
target_link_libraries(btree INTERFACE btree_std btree_db2 btree_myisam btree_wt btree_pkb)

