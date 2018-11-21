subdirs(rd_test)
subdirs(ecs_test)

# Give full access to headers to host applications
include_directories(${wcore_SOURCE_DIR}/source/include)
