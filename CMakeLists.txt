cmake_minimum_required(VERSION 3.10)

project(HelloWorld)

find_package(fmt CONFIG REQUIRED)

find_package(google_cloud_cpp_speech REQUIRED)

find_package(portaudio CONFIG REQUIRED)

add_executable(HelloWorld recorder.cpp)

target_link_libraries(HelloWorld PRIVATE fmt::fmt)

target_link_libraries(HelloWorld PUBLIC google-cloud-cpp::speech)

target_link_libraries(HelloWorld PRIVATE portaudio)
