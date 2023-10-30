# CMakeList.txt: sdl-01 的 CMake 项目，在此处包括源代码并定义
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
	-Wall
)


set(
	CXX_STANDARD 17
)

set(INCLUDE_DIR
	E:/SDL2/include
	E:/ffmpeg/include
)

set(LINK_DIR
	E:/SDL2/lib/x64
	E:/ffmpeg/lib
)



include_directories(${INCLUDE_DIR})
link_directories(${LINK_DIR})

# 将源代码添加到此项目的可执行文件。
add_executable (ffmpegTester "src/ffmpegTester.cpp")
add_executable (SDL_01 "src/sdl-01.cpp")
add_executable (parse_pcm "src/parse_pcm.cpp")
add_executable (parse_yuv420p "src/parse_yuv420p.cpp")
add_executable (07-08-demuxer-mp4 "src/07-08-demuxer-mp4.c")
# 设置所有的执行程序
set(EXECUTABLES ffmpegTester SDL_01 parse_pcm parse_yuv420p 07-08-demuxer-mp4)

# 设置需要链接的库
set(LIBS SDL2 avcodec avformat swscale avdevice avutil)
# link libs
foreach(EXECUTABLE ${EXECUTABLES})
	target_link_libraries(${EXECUTABLE} ${LIBS})
endforeach()

# set working directory
set_target_properties(${EXECUTABLES} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/res)


