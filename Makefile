CC=g++
CFLAGS=-std=c++11 -Wall
OBJ = utils.o graph_relabeling.o
DEPS = utils.h
EXEC=graph_relabeling

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(EXEC)