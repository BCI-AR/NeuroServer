# In a real application, please use the autoconf/automake system by Gnu.
#

main: main.c
	gcc $< -o $@ `pkg-config --cflags --libs gtk+-2.0` -Wall -g

clean:
	rm main
