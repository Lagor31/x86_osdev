#!/bin/sh 
find . -name '*.c' -o -name '*.asm' -o -name '*.h' | xargs wc -l | sort -nr
