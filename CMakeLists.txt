cmake_minimum_required(VERSION 3.16)

project(HTTPServerP
		VERSION 1.0
		DESCRIPTION "Http Server"
		LANGUAGES C CXX)

include(FetchContent)

FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)

FetchContent_MakeAvailable(googletest)

include(CTest)
include(GoogleTest)

add_subdirectory(src)
add_subdirectory(test)