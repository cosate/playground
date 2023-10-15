dump: dump.cpp
	g++ --std=c++11 -o dump.o dump.cpp

clean:
	rm *.o