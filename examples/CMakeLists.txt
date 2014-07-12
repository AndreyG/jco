project(examples)

set(CMAKE_CXX_FLAGS "-std=c++1y")

add_executable(ss_decoding ss_decoding.cpp)
target_link_libraries(ss_decoding jco)

add_executable(heterogeneous heterogeneous.cpp)
target_link_libraries(heterogeneous jco)
