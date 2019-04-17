cmake_minimum_required(VERSION 3.0)
project(htmlish VERSION 0.1)

include(FindSQLite3)

if("${SQLite_FOUND}")
else()
	message(FATAL_ERROR SQLite3 not found!)
endif()

add_subdirectory(htmlish)

add_library(storydb PUBLIC
	src/db.c
	src/storydb.c)
target_link_libraries(storydb PUBLIC
	htmlish
	"${SQLite3_LIBRARIES}")
target_include_directories(storydb PUBLIC "${SQLite3_INCLUDE_DIRS}")

add_executable(generate
	  category.gen.c
	ddate/ddate.c
	src/main.c
	src/storygit.c
	src/repo.c
	src/create.c
	src/note.c
	cystuff/src/itoa.c)
