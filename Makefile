CFLAGS = -Wall -pedantic
LDFLAGS = -lsndfile -lasound -lpthread -lncurses -lfftw3 -lm

OBJ =  audio.o graphics.o main.o

visualizer: $(OBJ)
	gcc $(OBJ) $(CFLAGS) $(LDFLAGS) -o $@

%.o: %.c
	gcc -c $^

.PHONY: clean
clean:
	rm -f $(OBJ) visualizer
