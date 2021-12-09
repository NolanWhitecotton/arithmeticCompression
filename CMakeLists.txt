cmake_minimum_required(VERSION 3.11)

# set the project name
project(arithmeticCompression CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCES 
	arithmeticCompression.cpp
)

#Create Example with all above sources
add_executable(
	${PROJECT_NAME} ${SOURCES}
)
