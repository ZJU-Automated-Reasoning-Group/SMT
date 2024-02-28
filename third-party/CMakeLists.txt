MESSAGE(STATUS "Z3 library: ${CMAKE_CURRENT_SOURCE_DIR}/z3/build/${Z3LibraryName}")
# TODO workaround for the duplicate target name
cmake_policy(SET CMP0048 NEW)
project(Z3 VERSION 4.12.5.0 LANGUAGES CXX)
set(LLVM_ENABLE_RTTI ON)
set(Z3_USE_LIB_GMP OFF)
add_definitions(-D_DEBUG=0 -UZ3DEBUG -D_MP_INTERNAL)
################################################################################
# Project version
################################################################################
set(Z3_FULL_VERSION_STR "${Z3_VERSION}") # Note this might be modified
message(STATUS "Z3 version ${Z3_VERSION}")
include(${CMAKE_CURRENT_SOURCE_DIR}/z3/cmake/z3_add_component.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/z3/cmake/z3_append_linker_flag_list_to_target.cmake)
SET(PROJECT_SOURCE_DIR_TMP ${PROJECT_SOURCE_DIR})
SET(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/z3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(Python3_EXECUTABLE /usr/bin/python3)
find_program(Python3_EXECUTABLE NAMES python3)
set(Z3_FULL_VERSION "\"4.12\"")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/z3/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/z3/src)

################################################################################
# API header files
################################################################################
# This lists the API header files that are scanned by
# some of the build rules to generate some files needed
# by the build
set(Z3_API_HEADER_FILES_TO_SCAN
        z3_api.h
        z3_ast_containers.h
        z3_algebraic.h
        z3_polynomial.h
        z3_rcf.h
        z3_fixedpoint.h
        z3_optimization.h
        z3_fpa.h
        z3_spacer.h
)
set(Z3_FULL_PATH_API_HEADER_FILES_TO_SCAN "")
foreach (header_file ${Z3_API_HEADER_FILES_TO_SCAN})
    set(full_path_api_header_file "${CMAKE_CURRENT_SOURCE_DIR}/z3/src/api/${header_file}")
    list(APPEND Z3_FULL_PATH_API_HEADER_FILES_TO_SCAN "${full_path_api_header_file}")
    if (NOT EXISTS "${full_path_api_header_file}")
        message(FATAL_ERROR "API header file \"${full_path_api_header_file}\" does not exist")
    endif()
endforeach()

