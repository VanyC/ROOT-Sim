#!/bin/bash

$HOME/local-install/bin/rootsim-cc -pthread *.c -g -o sugar
#./sugar --np 2 --nprc 4
./sugar --sequential --nprc 4
