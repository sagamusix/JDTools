cmake_minimum_required(VERSION 3.16)

project(JDTools)
add_executable(JDTool
	JDTools/Convert800to990.cpp
	JDTools/Convert800toVST.cpp
	JDTools/Convert990to800.cpp
	JDTools/ConvertVSTto800.cpp
	JDTools/InputFile.cpp
	JDTools/JDTools.cpp
	JDTools/SVZ.cpp
	JDTools/InputFile.hpp
	JDTools/JD-08.hpp
	JDTools/JD-800.hpp
	JDTools/JD-990.hpp
	JDTools/JDTools.hpp
	JDTools/PrecomputedTablesVST.hpp
	JDTools/SVZ.hpp
	JDTools/Utils.hpp
	JDTools/miniz.c
	JDTools/miniz.h
	JDTools/resource.h)

if(WIN32)
	target_sources(JDTools PRIVATE
		JDTools/JDTools.manifest
		JDTools/JDTools.rc)
endif()

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

set_property(TARGET JDTool PROPERTY CXX_STANDARD 20)
