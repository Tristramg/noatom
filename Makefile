CXX=g++
CXXFLAGS=-W -Wall -ansi -pedantic -O3 -Wno-deprecated 
LDFLAGS=-lgecodeint -lgecodesearch -lgecodekernel -lgecodescheduling -lboost_program_options -lgecodeminimodel
STATIC_LDFLAGS=  -lgecodedriver  -lgecodesearch -lgecodescheduling -lgecodegraph -lgecodeminimodel -lgecodeset -lgecodeint -lgecodekernel -lgecodesupport  -lboost_program_options -lpthread 

EXEC=noatom

all: $(EXEC)

static:  constraints.o instance.o gloutony.o
	$(CXX) -o $@ $^ -static-libgcc -static $(STATIC_LDFLAGS) 

noatom: constraints.o instance.o gloutony.o
	$(CXX) -o $@ $^   $(LDFLAGS)

%.o: %.c %.h
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)


