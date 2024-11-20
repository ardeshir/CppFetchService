cmake_minimum_required(VERSION 3.10)  
project(fetch_service)  
  
set(CMAKE_CXX_STANDARD 17)  
  
# Find PostgreSQL and libpqxx packages  
find_package(PostgreSQL REQUIRED)  
find_package(PkgConfig REQUIRED)  
pkg_check_modules(PQXX REQUIRED libpqxx)  
  
# Include directories for PostgreSQL and libpqxx  
include_directories(${PostgreSQL_INCLUDE_DIRS})  
include_directories(${PQXX_INCLUDE_DIRS})  
  
# Add the executable  
add_executable(fetch fetch.cpp)  
  
# Link libraries for PostgreSQL, libpqxx, and CURL  
target_link_libraries(fetch ${PostgreSQL_LIBRARIES} ${PQXX_LIBRARIES} curl) 