ROOT_CFLAGS = `root-config --cflags`
ROOT_LIBS = `root-config --libs` 

# project/
# ├── inc
# │   ├── superimpose_root_files_libs.h
# │   ├── group.h
# │   └── position_calculator.h
# ├── src
# │   ├── group.cpp
# │   ├── superimpose_root_files_libs.cpp
# │   └── position_calculator.cpp
# └── app
# 	└── superimpose_root_files
# 		├── makefile
# 		└── superimpose_root_files.cxx


# Compiler and compiler flags
superimpose_root_files: superimpose_root_files.cpp group.o position_calculator.o superimpose_root_files_libs.o group_to_root_libs.o
	g++ superimpose_root_files.cpp group.o position_calculator.o superimpose_root_files_libs.o group_to_root_libs.o -I../../inc -o superimpose_root_files.exe $(ROOT_CFLAGS) $(ROOT_LIBS)

superimpose_root_files_libs.o: ../../src/superimpose_root_files_libs.cpp ../../inc/superimpose_root_files_libs.h
	g++ -c ../../src/superimpose_root_files_libs.cpp -I../../inc -o superimpose_root_files_libs.o $(ROOT_CFLAGS) $(ROOT_LIBS)

position_calculator.o: ../../src/position_calculator.cpp ../../inc/position_calculator.h
	g++ -c ../../src/position_calculator.cpp -I../../inc -o position_calculator.o

group.o: ../../src/group.cpp ../../inc/group.h
	g++ -c ../../src/group.cpp -I../../inc -o group.o

group_to_root_libs.o: ../../src/group_to_root_libs.cpp ../../inc/group_to_root_libs.h
	g++ -c ../../src/group_to_root_libs.cpp -I../../inc -o group_to_root_libs.o $(ROOT_CFLAGS) $(ROOT_LIBS)


clean:
	rm -f *.o
	rm -f superimpose_root_files.exe



