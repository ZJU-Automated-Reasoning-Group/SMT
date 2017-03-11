#add_custom_command(
#   OUTPUT tmp
#   COMMAND  make
#)
#message("submodule shows test variable:" ${test_variable})
add_subdirectory(lib)
add_subdirectory(third-party)
add_subdirectory(tools)