cmake_minimum_required(VERSION 3.20)
project(myserver)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)

# cJSON source files
file(GLOB CJSON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cJSON/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/cJSON/*.h")

# Check if cJSON sources exist
if(NOT CJSON_SOURCES)
    message(FATAL_ERROR "cJSON source files not found. Please clone cJSON from https://github.com/DaveGamble/cJSON.git into the root of your project.")
endif()

find_program(PROTOC_EXECUTABLE protoc)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

if (NOT PROTOC_EXECUTABLE)
    message(FATAL_ERROR "protoc executable not found")
endif()

if (NOT GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "grpc_cpp_plugin executable not found")
endif()

set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")

add_custom_command(
    OUTPUT ${PROTO_GEN_DIR}/proto/hello.pb.cc ${PROTO_GEN_DIR}/proto/hello.pb.h
           ${PROTO_GEN_DIR}/proto/hello.grpc.pb.cc ${PROTO_GEN_DIR}/proto/hello.grpc.pb.h
    COMMAND ${PROTOC_EXECUTABLE} --proto_path=${PROTO_DIR}
            --grpc_out=${PROTO_GEN_DIR}/proto
            --cpp_out=${PROTO_GEN_DIR}/proto
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
            ${PROTO_DIR}/hello.proto
    DEPENDS ${PROTO_DIR}/hello.proto
    COMMENT "Generating Protobuf and gRPC source files from ${PROTO_DIR}/hello.proto"
)

add_library(protolib STATIC
    ${PROTO_GEN_DIR}/proto/hello.pb.cc
    ${PROTO_GEN_DIR}/proto/hello.grpc.pb.cc
    ${CJSON_SOURCES} # Add cJSON sources here
)

#httplib
include(FetchContent)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.10.3
)
FetchContent_MakeAvailable(httplib)

#target libraries
target_link_libraries(protolib
    gRPC::grpc++
    protobuf::libprotobuf
    tinyxml2::tinyxml2
    httplib::httplib
)

#directories
target_include_directories(protolib PUBLIC
    ${PROTO_GEN_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cJSON # Include cJSON directory
)

#server
add_executable(server src/server.cc src/sqlite3.c src/db.cc)
target_link_libraries(server
    protolib
    gRPC::grpc++
    tinyxml2::tinyxml2
    httplib::httplib
)

#server
target_include_directories(server PRIVATE
    ${PROTO_GEN_DIR}
    ${PROTO_GEN_DIR}/proto
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    sqlite3
    httplib::httplib
)

#client
add_executable(client src/client.cc src/sqlite3.c)
target_link_libraries(client
    protolib
    gRPC::grpc++
    tinyxml2::tinyxml2
)

#client
target_include_directories(client PRIVATE
    ${PROTO_GEN_DIR}
    ${PROTO_GEN_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    sqlite3
)