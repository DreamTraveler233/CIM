#!/bin/bash

rm -rf bin/test_config_thread_sefa
cd "build"
cmake ..
make
echo "编译完成！"
cd ..
cd bin
./test_config_thread_sefa