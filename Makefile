CC=g++
CFLAGS=-Wall -std=c++11
OBJ=utils.o graph_relabeling.o
DEPS=utils.h
EXEC=graph_relabeling

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<	

clean:
	rm -f $(OBJ) $(EXEC)

# http://stackoverflow.com/questions/3220277/what-do-the-makefile-symbols-and-mean