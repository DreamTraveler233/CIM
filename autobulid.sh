#!/bin/bash

rm -rf bin/test_thread
cd "build"
cmake ..
make
echo "编译完成！"
cd ..
cd bin
./test_thread