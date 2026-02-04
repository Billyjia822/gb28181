#!/bin/bash

# GB28181协议层完善 - 编译测试脚本

echo "=========================================="
echo "  GB28181 Protocol Enhancement Build Test"
echo "=========================================="
echo ""

# 设置项目根目录
PROJECT_ROOT="/home/leejia/share/workspace/Gb28181"
BUILD_DIR="$PROJECT_ROOT/build"

# 检查项目目录
if [ ! -d "$PROJECT_ROOT" ]; then
    echo "错误: 项目目录不存在: $PROJECT_ROOT"
    exit 1
fi

# 创建构建目录
echo "步骤 1: 创建构建目录..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
echo "✓ 构建目录创建完成"
echo ""

# 运行CMake配置
echo "步骤 2: 运行CMake配置..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "✗ CMake配置失败"
    exit 1
fi
echo "✓ CMake配置完成"
echo ""

# 编译项目
echo "步骤 3: 编译项目..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "✗ 编译失败"
    exit 1
fi
echo "✓ 编译完成"
echo ""

# 检查可执行文件
echo "步骤 4: 检查可执行文件..."
if [ -f "$BUILD_DIR/bin/gb28181_device" ]; then
    echo "✓ 可执行文件已生成: $BUILD_DIR/bin/gb28181_device"
    ls -lh "$BUILD_DIR/bin/gb28181_device"
else
    echo "✗ 可执行文件未生成"
    exit 1
fi
echo ""

# 检查库文件
echo "步骤 5: 检查库文件..."
if [ -f "$BUILD_DIR/lib/libgb28181_core.a" ]; then
    echo "✓ 核心库已生成: $BUILD_DIR/lib/libgb28181_core.a"
    ls -lh "$BUILD_DIR/lib/libgb28181_core.a"
else
    echo "✗ 核心库未生成"
    exit 1
fi
echo ""

echo "=========================================="
echo "  编译测试完成 - 所有检查通过!"
echo "=========================================="
echo ""
echo "新增功能模块:"
echo "  ✓ MD5工具类 (SIP Digest认证)"
echo "  ✓ SDP协商器 (媒体协商)"
echo "  ✓ XML解析器 (MANSCDP协议)"
echo "  ✓ 录像管理器 (RecordInfo)"
echo "  ✓ 回放管理器 (Playback)"
echo "  ✓ 告警管理器 (Alarm)"
echo "  ✓ 配置管理器 (DeviceConfig)"
echo "  ✓ PTZ控制器 (增强版)"
echo ""
