#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <time.h>

// Global variable to store the total execution time
double total_execution_time = 0;
// Global counter to keep track of the number of parallel_for executions.
int counter = 1;



int user_main(int argc, char **argv);

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}

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
    std::cout<<"\nTotal Execution Time for all parallel_for calls: "<<total_execution_time<<" milliseconds\n\n";
    // printf("Test Success. \n\n");

    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main


// Structure for single-index (1D range) parallel execution.
typedef struct {
  std::function<void(int)> lambda;
  int start;
  int end;
} thread_function_vector;

// Function executed by each thread in parallel_for (single-index version).
void* thread_func_for(void *ptr){
  thread_function_vector *data = (thread_function_vector*)ptr;
  for(int i = data->start; i < data->end; i++){
    data->lambda(i);
  }
  return NULL;
}

void parallel_for(int start, int end, std::function<void(int)> && lambda, int numThread) {
    thread_function_vector args[numThread - 1];  // Create only (numThread - 1) thread data.
    pthread_t thread_ids[numThread - 1];         // Array to hold (numThread - 1) thread identifiers.

    // Calculate the chunks size
    int range = end - start;
    int chunk_size = range / numThread;

    // Start the clock to record execution time
    clock_t start_time = clock();

    // Main thread handles the first chunk of work
    int main_start = start;
    int main_end = main_start + chunk_size;
    for (int i = main_start; i < main_end; i++) {
        lambda(i);  // Main thread executes its portion of work
    }

    // Create remaining threads for the rest of the work
    for (int i = 0; i < numThread - 1; i++) {
        args[i].lambda = lambda;
        args[i].start = main_end + i * chunk_size;
        if (i == numThread - 2) {
            args[i].end = end;
        } else {
            args[i].end = args[i].start + chunk_size;
        }
        
        // Create thread to handle assigned range
        if (pthread_create(&thread_ids[i], NULL, thread_func_for, (void*)&args[i]) != 0) {
            std::cerr << "Error creating thread" << std::endl;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < numThread - 1; i++) {
        if (pthread_join(thread_ids[i], NULL) != 0) {
            std::cerr << "Error joining thread" << std::endl;
        }
    }

    // Stop the clock and calculate total execution time
    clock_t end_time = clock();
    double total_time = ((double)(end_time - start_time)) * 1000.0 / CLOCKS_PER_SEC;

    // Print total execution time for all parallel_for call
    printf("\nTotal Execution Time for parallel_for call %d: %f milliseconds\n\n", counter, total_time);

    counter++;            // Increment the counter
    total_execution_time += total_time;  // Add to the total execution time
}




/*our parallel_for implementation
start1 and end1 are the range of the outer loop
start2 and end2 are the range of the inner loop
lambda is the function to be executed in parallel
numThreads is the number of threads to be used*/

// Structure for double-index (2D range) parallel execution.
typedef struct {
  std::function<void(int, int)> lambda;
  int start1;
  int end1;
  int start2;
  int end2;
} thread_function_matrix;

// Function executed by each thread in parallel_for (doble-index version).
void* thread_func_double_for(void *ptr){
  thread_function_matrix *data = (thread_function_matrix*)ptr;

  // Nested loop over the assigned 2D range
  for(int i = data->start1; i < data->end1; i++){
    for(int j = data->start2; j < data->end2; j++){
      data->lambda(i, j);
    }
  }
  return NULL;
}



void parallel_for(int start1, int end1, int start2, int end2, std::function<void(int, int)> && lambda, int numThread) {
    thread_function_matrix args[numThread - 1];  // Create only (numThread - 1) thread data.
    pthread_t thread_ids[numThread - 1];         // Array to hold (numThread - 1) thread identifiers.

    // Calculate the chunks size
    int range = end1 - start1;
    int chunk_size = range / numThread;

    // Start the clock to record execution time
    clock_t start_time = clock();

    // Main thread handles the first chunk of rows
    int main_start1 = start1;
    int main_end1 = main_start1 + chunk_size;
    for (int i = main_start1; i < main_end1; i++) {
        for (int j = start2; j < end2; j++) {
            lambda(i, j);  // Main thread executes its portion of work
        }
    }

    // Create remaining threads for the rest of the rows
    for (int i = 0; i < numThread - 1; i++) {
        args[i].lambda = lambda;
        args[i].start1 = main_end1 + i * chunk_size;
        args[i].start2 = start2;
        args[i].end2 = end2;

        // Last thread takes any remainder of the division.
        if (i == numThread - 2) {
            args[i].end1 = end1;
        } else {
            args[i].end1 = args[i].start1 + chunk_size;
        }

        // Create thread to handle assigned range
        if (pthread_create(&thread_ids[i], NULL, thread_func_double_for, (void*)&args[i]) != 0) {
            std::cerr << "Error creating thread" << std::endl;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < numThread - 1; i++) {
        if (pthread_join(thread_ids[i], NULL) != 0) {
            std::cerr << "Error joining thread" << std::endl;
        }
    }

    // Stop the clock and calculate total execution time
    clock_t end_time = clock();
    double total_time = ((double)(end_time - start_time)) * 1000.0 / CLOCKS_PER_SEC;

    // Print total execution time for all parallel_for call
    printf("\nTotal Execution Time for parallel_for call %d: %f milliseconds\n\n", counter, total_time);

    counter++;    // Increment the counter
    total_execution_time += total_time;  // Add to the total execution time
}
