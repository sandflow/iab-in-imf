cmake_minimum_required (VERSION 2.8)
project (iab-in-imf)

# import boost

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIR})

# required for asdcplib

if(WIN32)
	add_definitions(/DKM_WIN32 /D_CONSOLE /DASDCP_GCMULTI_PATCH /DASDCP_PLATFORM=\"win32\" /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
elseif(UNIX)
	add_definitions(/DASDCP_GCMULTI_PATCH /DASDCP_PLATFORM=\"unix\")
endif(WIN32)

# import asdcplib

add_subdirectory(external/asdcplib)
include_directories(external/asdcplib/src)

# include files

file(GLOB_RECURSE INCLUDE_FILES src/main/cpp/*.h )
include_directories(${INCLUDE_FILES})

# compiler/linker settings

set_property(GLOBAL PROPERTY CXX_STANDARD 11)
set_property(GLOBAL PROPERTY LINKER_LANGUAGE CXX)

# IABWriter project files

set(IA_WRITER "ia-imf-writer")
add_executable(${IA_WRITER} src/main/cpp/writer.cpp ${INCLUDE_FILES})
target_link_libraries(${IA_WRITER} ${Boost_LIBRARIES} libas02)

# IABReader project files

set(IA_READER "ia-imf-reader")
add_executable(${IA_READER} src/main/cpp/reader.cpp ${INCLUDE_FILES})
target_link_libraries(${IA_READER} ${Boost_LIBRARIES} libas02)

# install scripts

install(TARGETS ${IA_WRITER} ${IA_READER} RUNTIME DESTINATION bin)

# tests

enable_testing()

include(CTest)

file( GLOB_RECURSE UNIT_TESTS src/test/cpp/*.cpp )

# cli tests

add_test(NAME "ia-frame-wrap" COMMAND ${IA_WRITER} --in "${PROJECT_SOURCE_DIR}/src/test/resources/ia-frames" --out ia-frames.mxf)

set(IA_FRAMES_OUTPUT_DIR "${CMAKE_BINARY_DIR}/ia-frames")
file(MAKE_DIRECTORY "${IA_FRAMES_OUTPUT_DIR}")
add_test(NAME "ia-frame-unwrap" COMMAND ${IA_READER} --in ia-frames.mxf --out "${IA_FRAMES_OUTPUT_DIR}")


# lib tests

foreach( UNIT_TEST_PATH ${UNIT_TESTS} )
	get_filename_component(UNIT_TEST_NAME ${UNIT_TEST_PATH} NAME_WE)
    add_executable(${UNIT_TEST_NAME} ${UNIT_TEST_PATH} )
    target_link_libraries( ${UNIT_TEST_NAME} libas02)
	add_test(${UNIT_TEST_NAME}  ${UNIT_TEST_NAME} WORKINGDIRECTORY "${CMAKE_BINARY_DIR}")
endforeach( UNIT_TEST_PATH ${UNIT_TESTS} )


