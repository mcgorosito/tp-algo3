all:
	g++ -ggdb -Wall -Wno-unused-parameter -Wextra -no-pie -pedantic -m64 -O2 -march=native -o pcmi pcmi.cpp -std=c++14
	

clean:
	rm -f pcmi