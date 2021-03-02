CFLAGS := -g -Wall
LDFLAGS := -lX11 -lasound -lm

bar: bar.o

clean:
	rm *.o bar