################################################################################
# Traverse directories each adding a Z3 component
################################################################################
# I'm duplicating the order in ``mk_project.py`` for now to help us keep
# the build systems in sync.
#
# The components in these directory explicitly declare their dependencies so
# you may be able to re-order some of these directories but an error will be
# raised if you try to declare a component is dependent on another component
# that has not yet been declared.
add_subdirectory(z3/src/util)
add_subdirectory(z3/src/math/polynomial)
add_subdirectory(z3/src/math/dd)
add_subdirectory(z3/src/math/hilbert)
add_subdirectory(z3/src/math/simplex)
add_subdirectory(z3/src/math/automata)
add_subdirectory(z3/src/math/interval)
add_subdirectory(z3/src/math/realclosure)
add_subdirectory(z3/src/math/subpaving)
add_subdirectory(z3/src/ast)
add_subdirectory(z3/src/params)
add_subdirectory(z3/src/ast/rewriter)
add_subdirectory(z3/src/ast/rewriter/bit_blaster)
add_subdirectory(z3/src/ast/normal_forms)
add_subdirectory(z3/src/ast/macros)
add_subdirectory(z3/src/model)
add_subdirectory(z3/src/ast/euf)
add_subdirectory(z3/src/ast/converters)
add_subdirectory(z3/src/ast/substitution)
add_subdirectory(z3/src/ast/simplifiers)
add_subdirectory(z3/src/tactic)
add_subdirectory(z3/src/qe/mbp)
add_subdirectory(z3/src/qe/lite)
add_subdirectory(z3/src/smt/params)
add_subdirectory(z3/src/parsers/util)
add_subdirectory(z3/src/math/grobner)
add_subdirectory(z3/src/sat)
add_subdirectory(z3/src/nlsat)
add_subdirectory(z3/src/tactic/core)
add_subdirectory(z3/src/math/subpaving/tactic)
add_subdirectory(z3/src/tactic/aig)
add_subdirectory(z3/src/tactic/arith)
add_subdirectory(z3/src/solver)
add_subdirectory(z3/src/cmd_context)
add_subdirectory(z3/src/cmd_context/extra_cmds)
add_subdirectory(z3/src/parsers/smt2)
add_subdirectory(z3/src/solver/assertions)
add_subdirectory(z3/src/ast/pattern)
add_subdirectory(z3/src/math/lp)
add_subdirectory(z3/src/sat/smt)
add_subdirectory(z3/src/sat/tactic)
add_subdirectory(z3/src/nlsat/tactic)
add_subdirectory(z3/src/ackermannization)
add_subdirectory(z3/src/ast/proofs)
add_subdirectory(z3/src/ast/fpa)
add_subdirectory(z3/src/smt/proto_model)
add_subdirectory(z3/src/smt)
add_subdirectory(z3/src/tactic/bv)
add_subdirectory(z3/src/smt/tactic)
add_subdirectory(z3/src/tactic/sls)
add_subdirectory(z3/src/qe)
add_subdirectory(z3/src/muz/base)
add_subdirectory(z3/src/muz/dataflow)
add_subdirectory(z3/src/muz/transforms)
add_subdirectory(z3/src/muz/rel)
add_subdirectory(z3/src/muz/clp)
add_subdirectory(z3/src/muz/tab)
add_subdirectory(z3/src/muz/bmc)
add_subdirectory(z3/src/muz/ddnf)
add_subdirectory(z3/src/muz/spacer)
add_subdirectory(z3/src/muz/fp)
add_subdirectory(z3/src/tactic/ufbv)
add_subdirectory(z3/src/sat/sat_solver)
add_subdirectory(z3/src/tactic/smtlogics)
add_subdirectory(z3/src/tactic/fpa)
add_subdirectory(z3/src/tactic/fd_solver)
add_subdirectory(z3/src/tactic/portfolio)
#add_subdirectory(z3/src/opt)

file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/z3/src/opt/opt_params.hpp")
z3_add_component(opt1
        SOURCES
        z3/src/opt/maxcore.cpp
        z3/src/opt/maxlex.cpp
        z3/src/opt/maxsmt.cpp
        z3/src/opt/opt_cmds.cpp
        z3/src/opt/opt_context.cpp
        z3/src/opt/opt_cores.cpp
        z3/src/opt/opt_lns.cpp
        z3/src/opt/opt_pareto.cpp
        z3/src/opt/opt_parse.cpp
        z3/src/opt/opt_preprocess.cpp
        z3/src/opt/optsmt.cpp
        z3/src/opt/opt_solver.cpp
        z3/src/opt/pb_sls.cpp
        z3/src/opt/sortmax.cpp
        z3/src/opt/totalizer.cpp
        z3/src/opt/wmax.cpp
        COMPONENT_DEPENDENCIES
        sat_solver
        sls_tactic
        smt
        smtlogic_tactics
        PYG_FILES
        z3/src/opt/opt_params.pyg
)

add_custom_target(gen_opt_params
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/pyg2hpp.py" "${_full_pyg_file_path}" "${CMAKE_CURRENT_SOURCE_DIR}/z3/src/opt"
        DEPENDS "${PROJECT_SOURCE_DIR}/scripts/pyg2hpp.py"
        ${Z3_GENERATED_FILE_EXTRA_DEPENDENCIES}
        COMMENT "Generating \"${_full_output_file_path}\" from \"${pyg_file}\""
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        USES_TERMINAL
        VERBATIM
)

