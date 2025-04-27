cmake_minimum_required(VERSION 3.25)

project(MultithreadDownload
    VERSION 1.0.0
    DESCRIPTION "A simple multithreaded app"
)

find_package(CURL 8.13 REQUIRED)

set(APP_EXE download)

add_executable(${APP_EXE} main.cpp)

target_include_directories(${APP_EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(${APP_EXE} PUBLIC cxx_std_17)
target_sources(${APP_EXE} PUBLIC src/instance.cpp src/download_curl.cpp)

# linking curl
target_include_directories(${APP_EXE} PRIVATE ${CURL_INCLUDE_DIRS})
target_link_libraries(${APP_EXE} PRIVATE ${CURL_LIBRARIES})