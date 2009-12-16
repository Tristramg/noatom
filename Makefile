CXX=g++
CXXFLAGS=-W -Wall -ansi -pedantic -g -Wno-deprecated 
LDFLAGS=-lClp -lCoinUtils
EXEC=noatom

all: $(EXEC)

noatom: parse.o instance.o outages.o
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.c %.h
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)


