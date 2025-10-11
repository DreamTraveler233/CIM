#!/bin/bash
# ninja_build.sh - 使用Ninja构建系统的脚本

set -e

BUILD_TYPE=${1:-Debug}
BUILD_DIR=build-ninja
INCREMENTAL=${2:-false}

# 显示使用方法
usage() {
    echo "用法: $0 [BUILD_TYPE] [INCREMENTAL]"
    echo "  BUILD_TYPE: 构建类型 (Debug|Release) 默认: Debug"
    echo "  INCREMENTAL: 是否增量编译 (true|false) 默认: false"
    echo ""
    echo "注意: 需要先安装Ninja: sudo apt-get install ninja-build"
    echo ""
    echo "示例:"
    echo "  $0                    # Debug构建，完整编译"
    echo "  $0 Release            # Release构建，完整编译"
    echo "  $0 Debug true         # Debug构建，增量编译"
    echo "  $0 Release true       # Release构建，增量编译"
}

# 检查参数
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    usage
    exit 0
fi

# 检查Ninja是否安装
if ! command -v ninja &> /dev/null; then
    echo "错误: 未找到Ninja，请先安装:"
    echo "Ubuntu/Debian: sudo apt-get install ninja-build"
    echo "CentOS/RHEL: sudo yum install ninja-build"
    exit 1
fi

echo "构建类型: $BUILD_TYPE"
echo "增量编译: $INCREMENTAL"
echo "CPU核心数: $(nproc)"
echo "Ninja版本: $(ninja --version)"

if [[ "$INCREMENTAL" == "false" ]]; then
    echo "清理旧的构建文件..."
    rm -rf $BUILD_DIR
    echo "创建构建目录..."
    mkdir -p $BUILD_DIR
else
    echo "使用增量编译模式..."
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "构建目录不存在，创建新目录..."
        mkdir -p $BUILD_DIR
    fi
fi

echo "进入构建目录并执行 cmake..."
cd $BUILD_DIR

# 只在非增量模式下重新运行cmake
if [[ "$INCREMENTAL" == "false" ]]; then
    # 使用Ninja作为生成器
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -G Ninja
fi

echo "开始编译..."
# 使用Ninja进行并行编译
ninja

echo "编译完成！可执行文件位于 bin/ 目录中"