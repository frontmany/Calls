@echo off
if not exist build mkdir build
if not exist vendor mkdir vendor
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DDEPENDENCIES_FETCHED=FALSE