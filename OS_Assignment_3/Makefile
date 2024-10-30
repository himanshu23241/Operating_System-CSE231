all: Dam_Shell run clean

Dam_Shell:
	gcc -o Dam_Shell simpleshell.c Dam_Scheduler.c -Werror -Wall -Wextra

run:
	./Dam_Shell 4 500

clean:
	rm -rf Dam_Shell
