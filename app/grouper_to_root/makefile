ROOT_CFLAGS = `root-config --cflags`
ROOT_LIBS = `root-config --libs` 


compile: grouper_to_root.cxx
	g++ -o grouper_to_root.exe grouper_to_root.cxx $(ROOT_CFLAGS) $(ROOT_LIBS)

clean:
	rm -f *.exe


