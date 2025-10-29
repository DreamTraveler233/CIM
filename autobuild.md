# 自动化构建脚本使用说明

本项目提供了一个名为 `autobuild.sh` 的自动化构建脚本，用于简化项目的编译和测试流程。以下是该脚本的使用说明：

## 基本用法

在项目根目录下，打开终端并运行以下命令以执行默认的调
试构建：
```bash
bash autobuild.sh
```
这将创建一个名为 `build` 的构建目录，并在其中进行调试构建。

## 可选参数

- `--release`：执行发布构建，而非默认的调试构建。
```bash
bash autobuild.sh --release
```

- `--with-tests`：在构建完成后运行单元测试。
```bash
bash autobuild.sh --with-tests
```

- `--run-smoke`：在构建完成后运行烟雾测试服务器。
```bash
bash autobuild.sh --run-smoke
```

- `--port <port_number>`：指定烟雾测试服务器的端口号（默认为 8080）。
```bash
bash autobuild.sh --run-smoke --port 9090
```