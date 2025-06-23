cmake_minimum_required(VERSION 3.10)

project(HelloWorld)

find_package(fmt CONFIG REQUIRED)

find_package(google_cloud_cpp_speech REQUIRED)

add_executable(HelloWorld helloworld.cpp)

target_link_libraries(HelloWorld PRIVATE fmt::fmt)

target_link_libraries(HelloWorld PUBLIC google-cloud-cpp::speech)
