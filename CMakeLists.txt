cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(regexp VERSION 1.0.0 LANGUAGES CXX)
include(CTest)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_STANDARD_REQUIRED on)

add_library(${PROJECT_NAME}lib STATIC
    regexplib.cpp
    regexplib.hpp
)

if (BUILD_TESTING)
    find_package(GTest REQUIRED)

    if (SANITIZER)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=undefined")
        if (${SANITIZER} MATCHES "ASAN")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
        elseif(${SANITIZER} MATCHES "TSAN")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
        endif()
        if (${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=implicit-conversion")
        endif()
    endif ()

    # add_executable(${PROJECT_NAME}-ut
    #     ut.cpp
    # )

    # target_link_libraries(${PROJECT_NAME}-ut PRIVATE
    #     ${PROJECT_NAME}lib
    #     gtest::gtest
    # )
endif ()

add_executable(${PROJECT_NAME}
    regexp.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}lib
)
