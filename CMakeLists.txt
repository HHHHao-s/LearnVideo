﻿# CMakeList.txt: sdl-01 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MyProject")

set(COMPILE_FLAGS
	-Wall  -lm
) 


set(
	CMAKE_CXX_STANDARD 17
)

if (CMAKE_HOST_WIN32)
	set(INCLUDE_DIR
		${CMAKE_SOURCE_DIR}/env/win/SDL2/include
		${CMAKE_SOURCE_DIR}/env/win/ffmpeg/include
	)
	set(LINK_DIR
		${CMAKE_SOURCE_DIR}/env/win/SDL2/lib/x64
		${CMAKE_SOURCE_DIR}/env/win/ffmpeg/lib
	)
elseif(CMAKE_HOST_UNIX)
	set(INCLUDE_DIR
		${CMAKE_SOURCE_DIR}/env/linux_shared/include
		${CMAKE_SOURCE_DIR}/env/linux_shared/include/SDL2
	)
	set(LINK_DIR
		${CMAKE_SOURCE_DIR}/env/linux_shared/lib
	)
endif()


include_directories(${INCLUDE_DIR})


# 将源代码添加到此项目的可执行文件。
# 设置所有的执行程序
set(CEXECUTABLES mux encode-video encode_pcm_aac 07-08-demuxer-mp4 07-09-avio resample_audio demux-decode scaler)
set(CPPEXECUTABLES ffmpegTester sdl-01 parse_pcm parse_yuv420p)
# 设置需要链接的库
set(LIBS m SDL2 avcodec avformat swscale avdevice avutil avfilter swresample postproc)
if (CMAKE_HOST_UNIX)
	set(LIBS ${LIBS} fdk-aac x264)	
	# set(CMAKE_INSTALL_RPATH ${LINK_DIR})
	set(ENV{LD_LIBRARY_PATH} ${LINK_DIR})
	# set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
# message(STATUS "LIBS: ${LIBS}")
# link libs
foreach(EXECUTABLE ${CEXECUTABLES})
	add_executable(${EXECUTABLE} src/${EXECUTABLE}.c)
	target_link_directories(${EXECUTABLE} PRIVATE ${LINK_DIR})
	target_link_libraries(${EXECUTABLE} ${LIBS})
endforeach()

foreach(EXECUTABLE ${CPPEXECUTABLES})
	add_executable(${EXECUTABLE} src/${EXECUTABLE}.cpp)
	target_link_directories(${EXECUTABLE} PRIVATE ${LINK_DIR})
	target_link_libraries(${EXECUTABLE} ${LIBS})
endforeach()

# set working directory
set_target_properties(${CPPEXECUTABLES} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/res)
set_target_properties(${CEXECUTABLES} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/res)

# mp4Muxer project
add_executable(mp4Muxer src/mp4Muxer/include/resampler.h src/mp4Muxer/include/videoEncoder.h src/mp4Muxer/src/videoEncoder.cpp src/mp4Muxer/src/main.cpp src/mp4Muxer/src/muxer.cpp src/mp4Muxer/include/muxer.h src/mp4Muxer/include/audioEncoder.h src/mp4Muxer/src/audioEncoder.cpp)
target_link_libraries(mp4Muxer ${LIBS})
target_link_directories(mp4Muxer PRIVATE ${LINK_DIR})
target_include_directories(mp4Muxer PRIVATE src/mp4Muxer/include)
set_target_properties(mp4Muxer PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/res)

# debug前要export LD_LIBRARY_PATH=${LIBS}:$LD_LIBRARY_PATH