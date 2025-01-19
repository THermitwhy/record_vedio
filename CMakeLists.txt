# CMakeList.txt: last30 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
set(PROJECT_NAME "record_vedio")
#project(${PROJECT_NAME})
# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()
project (${PROJECT_NAME})
include_directories("E:/soft/ffmpeg-master-latest-win64-gpl-shared/include")#
include_directories("D:/lib/libyuv/include")#D:\lib\portaudio-19.7.0\include
include_directories("D:/lib/portaudio-19.7.0/include")
# 将源代码添加到此项目的可执行文件。
add_executable (${PROJECT_NAME}  "utils.hpp" "bvedio.h" "bvedio.cpp"  "dx_capture.hpp""baudio.h" "vedio_.cpp")
#add_executable (${PROJECT_NAME}  "dx_capture.hpp")
target_link_directories(${PROJECT_NAME} PUBLIC "E:/soft/ffmpeg-master-latest-win64-gpl-shared/lib")
target_link_directories(${PROJECT_NAME} PUBLIC "D:/lib/libyuv/Debug")
target_link_directories(${PROJECT_NAME} PUBLIC "D:lib/portaudio-19.7.0/bin")
target_link_libraries(${PROJECT_NAME} "avcodec.lib" "avdevice.lib" "swscale.lib" "avformat.lib" "avutil.lib" "yuv.lib")
target_link_libraries(${PROJECT_NAME} "portaudio_x64.lib")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
