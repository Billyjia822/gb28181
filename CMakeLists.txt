cmake_minimum_required(VERSION 3.10)
project(GB28181Device VERSION 1.0.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/3rd/osip/include
    ${PROJECT_SOURCE_DIR}/3rd/exosip/include
)

# 3rd库源文件
set(OSIP_SOURCES
    ${PROJECT_SOURCE_DIR}/3rd/osip/src/osip_parser.c
)

set(EXOSIP_SOURCES
    ${PROJECT_SOURCE_DIR}/3rd/exosip/src/eXosip.c
)

# 创建oSIP库
add_library(osip STATIC ${OSIP_SOURCES})

# 创建eXosip库
add_library(exosip STATIC ${EXOSIP_SOURCES})
target_link_libraries(exosip osip)

# 源文件
file(GLOB_RECURSE SIP_SOURCES "src/sip/*.cpp")
file(GLOB_RECURSE RTP_SOURCES "src/rtp/*.cpp")
file(GLOB_RECURSE MEDIA_SOURCES "src/media/*.cpp")
file(GLOB_RECURSE DEVICE_SOURCES "src/device/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")
file(GLOB_RECURSE PS_SOURCES "src/ps/*.cpp")

# 创建核心库
add_library(gb28181_core STATIC
    ${SIP_SOURCES}
    ${RTP_SOURCES}
    ${MEDIA_SOURCES}
    ${DEVICE_SOURCES}
    ${UTILS_SOURCES}
    ${PS_SOURCES}
)

target_link_libraries(gb28181_core
    exosip
)

# 创建可执行文件
add_executable(gb28181_device
    src/main.cpp
)

target_link_libraries(gb28181_device
    gb28181_core
)

# Windows平台特定设置
if(WIN32)
    target_link_libraries(gb28181_device ws2_32 winmm)
    target_link_libraries(exosip ws2_32 winmm)
endif()

# 安装规则
install(TARGETS gb28181_device
    RUNTIME DESTINATION bin
)

install(FILES config/device_config.json
    DESTINATION config
)
