# In a real application, please use the autoconf/automake system by Gnu.
#

main: main.c ../src/nsutil.o ../src/nsnet.o ../src/openedf.o ../src/pctimer.o
	gcc $< -o $@ `pkg-config --cflags --libs gtk+-2.0` -Wall -g -I../src ../src/nsutil.o ../src/nsnet.o ../src/openedf.o ../src/pctimer.o

clean:
	rm main
