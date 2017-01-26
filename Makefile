CC=g++
FLAGS=-std=c++11 -Wall

all:
	$(CC) $(FLAGS) graph_relabeling.cpp -o graph_relabeling

clean:
	rm graph_relabeling