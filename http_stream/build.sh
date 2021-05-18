#!/bin/bash

g++-8 --std=c++17 stream_main.cpp mpngdec.cpp read_stream.cpp string_utils.cpp -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lcurl
