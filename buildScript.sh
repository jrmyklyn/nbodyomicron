#!/bin/bash
gcc -pthread nbody.c -lGL -lGLU -lglut -lm -I./CsvParser/include CsvParser/src/csvparser.c -o nbody.out
