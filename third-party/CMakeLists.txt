IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/${Z3LibraryName})
    ADD_CUSTOM_COMMAND(
        OUTPUT z3build
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/${Z3LibraryName} ${PROJECT_BINARY_DIR}/lib/${Z3LibraryName}
    )
else()
 #   include(ExternalProject)
 #   ExternalProject_Add( z3_project
 #           SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/z3
 #           BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/
 #           CONFIGURE_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/z3 COMMAND python2.7 scripts/mk_make.py --staticlib
 #           BUILD_COMMAND "${MAKE} -C ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/"
 #           UPDATE_COMMAND ""
 #           INSTALL_COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/${z3library} ${PROJECT_BINARY_DIR}/lib/${z3library}
 #           LOG_UPDATE 1
 #           LOG_CONFIGURE 1
 #           LOG_BUILD 1
 #           LOG_TEST 1
 #           LOG_INSTALL 1
 #           USES_TERMINAL_CONFIGURE 1
 #   )
 #   ADD_CUSTOM_COMMAND(
 #           OUTPUT z3build
 #           COMMAND "cp ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/${z3library} ${PROJECT_BINARY_DIR}/lib/${z3library}"
 #   )
    ADD_CUSTOM_COMMAND(
        OUTPUT z3build
        COMMAND python2.7 scripts/mk_make.py --staticlib WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/z3
        COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/  VERBATIM USES_TERMINAL
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/${z3library} ${PROJECT_BINARY_DIR}/lib/${z3library}
    )
endif()

add_custom_target(z3 ALL DEPENDS z3build)