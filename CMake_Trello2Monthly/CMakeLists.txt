# CMakeList.txt : CMake project for CMake_Trello2Monthly, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)


# Add source to this project's executable.
set(SOURCES monthly.cpp monthly.h)
add_executable (CMake_Trello2Monthly "CMake_Trello2Monthly.cpp" "CMake_Trello2Monthly.h" "monthly.cpp" "monthly.cpp")
find_package(cpprestsdk REQUIRED)
target_link_libraries(CMake_Trello2Monthly PRIVATE cpprestsdk::cpprest)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(CMake_Trello2Monthly PRIVATE spdlog::spdlog)
# TODO: Add tests and install targets if needed.
