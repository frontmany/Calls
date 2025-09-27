@echo off
if not exist build mkdir build
if not exist vendor mkdir vendor
cd build
cmake .. -DDEPENDENCIES_FETCHED=TRUE -WIN_CONSOLE=FALSE