cmake_minimum_required(VERSION 3.11)

project(filetest CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(filetest
	src/main.cpp
	#src/http_server.cpp
	#src/http_server.h
	#src/sdk.h
	#src/model.h
	#src/model.cpp
	#src/tagged.h
	#src/boost_json.cpp
	#src/json_loader.h
	#src/json_loader.cpp
	#src/request_handler.cpp
	#src/request_handler.h
)
target_link_libraries(filetest PRIVATE Threads::Threads)
