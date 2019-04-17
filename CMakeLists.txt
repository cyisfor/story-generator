cmake_minimum_required(VERSION 3.0)
project(htmlish VERSION 0.1)

include(FindSQLite3)



add_library(storydb PUBLIC
	src/db.c 
	src/storydb.c)
target_link_libraries(storydb PUBLIC "${SQLite3_LIBRARIES}")
target_include_directories(storydb PUBLIC "${SQLite3_INCLUDE_DIRS}")


add_executable(parse
	src/main.c)
