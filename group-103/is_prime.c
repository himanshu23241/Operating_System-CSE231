int isPrime(int n) {
    if (n < 2) return 0; // Numbers less than 2 are not prime
    
    for (int i = 2; i < n; i++) {
        if (n % i == 0) {
            return 0; // If n is divisible by any number other than 1 and itself, it is not prime
        }
    }
    return 1; // If no divisors were found, n is prime
}

int _start() {
    int val = isPrime(10); // Check if 10 is prime
    return val;
}