add_subdirectory(z3/src/api)
add_subdirectory(z3/src/api/dll)
################################################################################
# libz3
################################################################################
get_property(Z3_LIBZ3_COMPONENTS_LIST GLOBAL PROPERTY Z3_LIBZ3_COMPONENTS)
set (object_files "")
foreach (component ${Z3_LIBZ3_COMPONENTS_LIST})
    list(APPEND object_files $<TARGET_OBJECTS:${component}>)
endforeach()
if (Z3_BUILD_LIBZ3_SHARED)
    set(lib_type "SHARED")
else()
    set(lib_type "STATIC")
endif()
# Enable static msvc runtime.
if (MSVC AND Z3_BUILD_LIBZ3_MSVC_STATIC)
    set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELWITHDEBINFO
    )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        string(REPLACE "/MDd" "/MTd" ${CompilerFlag} "${${CompilerFlag}}")
        set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
        message("MSVC flags: ${CompilerFlag}:${${CompilerFlag}}")
    endforeach()
endif()
add_library(libz3 ${lib_type} ${object_files})
add_dependencies(libz3 gen_opt_params)
target_include_directories(libz3 INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/z3/src/api>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
#set_target_properties(libz3 PROPERTIES
#        # VERSION determines the version in the filename of the shared library.
#        # SOVERSION determines the value of the DT_SONAME field on ELF platforms.
#        # On ELF platforms the final compiled filename will be libz3.so.W.X.Y.Z
#        # but symlinks will be made to this file from libz3.so and also from
#        # libz3.so.W.X.
#        # This indicates that no breaking API changes will be made within a single
#        # minor version.
#        VERSION ${Z3_VERSION}
#        SOVERSION ${Z3_VERSION_MAJOR}.${Z3_VERSION_MINOR})

if (NOT MSVC)
    # On UNIX like platforms if we don't change the OUTPUT_NAME
    # the library gets a name like ``liblibz3.so`` so we change it
    # here. We don't do a rename with MSVC because we get file naming
    # conflicts (the z3 executable also has this OUTPUT_NAME) with
    # ``.ilk``, ``.pdb``, ``.lib`` and ``.exp`` files sharing the same
    # prefix.
    set_target_properties(libz3 PROPERTIES OUTPUT_NAME z3)
endif()

# The `PRIVATE` usage requirement is specified so that when building Z3 as a
# shared library the dependent libraries are specified on the link command line
# so that if those are also shared libraries they are referenced by `libz3.so`.
target_link_libraries(libz3 PRIVATE ${Z3_DEPENDENT_LIBS})

# This is currently only for the OpenMP flags. It needs to be set
# via `target_link_libraries()` rather than `z3_append_linker_flag_list_to_target()`
# because when building the `libz3` as a static library when the target is exported
# the link dependencies need to be exported too.
foreach (flag_name ${Z3_DEPENDENT_EXTRA_CXX_LINK_FLAGS})
    target_link_libraries(libz3 PRIVATE ${flag_name})
endforeach()

# Declare which header file are the public header files of libz3
# these will automatically installed when the libz3 target is installed
set (libz3_public_headers
        z3_algebraic.h
        z3_api.h
        z3_ast_containers.h
        z3_fixedpoint.h
        z3_fpa.h
        z3.h
        c++/z3++.h
        z3_macros.h
        z3_optimization.h
        z3_polynomial.h
        z3_rcf.h
        z3_v1.h
        z3_spacer.h
)
foreach (header ${libz3_public_headers})
    set_property(TARGET libz3 APPEND PROPERTY
            PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/src/api/${header}")
endforeach()
set_property(TARGET libz3 APPEND PROPERTY
        PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/z3/src/util/z3_version.h")

install(TARGETS libz3
        EXPORT Z3_EXPORTED_TARGETS
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" # On Windows this installs ``libz3.lib`` which CMake calls the "corresponding import library". Do we want this installed?
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" # For Windows. DLLs are runtime targets for CMake
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

if (MSVC)
    # Handle settings dll exports when using MSVC
    # FIXME: This seems unnecessarily complicated but I'm doing
    # this because this is what the python build system does.
    # CMake has a much more elegant (see ``GenerateExportHeader.cmake``)
    # way of handling this.
    set(dll_module_exports_file "${CMAKE_CURRENT_BINARY_DIR}/api_dll.def")
    add_custom_command(OUTPUT "${dll_module_exports_file}"
            COMMAND
            "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/scripts/mk_def_file.py"
            "${dll_module_exports_file}"
            "libz3"
            ${Z3_FULL_PATH_API_HEADER_FILES_TO_SCAN}
            DEPENDS
            "${PROJECT_SOURCE_DIR}/scripts/mk_def_file.py"
            ${Z3_GENERATED_FILE_EXTRA_DEPENDENCIES}
            ${Z3_FULL_PATH_API_HEADER_FILES_TO_SCAN}
            COMMENT "Generating \"${dll_module_exports_file}\""
            USES_TERMINAL
            VERBATIM
    )
    add_custom_target(libz3_extra_depends
            DEPENDS "${dll_module_exports_file}"
    )
    add_dependencies(libz3 libz3_extra_depends)
    z3_append_linker_flag_list_to_target(libz3 "/DEF:${dll_module_exports_file}")
endif()

################################################################################
# Z3 API bindings
################################################################################
option(Z3_BUILD_PYTHON_BINDINGS "Build Python bindings for Z3" OFF)
if (Z3_BUILD_PYTHON_BINDINGS)
    if (NOT Z3_BUILD_LIBZ3_SHARED)
        message(FATAL_ERROR "The python bindings will not work with a static libz3. "
                "You either need to disable Z3_BUILD_PYTHON_BINDINGS or enable Z3_BUILD_LIBZ3_SHARED")
    endif()
    add_subdirectory(z3/src/api/python)
endif()

################################################################################
# .NET bindings
################################################################################
option(Z3_BUILD_DOTNET_BINDINGS "Build .NET bindings for Z3" OFF)
if (Z3_BUILD_DOTNET_BINDINGS)
    if (NOT Z3_BUILD_LIBZ3_SHARED)
        message(FATAL_ERROR "The .NET bindings will not work with a static libz3. "
                "You either need to disable Z3_BUILD_DOTNET_BINDINGS or enable Z3_BUILD_LIBZ3_SHARED")
    endif()
    add_subdirectory(z3/src/api/dotnet)
endif()

################################################################################
# Java bindings
################################################################################
option(Z3_BUILD_JAVA_BINDINGS "Build Java bindings for Z3" OFF)
if (Z3_BUILD_JAVA_BINDINGS)
    if (NOT Z3_BUILD_LIBZ3_SHARED)
        message(FATAL_ERROR "The Java bindings will not work with a static libz3. "
                "You either need to disable Z3_BUILD_JAVA_BINDINGS or enable Z3_BUILD_LIBZ3_SHARED")
    endif()
    add_subdirectory(z3/src/api/java)
endif()

################################################################################
# Julia bindings
################################################################################
option(Z3_BUILD_JULIA_BINDINGS "Build Julia bindings for Z3" OFF)
if (Z3_BUILD_JULIA_BINDINGS)
    if (NOT Z3_BUILD_LIBZ3_SHARED)
        message(FATAL_ERROR "The Julia bindings will not work with a static libz3."
                "You either need to disable Z3_BUILD_JULIA_BINDINGS or enable Z3_BUILD_LIBZ3_SHARED")
    endif()
    add_subdirectory(z3/src/api/julia)
endif()

# TODO: Implement support for other bindigns

SET(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR_TMP})