#ifndef SIMPLEMULTITHREADER_H
#define SIMPLEMULTITHREADER_H

#include <stdlib.h>
#include <iostream>
#include <functional>
#include <pthread.h>
using namespace std;

double total_time = 0 ;

struct SimpleMultithreader {
    // parallel_for for one-dimensional loop
    static void parallel_for(int low, int high, function<void(int)> &&lambda, int numThreads);

    // parallel_for for two-dimensional loop
    static void parallel_for(int low1, int high1, int low2, int high2, function<void(int, int)> &&lambda, int numThreads);
};
#define parallel_for SimpleMultithreader::parallel_for

//structure for storing vector parameters
struct ThreadParams1D {
    function<void(int)> lambda;
    int threadLow;
    int threadHigh;
};

//structure for storing matrix parameters
struct ThreadParams2D {
    function<void(int, int)> lambda;
    int threadLow1;
    int threadHigh1;
    int low2;
    int high2;
};

//function for vector
void SimpleMultithreader::parallel_for(int low, int high, function<void(int)> &&lambda, int numThreads) {
    clock_t start_time = clock();

    pthread_t threads[numThreads];
    int step = (high - low) / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int threadLow = low + i * step;
        int threadHigh = (i == numThreads - 1) ? high : threadLow + step;

        ThreadParams1D *params = new ThreadParams1D;
        params->lambda = lambda;
        params->threadLow = threadLow;
        params->threadHigh = threadHigh;

        pthread_create(&threads[i], nullptr, [](void *args) -> void * {
            ThreadParams1D *params = static_cast<ThreadParams1D *>(args);
            function<void(int)> lambda = params->lambda;
            int threadLow = params->threadLow;
            int threadHigh = params->threadHigh;

            for (int j = threadLow; j < threadHigh; ++j) {
                lambda(j);
            }

            free(params);
            return nullptr;
        }, params);
    }

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    clock_t end_time = clock();
    double elapsed_time = double(end_time - start_time) * 1000 / CLOCKS_PER_SEC;
    total_time  += elapsed_time;
    printf("Total execution time: %f seconds ms\n", elapsed_time);
}

//function for matrix
void SimpleMultithreader::parallel_for(int low1, int high1, int low2, int high2, function<void(int, int)> &&lambda, int numThreads) {
    clock_t start_time = clock();

    pthread_t threads[numThreads];
    int step1 = (high1 - low1) / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int threadLow1 = low1 + i * step1;
        int threadHigh1 = (i == numThreads - 1) ? high1 : threadLow1 + step1;

        ThreadParams2D *params = new ThreadParams2D;
        params->lambda = lambda;
        params->threadLow1 = threadLow1;
        params->threadHigh1 = threadHigh1;
        params->low2 = low2;
        params->high2 = high2;

        pthread_create(&threads[i], nullptr, [](void *args) -> void * {
            ThreadParams2D *params = static_cast<ThreadParams2D *>(args);
            function<void(int, int)> lambda = params->lambda;
            int threadLow1 = params->threadLow1;
            int threadHigh1 = params->threadHigh1;
            int low2 = params->low2;
            int high2 = params->high2;

            for (int j = threadLow1; j < threadHigh1; ++j) {
                for (int k = low2; k < high2; ++k) {
                    lambda(j, k);
                }
            }

            free(params);
            return nullptr;
        }, params);
    }

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    clock_t end_time = clock();
    double elapsed_time = double(end_time - start_time) * 1000 / CLOCKS_PER_SEC;
    total_time += elapsed_time;
    printf("Total execution time: %f ms\n", elapsed_time);
}

int user_main(int argc, char **argv);

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}

//main function
int main(int argc, char **argv) {
  /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);
 
    auto /*name*/ lambda2 = [/*nothing captured*/]() {
    printf ("Total time taken : %f ms\n", total_time) ;
    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main


#endif