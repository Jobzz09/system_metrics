cmake_minimum_required(VERSION 3.0.0)
project(rb_metrics VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
    
add_compile_options(-g)

include(CTest)
enable_testing()

file(GLOB TARGET_SRC "./src/*.cpp" )

add_executable(${PROJECT_NAME} ${TARGET_SRC})

#========== Boost ==========
set (BOOST_COMPONENTS
    thread 
    filesystem
    unit_test_framework
    json
    system
    )

set (Boost_USE_STATIC_LIBS ON)

set (Boost_USE_MULTITHREADED ON)
    
include_directories(${Boost_INCLUDE_DIRS})
    
find_package(Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED) 
    
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_include_directories(
    ${PROJECT_NAME} PRIVATE include
)
