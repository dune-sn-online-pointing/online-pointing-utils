ROOT_CFLAGS = `root-config --cflags`
ROOT_LIBS = `root-config --libs` 

# project/
# ├── inc
# │   ├── group_to_root_libs.h
# │   ├── group.h
# │   └── position_calculator.h
# ├── src
# │   ├── group.cpp
# │   ├── group_to_root_libs.cpp
# │   └── position_calculator.cpp
# └── app
# 	└── group_to_root
# 		├── makefile
# 		└── group_to_root.cxx


# Compiler and compiler flags
group_to_root: group_to_root.cpp group.o position_calculator.o group_to_root_libs.o
	g++ group_to_root.cpp group.o position_calculator.o group_to_root_libs.o -I../../inc -o group_to_root.exe $(ROOT_CFLAGS) $(ROOT_LIBS)

group_to_root_libs.o: ../../src/group_to_root_libs.cpp ../../inc/group_to_root_libs.h
	g++ -c ../../src/group_to_root_libs.cpp -I../../inc -o group_to_root_libs.o $(ROOT_CFLAGS) $(ROOT_LIBS)

position_calculator.o: ../../src/position_calculator.cpp ../../inc/position_calculator.h
	g++ -c ../../src/position_calculator.cpp -I../../inc -o position_calculator.o

group.o: ../../src/group.cpp ../../inc/group.h
	g++ -c ../../src/group.cpp -I../../inc -o group.o

clean:
	rm -f *.o
	rm -f group_to_root.exe



