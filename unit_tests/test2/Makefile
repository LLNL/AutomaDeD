CXX=mpic++

# This is the path where the library was installed on
STATETRACKER_LIB=../../../install/lib

all:
	$(CXX) -g -c test.C 
	$(CXX) -g -o test test.o -lstracker -L$(STATETRACKER_LIB) -rdynamic


clean:
	rm -f *.o test
