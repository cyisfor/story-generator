cmake_minimum_required(VERSION 3.0)
project(story-generator VERSION 0.1)

include(FindSQLite3)

if(SQLite3_FOUND)
else()
	message(FATAL_ERROR "SQLite3 not found!")
endif()

add_subdirectory(htmlish)

add_library(storydb
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

target_include_directories(generate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ddate")
target_link_libraries(generate storydb)

add_subdirectory(str_to_enum_trie)

add_custom_command(OUTPUT category.gen.c category.gen.h
	COMMAND
	noupper=1 prefix=category enum=CATEGORY exec "${CMAKE_CURRENT_BINARY_DIR}/str_to_enum_trie/str_to_enum_trie" <src/categories.list
	MAIN_DEPENDENCY
	src/categories.list
	DEPENDS
	str_to_enum_trie)

