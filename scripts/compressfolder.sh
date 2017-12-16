#!/bin/bash
# Runs compress.py on all files in a directory.
for f in *; 
do python compress.py $f; 
done;
