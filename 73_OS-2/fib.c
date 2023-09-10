#include <stdio.h>
#include <stdlib.h>
int fibonacci(int n) {
    if (n <= 0) {
        return 0;
    } else if (n == 1) {
        return 1;
    } else {
        int a = 0;
        int b = 1;
        int result = 0;
        for (int i = 2; i <= n; i++) {
            result = a + b;
            a = b;
            b = result;
        }
        return result;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    if (n < 0) {
        fprintf(stderr, "Please provide a non-negative integer for n.\n");
        return EXIT_FAILURE;
    }

    int result = fibonacci(n);
    printf("The %dth Fibonacci number is: %d\n", n, result);

    return EXIT_SUCCESS;
}
