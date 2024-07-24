cmake_minimum_required (VERSION 3.13)

project (example 
    VERSION 1.3.3
    LANGUAGES C
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(FIREBIRD_DIR /opt/firebird)
set(FIREBIRD_INCLUDE_DIR ${FIREBIRD_DIR}/include)

add_executable(example ${luceneudr_sources})
target_sources(example PRIVATE
    "example.c"
    "fb_api_impl.c"
)

target_compile_definitions(example PRIVATE HAVE_CONFIG_H)
target_compile_options(example PRIVATE
#    -Werror

    -Wall
    -Wextra
    -Wpedantic

    -Wcast-align
    -Wconversion
    -Wenum-compare
    -Wfloat-equal
    -Wredundant-decls
    -Wno-unused-parameter
)


####################################
# include directories
####################################
target_include_directories(example PRIVATE
    ${FIREBIRD_INCLUDE_DIR}
)

target_link_libraries(example 
    -lstdc++fs
    -lfbclient
)

