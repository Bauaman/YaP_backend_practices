cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${importTargets}")

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Установите режим сборки по умолчанию в Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
	src/logger.cpp	
	src/logger.h
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model_app.cpp
	src/model_app.h
	src/model_game.cpp
	src/model_game.h
	src/model.cpp
	src/model.h
	src/tagged.h
	src/command_line_parser.h
	src/ticker.h
	src/json_loader.cpp
	src/json_loader.h
	src/request_handler.cpp
	src/request_handler.h
	src/api_handler.cpp
	src/api_handler.h
	src/game_server.h
	src/response_maker.cpp
	src/response_maker.h
	src/aux.cpp	
	src/aux.h
)
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)