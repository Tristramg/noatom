CXX=g++
CXXFLAGS=-W -Wall -ansi -pedantic -g -Wno-deprecated 
LDFLAGS=-lgecodeint -lgecodesearch -lgecodekernel -lgecodesupport -lgecodescheduling -lgecodegist -lgecodedriver
EXEC=noatom

all: $(EXEC)

noatom: constraints.o instance.o gloutony.o
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.c %.h
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)


