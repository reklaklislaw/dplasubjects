#!/bin/bash

echo "compiling findfield..."
gcc -O3 -lz -lpthread -Wall findfield.c -o findfield

echo "compiling skosntriple2simplejson..."
gcc -O3 -lz -lpthread -Wall skosntriple2simplejson.c -o skosntriple2simplejson

echo "compiling searchontology..."
gcc -O3 -lz -lpthread -Wall searchontology.c -o searchontology

echo "done"
