@ECHO OFF
cls
del "a.exe"
g++ .\server.cpp -lws2_32 -std=c++11 -pthread
a 1.0.3.46 11111
