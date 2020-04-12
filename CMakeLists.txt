cmake_minimum_required (VERSION 3.5)
project (iab-in-imf)

# import boost

#set(Boost_USE_STATIC_LIBS ON)
#set(Boost_USE_MULTITHREADED ON)
#find_package(Boost REQUIRED COMPONENTS program_options)
#include_directories(${Boost_INCLUDE_DIR})

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

# project files

#file(GLOB_RECURSE SRC_FILES src/main/*.cpp src/*.h )
#file(GLOB_RECURSE INCLUDE_FILES src/main/*.h )
#include_directories(${INCLUDE_FILES})
#add_executable(${PROJECT_NAME} ${SRC_FILES} ${INCLUDE_FILES})
#target_link_libraries(${PROJECT_NAME} libas02)
#target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} libas02)

#set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
#set_property(TARGET ${PROJECT_NAME} PROPERTY LINKER_LANGUAGE CXX)

# tests

enable_testing()

include(CTest)

file( GLOB_RECURSE UNIT_TESTS src/test/*.cpp )

foreach( UNIT_TEST_PATH ${UNIT_TESTS} )
	get_filename_component(UNIT_TEST_NAME ${UNIT_TEST_PATH} NAME_WE)
    add_executable(${UNIT_TEST_NAME} ${UNIT_TEST_PATH} )
    target_link_libraries( ${UNIT_TEST_NAME} libas02)
	add_test(${UNIT_TEST_NAME}  ${UNIT_TEST_NAME} WORKINGDIRECTORY "${CMAKE_BINARY_DIR}")
endforeach( UNIT_TEST_PATH ${UNIT_TESTS} )


