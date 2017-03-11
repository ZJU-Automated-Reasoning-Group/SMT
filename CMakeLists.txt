<<<<<<< 115ac3e720a59bf0f5aa0057d0a5fa3ff93cb25f
add_custom_command(
    OUTPUT tmp
    COMMAND  make
)
=======
#add_custom_command(
#   OUTPUT tmp
#   COMMAND  make
#)
#message("submodule shows test variable:" ${test_variable})
add_subdirectory(lib)
add_subdirectory(third-party)
add_subdirectory(tools)
>>>>>>> modify cmake building system.
