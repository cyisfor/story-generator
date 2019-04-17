cmake_minimum_required(VERSION 3.0)
project(story-generator VERSION 0.1)

include(FindSQLite3)

if(SQLite3_FOUND)
else()
	message(FATAL_ERROR "SQLite3 not found!")
endif()

include(FindPkgConfig)

function(external_package target prefix)
	target_include_directories("${target}" PRIVATE
		"${${prefix}_INCLUDE_DIRS}")
	target_compile_options("${target}" PRIVATE
		"${${prefix}_CFLAGS}")
	target_link_options("${target}" PRIVATE
		"${${prefix}_LDFLAGS}")
	target_link_libraries("${target}" PRIVATE
		"${${prefix}_LIBRARIES}")
endfunction()

add_subdirectory(htmlish)

set_source_files_properties(src/storydb.c
	PROPERTIES
	OBJECT_DEPENDS "db.sql.stmt.c")

add_library(justdb
	src/db.c)
target_link_libraries(justdb PUBLIC
		htmlish # for no error thingies from database
		)
target_include_directories(justdb PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/htmlish/src"
	"${CMAKE_CURRENT_BINARY_DIR}" # db_oid
	)

external_package(justdb SQLite3)

add_library(storydb
	src/storydb.c)
target_link_libraries(storydb PUBLIC
	justdb)


add_executable(generate
	category.gen.c
	ddate/ddate.c
	src/main.c
	src/storygit.c
	src/repo.c
	src/create.c
	src/note.c
	cystuff/src/itoa.c)

pkg_check_modules(GIT
	REQUIRED
	libgit2)
external_package(generate GIT)

target_include_directories(generate PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/ddate")
target_link_libraries(generate PRIVATE storydb)

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
	src/note.c
	cystuff/src/itoa.c
	cystuff/src/mmapfile.c)

external_package(statements2source SQLite3)
target_include_directories(statements2source PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/cystuff/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/htmlish/src"
	)
target_link_libraries(statements2source PRIVATE htmlish justdb)

add_custom_command(
	OUTPUT db.sql.stmt.c
	COMMAND
	onlydefine=1
	"${CMAKE_CURRENT_BINARY_DIR}/statements2source"
	derp
	<"${CMAKE_CURRENT_SOURCE_DIR}/src/db.sql.stmt"
	>	"${CMAKE_CURRENT_BINARY_DIR}/db.sql.stmt.c"
	MAIN_DEPENDENCY
	"${CMAKE_CURRENT_SOURCE_DIR}/src/db.sql.stmt"
	DEPENDS
	statements2source)

add_executable(dboid_make
	src/db_oid/make.c)

add_custom_command(
	OUTPUT db_oid/gen.h
	COMMAND
	"${CMAKE_CURRENT_BINARY_DIR}/dboid_make"
	db_oid "${CMAKE_CURRENT_SOURCE_DIR}/src/db_oid"
	DEPENDS
	dboid_make)
file(MAKE_DIRECTORY db_oid)

set_source_files_properties(src/db.c
	PROPERTIES
	OBJECT_DEPENDS
	"db_oid/gen.h;schema.sql.gen.c;indexes.sql.gen.c"
	)

# And now you learn the depths of my insanity

file(MAKE_DIRECTORY template)
add_custom_command(
	OUTPUT template/statements2source.c.c
	COMMAND
	"${CMAKE_CURRENT_BINARY_DIR}/ctemplate/ctemplate"
	<"${CMAKE_CURRENT_SOURCE_DIR}/template/statements2source.c"
	>"template/statements2source.c.c"
	MAIN_DEPENDENCY
	"${CMAKE_CURRENT_SOURCE_DIR}/template/statements2source.c"
	DEPENDS
	ctemplate)
add_subdirectory(ctemplate)

set_source_files_properties(src/statements2source.c
	PROPERTIES
	OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/template/statements2source.c.c")

function(makeSQL name)
	set(src	"${CMAKE_CURRENT_SOURCE_DIR}/src/${name}.sql")
	set(dest "${name}.sql.gen.c")
	add_custom_command(
		OUTPUT "${dest}"
		COMMAND
		"${CMAKE_CURRENT_BINARY_DIR}/make_sql"
		<"${src}"
		>"${dest}"
		MAIN_DEPENDENCY
		"${src}"
		DEPENDS
		make_sql
		)
endfunction()

makeSQL(schema)
makeSQL(indexes)

add_executable(make_sql
	src/make-sql.c)
