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
target_include_directories(storydb PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/htmlish/src"
	"${SQLite3_INCLUDE_DIRS}"
	)

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

add_executable(statements2source
	EXCLUDE_FROM_ALL
	src/statements2source.c
	src/db.c
	src/note.c
	cystuff/src/itoa.c
	cystuff/src/mmapfile.c)

target_include_directories(statements2source PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/cystuff/src"
	"${SQLite3_INCLUDE_DIRS}")
target_link_libraries(statements2source "${SQLite3_LIBRARIES}")

add_custom_command(
	OUTPUT db.sql.stmt.c
	COMMAND
	onlydefine=1
	"${CMAKE_CURRENT_BINARY_DIR}/statements2source"
	derp
	<"${CMAKE_CURRENT_SOURCE_DIR}/db.sql.stmt"
	>	"${CMAKE_CURRENT_BINARY_DIR}/db.sql.stmt.c"
	MAIN_DEPENDENCY
	"${CMAKE_CURRENT_SOURCE_DIR}/db.sql.stmt"
	DEPENDS
	statements2source)
