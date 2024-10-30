#include <stdio.h>
#include "dummy_main.h"

unsigned long long fib(int n) {
    if (n<2) return n;
    else return fib(n - 2) + fib(n - 1);
}

int main(int argc, char** argv){
    int n= 41;
    printf("Fib(%d) = %llu\n", n, fib(n));
    return 0;
}