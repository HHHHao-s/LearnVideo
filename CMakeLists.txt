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
add_executable (ffmpegTester "ffmpegTester.cpp")

add_executable (SDL_01 "sdl-01.cpp")


set(EXECUTABLES ffmpegTester SDL_01)
set(FFMPEG_LIBS avcodec avformat swscale avdevice avutil)
target_link_libraries(ffmpegTester SDL2 ${FFMPEG_LIBS})
target_link_libraries(SDL_01 SDL2 ${FFMPEG_LIBS})
# TODO: 如有需要，请添加测试并安装目标。
