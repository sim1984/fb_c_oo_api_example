cmake_minimum_required (VERSION 3.13)

project (example 
    VERSION 1.3.3
    LANGUAGES C
)

add_executable(example)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)
    set(FIREBIRD_DIR C:/Firebird/5.0)
elseif(UNIX)
    set(FIREBIRD_DIR /opt/firebird)
endif()

set(FIREBIRD_INCLUDE_DIR ${FIREBIRD_DIR}/include)


target_sources(example PRIVATE
    "example.c"
    "fb_api_impl.c"
)

target_compile_definitions(example PRIVATE HAVE_CONFIG_H)

if(CMAKE_COMPILER_IS_GNUCC)
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
elseif(MSVC)    
    target_compile_options(example PRIVATE 
        -W4 
        -EHsc 
#        -WX
    )
endif()

####################################
# include directories
####################################
target_include_directories(example PRIVATE
    ${FIREBIRD_INCLUDE_DIR}
)

if(MSVC)
    target_link_directories(example PRIVATE ${FIREBIRD_DIR}/lib)

    target_link_libraries(example fbclient_ms.lib)
else()    
    target_link_libraries(example -lfbclient)
endif()


