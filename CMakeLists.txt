cmake_minimum_required(VERSION 2.8)
project (jco)

set(cpps
    src/parser.cpp
    src/serialization.cpp
    src/out_stream.cpp
    src/printer_base.cpp
    src/printer_factory.cpp
    src/single_line_printer.cpp
    src/pretty_printer.cpp
)

file(GLOB_RECURSE headers src/*.h include/*.h)

include_directories(include)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror=return-type")

add_library(jco
${cpps}
${headers}
)

add_subdirectory(examples)
add_subdirectory(tests)
