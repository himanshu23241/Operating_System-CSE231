all:
	gcc -m32 -o Smart_loader Smart_loader.c
	gcc -m32 -no-pie -nostdlib -o sum sum.c
	gcc -m32 -no-pie -nostdlib -o fib fib.c
	gcc -m32 -no-pie -nostdlib -o is_prime is_prime.c

clean:
	-@rm -f Smart_loader sum fib is_prime