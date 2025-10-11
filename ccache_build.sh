#!/bin/bash
# ccache_build.sh - 使用ccache加速构建的脚本

set -e

BUILD_TYPE=${1:-Debug}
BUILD_DIR=build-ccache
INCREMENTAL=${2:-false}

# 显示使用方法
usage() {
    echo "用法: $0 [BUILD_TYPE] [INCREMENTAL]"
    echo "  BUILD_TYPE: 构建类型 (Debug|Release) 默认: Debug"
    echo "  INCREMENTAL: 是否增量编译 (true|false) 默认: false"
    echo ""
    echo "注意: 需要先安装ccache: sudo apt-get install ccache"
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

# 检查ccache是否安装
if ! command -v ccache &> /dev/null; then
    echo "错误: 未找到ccache，请先安装:"
    echo "Ubuntu/Debian: sudo apt-get install ccache"
    echo "CentOS/RHEL: sudo yum install ccache"
    exit 1
fi

echo "构建类型: $BUILD_TYPE"
echo "增量编译: $INCREMENTAL"
echo "CPU核心数: $(nproc)"
echo "CCache版本: $(ccache --version | head -n 1)"

# 显示ccache统计信息
echo ""
echo "CCache统计信息:"
ccache -s

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

# 配置使用ccache
export CC="ccache gcc"
export CXX="ccache g++"

# 只在非增量模式下重新运行cmake
if [[ "$INCREMENTAL" == "false" ]]; then
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
fi

echo "开始编译..."
# 使用所有CPU核心进行并行编译
make -j$(nproc)

echo ""
echo "编译完成！可执行文件位于 bin/ 目录中"
echo ""
echo "CCache更新后的统计信息:"
ccache -s