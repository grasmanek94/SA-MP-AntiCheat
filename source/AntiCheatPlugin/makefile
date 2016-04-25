# This file demonstrates how to compile the AntiCheat project on Linux.
#
# To compile AntiCheat do:
#
# make AntiCheat 
#

GPP = g++
GCC = gcc
AntiCheat_OUTFILE = "./AntiCheat.so"

COMPILE_FLAGS = -std=c++0x -m32 -fPIC -c -I ./../../../sampgdk-3.4-Linux/include -I ./../../../sampgdk-3.4-Linux/lib -w -D LINUX -D PROJECT_NAME=\"AntiCheat\"

AntiCheat = -D AntiCheat $(COMPILE_FLAGS)

all: AntiCheat

clean:
	-rm -f *~ *.o *.so

AntiCheat: clean
	$(GPP) $(AntiCheat) *.cpp
	$(GPP) -lsampgdk -m32 -fshort-wchar -shared -o $(AntiCheat_OUTFILE) *.o

