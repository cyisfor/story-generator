cmake_minimum_required(VERSION 3.0)
project(story-generator VERSION 0.1)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/add_module")
include(AddModule)

include(FindSQLite3)

add_compile_options(-fdiagnostics-color=always)

if(SQLite3_FOUND)
else()
	message(FATAL_ERROR "SQLite3 not found!")
endif()

include(FindPkgConfig)

function(external_package target prefix)
	target_include_directories("${target}" PUBLIC
		"${${prefix}_INCLUDE_DIRS}")
	target_compile_options("${target}" PUBLIC
		"${${prefix}_CFLAGS}")
	target_link_options("${target}" PUBLIC
		"${${prefix}_LDFLAGS}")
	target_link_libraries("${target}" PUBLIC
		"${${prefix}_LIBRARIES}")
endfunction()

add_module(htmlish
  GIT e469f0e09cbb45aab938edd03388e18483cc26f0
  file:///home/repo/htmlish
  git@github.com:cyisfor/htmlish.git
  https://github.com/cyisfor/htmlish.git)

# note: we have to set OBJECT_DEPENDS manually
# since gcc rage quits if the include file is not found
# and cmake doesn't parse the rage quit message to find what
# file needs to be generated.
set_source_files_properties(src/storydb.c
	PROPERTIES
	OBJECT_DEPENDS "db.sql.stmt.c")

add_module(cstuff
  GIT 874ae5456e63ea82ddba9cca149600a71be06daf
  file:///home/repo/cstuff
  git@github.com:cyisfor/cstuff.git
  https://github.com/cyisfor/cstuff.git)

add_cstuff(record)
  
add_library(justdb
	src/db.c)
target_link_libraries(justdb PUBLIC
		htmlish # for no error thingies from database
		cstuff)
target_include_directories(justdb PUBLIC
	"${CMAKE_CURRENT_BINARY_DIR}" # db_oid
	)

external_package(justdb SQLite3)

add_library(storydb
	src/storydb.c)
target_link_libraries(storydb PUBLIC
	justdb)

#######################################
# THE PROGRAMS

add_executable(generate
	category.gen.c
	ddate/ddate.c
	src/main.c
	src/storygit.c
	src/repo.c
	src/create.c
	cstuff/itoa.c)

pkg_check_modules(GIT
	REQUIRED
	libgit2)
external_package(generate GIT)

target_include_directories(generate PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/ddate")
target_link_libraries(generate PRIVATE storydb)

add_executable(ready
	src/ready.c
	cstuff/itoa.c)
target_link_libraries(ready PRIVATE storydb)
external_package(ready GIT)

add_executable(news_log
	src/news-log.c
	src/repo.c
	cstuff/become.c)
target_include_directories(news_log PRIVATE cstuff)
target_link_libraries(news_log PRIVATE storydb)
external_package(news_log GIT)

add_executable(make_log
	src/make-log.c
	src/repo.c
	cstuff/itoa.c
	cstuff/become.c)
target_include_directories(make_log PRIVATE cstuff)
target_link_libraries(make_log PRIVATE storydb xmlfixes)
external_package(make_log GIT)

set_source_files_properties(src/make-log.c
	PROPERTIES
	OBJECT_DEPENDS
	"template/make-log.html.c;template/make-log.row.html.c"
	)

add_executable(make_contents
	src/make-contents.c
	cstuff/itoa.c)
target_include_directories(make_contents PRIVATE cstuff)
target_link_libraries(make_contents PRIVATE storydb xmlfixes)
external_package(make_contents GIT)
set_source_files_properties(src/make-contents.c
	OBJECT_DEPENDS
	"template/page.html.c;template/contents-body.html.c;template/contents-story.html.c")

add_executable(describe
	src/describe.c
	cstuff/itoa.c
	src/db.c)
target_include_directories(describe PRIVATE cstuff)
target_link_libraries(describe PRIVATE storydb xml readline)
external_package(describe GIT)

add_executable(find_chapters_to_publish
	src/find-chapters-to-publish.c
	cstuff/itoa.c)
target_include_directories(find_chapters_to_publish PRIVATE cstuff)
target_link_libraries(find_chapters_to_publish PRIVATE storydb)
external_package(find_chapters_to_publish GIT)

##############################################33
# Now a bunch of generated files

add_module(str_to_enum_trie
  GIT 445f60aca94dc7d8006df4780f7a97114d9bf750
  file:///home/repo/str_to_enum_trie
  git@github.com:cyisfor/str_to_enum_trie.git
  https://github.com/cyisfor/str_to_enum_trie.git)

# And now you learn the depths of my madness

file(MAKE_DIRECTORY template)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/add_module")
include(AddModule)

add_module(ctemplate 
  GIT 970b8ecbe6557a491f75ee606284e399ecad254d
  file:///home/repo/ctemplate
  git@github.com:cyisfor/ctemplate.git
  https://github.com/cyisfor/ctemplate.git)

function(ctemplate name)
	add_ctemplate(
		"${name}"
		"${CMAKE_CURRENT_SOURCE_DIR}/template/${name}"
		"template/${name}.c")
endfunction()

ctemplate(statements2source.c)
ctemplate(make-log.html)
ctemplate(make-log.row.html)
# for make-contents
ctemplate(page.html)
ctemplate(contents-body.html)
ctemplate(contents-story.html)

str_to_enum_trie(category src/categories.list
  ENUM CATEGORY
  NOUPPER)

add_executable(statements2source
	EXCLUDE_FROM_ALL
	src/statements2source.c
	cstuff/itoa.c
	cstuff/mmapfile.c)

external_package(statements2source SQLite3)
external_package(statements2source GIT)
target_include_directories(statements2source PUBLIC
  cstuff
  "${CMAKE_CURRENT_SOURCE_DIR}/htmlish/src")
target_link_libraries(statements2source PRIVATE htmlish justdb)

add_custom_command(
	OUTPUT db.sql.stmt.c
	COMMAND
	onlydefine=1
	"./statements2source"
	derp
	<"${CMAKE_CURRENT_SOURCE_DIR}/src/db.sql.stmt"
	>	"${CMAKE_CURRENT_BINARY_DIR}/db.sql.stmt.c"
	MAIN_DEPENDENCY
	"${CMAKE_CURRENT_SOURCE_DIR}/src/db.sql.stmt"
	DEPENDS
	statements2source)

set_source_files_properties(src/statements2source.c
	PROPERTIES
	OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/template/statements2source.c.c")

add_executable(dboid_make
	src/db_oid/make.c)

add_custom_command(
	OUTPUT db_oid/gen.h
	COMMAND
	"./dboid_make"
	db_oid "${CMAKE_CURRENT_SOURCE_DIR}/src/db_oid"
	DEPENDS
	dboid_make)
file(MAKE_DIRECTORY db_oid)

set_source_files_properties(src/db.c
	PROPERTIES
	OBJECT_DEPENDS
	"db_oid/gen.h;schema.sql.gen.c;indexes.sql.gen.c"
	)

function(makeSQL name)
	set(src	"${CMAKE_CURRENT_SOURCE_DIR}/src/${name}.sql")
	set(dest "${name}.sql.gen.c")
	add_custom_command(
		OUTPUT "${dest}"
		COMMAND
		"./make_sql"
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

# str_to_enum_trie makes category.gen.c makes generate
# make-sql.c makes make-sql makes schema.sql.c makes justdb
# db_oid/make.c makes make_dboid makes db_oid/gen.h makes justdb
# ctemplate/ctemplate makes statements2source.c.c makes statements2source.c makes statements2source makes storydb
