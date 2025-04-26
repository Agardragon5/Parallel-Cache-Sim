project(memory)
add_library(memory SHARED memory.c stree.c)
target_include_directories(memory PRIVATE ../common)
