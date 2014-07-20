find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})

add_executable(tests
src/serialization.cpp
)

target_link_libraries(tests jco ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES})
