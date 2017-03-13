cmake_minimum_required(VERSION 2.8.8)


# 1: find LLVM package
if (NOT LLVM_BUILD_PATH)
    message(FATAL_ERROR "please specified the build folder of LLVM by -DLLVM_BUILD_PATH=")
else()
    message(STATUS "LLVM_BUILD_PATH set to : ${LLVM_BUILD_PATH}")
endif()

find_package(LLVM
        REQUIRED
        CONFIG
        PATHS ${LLVM_BUILD_PATH}
        NO_DEFAULT_PATH)

if(NOT LLVM_FOUND)
    message(FATAL_ERROR "can not find LLVMConfig.cmake under paths: ${LLVM_BUILD_PATH}")
else()
    message(STATUS "Found LLVM : ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
    message(STATUS "LLVM header files locate in : ${LLVM_INCLUDE_DIRS}")
    message(STATUS "LLVM definitions is : ${LLVM_DEFINITIONS}")

    # we incorporate the CMake features provided by LLVM:
    list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
    include(AddLLVM)
    include(HandleLLVMOptions)
    set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
    set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")

    # set(LLVM_REQUIRES_RTTI TRUE)
    # set(LLVM_REQUIRES_EH TRUE)

    include_directories(${LLVM_INCLUDE_DIRS})
    link_directories(${LLVM_LIBRARY_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
    message(STATUS "definitions in LLVM ${LLVM_DEFINITIONS}")
endif()


include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/z3/src/api/c++
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/z3/src/api/
)

include_directories(BEFORE
        ${LLVM_BUILD_MAIN_SRC_DIR}/lib)
include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -Wall -Wno-comment -Wno-deprecated -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -L/usr/local/lib")

if(UNIX)
    SET(Z3LibExt .so)
else()
    SET(Z3LibExt .dll)
endif()

if(NOT ENABLE_STATICZ3)
    SET(Z3LinkOption -fopenmp -lrt)
    SET(Z3LibraryName "libz3${Z3LibExt}")
else()
    SET(Z3LinkOption -fopenmp -lrt)
    SET(Z3LibraryName "libz3.a")
endif()


# 5: add function definitions
function(PP_GET_SOURCE_FILES RETURN_LIST)
    file(GLOB ORIGINAL
            "*.c"
            "*.cpp"
            "*.cc")

    foreach(g ${ORIGINAL})
        get_filename_component(g ${g} NAME)
        list(APPEND FILES ${g})
    endforeach()
    set(${RETURN_LIST} ${FILES} PARENT_SCOPE)
endfunction()

function(PP_ADD_LIBRARY LIB_NAME)
    PP_GET_SOURCE_FILES(SRC_LISTS)
    message(STATUS "found source files: ${SRC_LISTS}")
    add_llvm_library(${LIB_NAME}
            ${SRC_LISTS})
endfunction()

function(PP_ADD_SHARED_LIBRARY LIB_NAME)
    PP_GET_SOURCE_FILES(SRC_LISTS)
    message(STATUS "found source files: ${SRC_LISTS}")
    add_llvm_loadable_module(${LIB_NAME}
            ${SRC_LISTS})
endfunction()

function(PP_ADD_ALL_SUBDIRS)
    file(GLOB entries *)
    foreach(entry ${entries})
        if(IS_DIRECTORY ${entry} AND EXISTS ${entry}/CMakeLists.txt)
            add_subdirectory(${entry})
        endif()
    endforeach(entry)
endfunction()

add_subdirectory(lib)
add_subdirectory(third-party)
add_subdirectory(tools)