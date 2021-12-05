/*  Main code for performance measurements */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "lists.h"

// arguments for subroutines with standard concurrent lists
typedef struct __c_arg_T  {
    int iterations; // num of times to loop
    c_list_T* c_list; // standard concurrent linked list object
} c_arg_T;

// arguments for subroutines with hand over hand lists
typedef struct __h_arg_T    {
    int iterations; // num of times to loop
    h_list_T* h_list; // hand over hand linked list object
} h_arg_T;

// iterative insert subroutine for standard concurrent lists
void *cInsert(void *args)    {
    c_arg_T* c_args = (c_arg_T *) args;
    for (int i = 0; i < c_args->iterations; ++i)    {
        if (cListInsert(c_args->c_list, rand()) < 0)    {
            fprintf(stderr, "error creating new node for standard concurrent linked list: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

// iterative insert subroutine for standard hand over hand lists
void *hInsert(void *args)    {
    h_arg_T* h_args = (h_arg_T *) args;
    for (int i = 0; i < h_args->iterations; ++i)    {
        if (hListInsert(h_args->h_list, rand()) < 0)    {
            fprintf(stderr, "error creating new node for hand over hand linked list: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

// iterative lookup subroutine for standard concurrent lists
void *cLookup(void *args)    {
    c_arg_T* c_args = (c_arg_T *) args;
    for (int i = 0; i < c_args->iterations; ++i)    {
        cListLookup(c_args->c_list, rand());
    }
    return NULL;
}

// iterative lookup subroutine for standard hand over hand lists
void *hLookup(void *args)    {
    h_arg_T* h_args = (h_arg_T *) args;
    for (int i = 0; i < h_args->iterations; ++i)    {
        hListLookup(h_args->h_list, rand());
    }
    return NULL;
}

// (returns microseconds) subtract timevals and convert result into a double
long double timeElapsed(struct timeval start_time, struct timeval end_time)    {
    double rv;
    rv = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec) / 1000.0);
    return rv;
}

int main(int argc, char* argv[])  {

    int tests = 20; // number of tests to be run
    FILE *ofptr = NULL; // output file
    char* ofname = "output.csv";

    // check arrguments
    int opt;
    while ((opt = getopt(argc, argv, "i:o:")) != -1)   {
        switch (opt)
        {
        case 'i':
            tests = atoi(optarg);
            break;
        case 'o':
            ofname = optarg;
            break;
        case '?':
            fprintf(stderr, "usage: %s [-i iterations] [-o outputfile]\n", argv[0]);
            exit(EXIT_FAILURE);
            break;
        default:
            break;
        }
    }

    // open output.csv if file not given
    printf("output file: %s\n", ofname);
    if ((ofptr = fopen(ofname, "w+")) == NULL)  {
        fprintf(stderr, "%s: could not open file %s: %s\n", argv[0], ofname, strerror(errno));
        exit(EXIT_SUCCESS);
    }
    
    // insert head of table
    fprintf(ofptr, "test_no,list_type,iteration,time\n");
    fflush(ofptr);

    pthread_t p[2]; // thread types
    struct timeval start_time, end_time; // used for time measurement

    // test 1: starting with an empty list, two threads running at the same time 
    // insert 10,000 random integers each on the same list
    printf("==========================================\n");
    printf("test 1\n");
    printf("==========================================\n");

    // standard concurrent linked list test
    printf("standard concurrent test\n");
    // package arguments
    c_list_T* c_list_1 = (c_list_T*) malloc(sizeof(c_list_T));
    c_arg_T* c_args = (c_arg_T*) malloc(sizeof(c_arg_T));
    c_args->c_list = c_list_1;
    c_args->iterations = 10000;

    for (int i = 0; i < tests; ++i) {
        // progress
        printf("\r%i/%i tests done\t %% %2.2f", i, tests, ((float) i * 100.0)/((float) tests));
        fflush(stdout);

        // create standard concurrent linked list
        c_list_1 = (c_list_T*) malloc(sizeof(c_list_T));
        cListInit(c_list_1);

        gettimeofday(&start_time, NULL); // start time

        // 2 threads each insert 10,000 random integers
        pthread_create(&(p[0]), NULL, cInsert, c_args);
        pthread_create(&(p[1]), NULL, cInsert, c_args);

        // wait for threads to finish
        pthread_join(p[0], NULL);
        pthread_join(p[1], NULL);

        // get end time, print results
        gettimeofday(&end_time, NULL);
        fprintf(ofptr,"1,standard_concurrent,%i,%.3Lf\n", i+1, timeElapsed(start_time, end_time));
        fflush(ofptr);

        // delete list
        free(c_list_1);
    }
    
    // delete arguments
    free(c_args);
    c_args = NULL;
    c_list_1 = NULL;
    
    // done
    printf("\nstandard concurrent tests done\n");

    printf("------------------------------------------\n");

    // hand over hand concurrent linked list
    printf("hand over hand test\n");
    // package arguments
    h_list_T* h_list_1 = (h_list_T*) malloc(sizeof(h_list_T));
    h_arg_T* h_args = (h_arg_T*) malloc(sizeof(h_arg_T));
    h_args->h_list = h_list_1;
    h_args->iterations = 10000;

    for (int i = 0; i < tests; ++i) { // run test n times
        // progress
        printf("\r%i/%i tests done\t %% %2.2f", i, tests, ((float) i * 100.0)/((float) tests));
        fflush(stdout);

        // create hoh linked list
        h_list_1 = (h_list_T*) malloc(sizeof(h_list_T));
        hListInit(h_list_1);

        // get start time
        gettimeofday(&start_time, NULL);

        // 2 threads each insert 10,000 random integers
        pthread_create(&(p[0]), NULL, hInsert, h_args);
        pthread_create(&(p[1]), NULL, hInsert, h_args);

        // wait for threads to finish
        pthread_join(p[0], NULL);
        pthread_join(p[1], NULL);

        // get end time, print results
        gettimeofday(&end_time, NULL);
        fprintf(ofptr,"1,hand_over_hand,%i,%.3Lf\n", i+1, timeElapsed(start_time, end_time));
        fflush(ofptr);

        // delete list contents
        free(h_list_1);
    }

    // delete arguments
    free(c_args);
    h_args = NULL;
    h_list_1 = NULL; 

    // done
    printf("\nhand over hand tests done\n");

    // test 2: Starting with an empty list, one thread inserts 1 million random 
    // integers, while another thread looks up 10,000 random integers at the same time.
    printf("==========================================\n");
    printf("test 2\n");
    printf("==========================================\n");

    // standard concurrent linked list test
    printf("standard concurrent test\n");
    // package arguments
    c_list_T* c_list_2 = (c_list_T*) malloc(sizeof(c_list_T));
    // 1 million inserts
    c_arg_T* c_args_insert = (c_arg_T*) malloc(sizeof(c_arg_T));
    c_args_insert->c_list = c_list_2;
    c_args_insert->iterations = 1000000;
    // 10000 lookups
    c_arg_T* c_args_lookup = (c_arg_T*) malloc(sizeof(c_arg_T));
    c_args_lookup->c_list = c_list_2;
    c_args_lookup->iterations = 10000;

    for (int i = 0; i < tests; ++i) {
        // progress
        printf("\r%i/%i tests done\t %% %2.2f", i, tests, ((float) i * 100.0)/((float) tests));
        fflush(stdout);

        // create standard concurrent linked list
        cListInit(c_list_2);

        gettimeofday(&start_time, NULL); // start time

        // 1 thread inserts 1 million integers
        pthread_create(&(p[0]), NULL, cInsert, c_args_insert);
        // 1 thread looks up 100000 integers
        pthread_create(&(p[1]), NULL, cLookup, c_args_lookup);

        // wait for threads to finish
        pthread_join(p[0], NULL);
        pthread_join(p[1], NULL);

        // get end time, print results
        gettimeofday(&end_time, NULL);
        fprintf(ofptr,"2,standard_concurrent,%i,%.3Lf\n", i+1, timeElapsed(start_time, end_time));
        fflush(ofptr);

        // delete list;
        free(c_list_2);
        c_list_2 = NULL;
        c_list_2 = (c_list_T*) malloc(sizeof(c_list_T));
        c_args_insert->c_list = c_list_2; // something or rather
        c_args_lookup->c_list = c_list_2;
    }
    
    // delete arguments
    free(c_args_insert);
    c_args_insert = NULL;
    free(c_args_lookup);
    c_args_lookup = NULL;
    free(c_list_2);
    c_list_2 = NULL;

    // done
    printf("\nstandard concurrent tests done\n");

    printf("------------------------------------------\n");
    
    // hand over hand linked list test
    printf("hand over hand test\n");
    // package arguments
    h_list_T* h_list_2 = (h_list_T*) malloc(sizeof(h_list_T));
    hListInit(h_list_2);
    // 1 million inserts
    h_arg_T* h_args_insert = (h_arg_T*) malloc(sizeof(h_arg_T));
    h_args_insert->h_list = h_list_2;
    h_args_insert->iterations = 1000000;
    // 10000 lookups
    h_arg_T* h_args_lookup = (h_arg_T*) malloc(sizeof(h_arg_T));
    h_args_lookup->h_list = h_list_2;
    h_args_lookup->iterations = 10000;

    for (int i = 0; i < tests; ++i) {
        // progress
        printf("\r%i/%i tests done\t %% %2.2f", i, tests, ((float) i * 100.0)/((float) tests));
        fflush(stdout);


        gettimeofday(&start_time, NULL); // start time

        // 1 thread inserts 1 million integers
        pthread_create(&(p[0]), NULL, hInsert, h_args_insert);
        // 1 thread looks up 100000 integers
        pthread_create(&(p[1]), NULL, hLookup, h_args_lookup);

        // wait for threads to finish
        pthread_join(p[0], NULL);
        pthread_join(p[1], NULL);

        // get end time, print results
        gettimeofday(&end_time, NULL);
        fprintf(ofptr,"2,hand_over_hand,%i,%.3Lf\n", i+1, timeElapsed(start_time, end_time));
        fflush(ofptr);

        // delete list;
        free(h_list_2);
        h_list_2 = NULL;
        // create new list
        h_list_2 = (h_list_T*) malloc(sizeof(h_list_T));
        hListInit(h_list_2);
        // update args
        h_args_insert->h_list = h_list_2;
        h_args_lookup->h_list = h_list_2;
    }
    
    // delete arguments
    free(h_args_insert);
    h_args_insert = NULL;
    free(h_args_lookup);
    h_args_lookup = NULL;
    free(h_list_2);
    h_list_2 = NULL;

    // done
    printf("\nhand over hand tests done\n");

    // test 3: Starting with a list containing 1 million random integers, two 
    // threads running at the same time look up 10,000 random integers each.
    printf("==========================================\n");
    printf("test 3\n");
    printf("==========================================\n");

    // standard concurrent linked list test
    printf("standard concurrent test\n");
    
    // create list with 1 million random integers
    c_list_T* c_list_3 = (c_list_T*) malloc(sizeof(c_list_T));
    cListInit(c_list_3);
    // package arguments for insert subroutine
    c_args = (c_arg_T*) malloc(sizeof(c_arg_T));
    c_args->c_list = c_list_3;
    c_args->iterations = 1000000;
    // call insert subroutine
    cInsert(c_args);

    // 10,000 inserts
    c_args->iterations = 10000;

    // iterate for n tests
    for (int i = 0; i < tests; ++i) {
        // progress
        printf("\r%i/%i tests done\t %% %2.2f", i, tests, ((float) i * 100.0)/((float) tests));
        fflush(stdout);

        gettimeofday(&start_time, NULL); // start time

        // 2 threads lookup 10,000 integers
        pthread_create(&(p[0]), NULL, cLookup, c_args);
        pthread_create(&(p[1]), NULL, cLookup, c_args);

        // wait for threads to finish
        pthread_join(p[0], NULL);
        pthread_join(p[1], NULL);

        // get end time, print results
        gettimeofday(&end_time, NULL);
        fprintf(ofptr,"3,standard_concurrent,%i,%.3Lf\n", i+1, timeElapsed(start_time, end_time));
        fflush(ofptr);
    }

    // tidy up
    free(c_list_3);
    c_list_3 = NULL;
    free(c_args);
    c_args = NULL;

    // done
    printf("\nstandard concurrent tests done\n");

    printf("------------------------------------------\n");

    // hand over hand linked list test
    printf("hand over hand test\n");

    // create list with 1 million random integers
    h_list_T* h_list_3 = (h_list_T*) malloc(sizeof(h_list_T));
    hListInit(h_list_3);
    // package arguments for insert subroutine
    h_args = (h_arg_T*) malloc(sizeof(h_arg_T));
    h_args->h_list = h_list_3;
    h_args->iterations = 1000000;
    // call insert subroutine
    hInsert(h_args);

    // 10,000 inserts
    h_args->iterations = 10000;

    // iterate for n tests
    for (int i = 0; i < tests; ++i) {
        // progress
        printf("\r%i/%i tests done\t %% %2.2f", i, tests, ((float) i * 100.0)/((float) tests));
        fflush(stdout);

        gettimeofday(&start_time, NULL); // start time

        // 2 threads lookup 10,000 integers
        pthread_create(&(p[0]), NULL, hLookup, h_args);
        pthread_create(&(p[1]), NULL, hLookup, h_args);

        // wait for threads to finish
        pthread_join(p[0], NULL);
        pthread_join(p[1], NULL);

        // get end time, print results
        gettimeofday(&end_time, NULL);
        fprintf(ofptr,"3,hand_over_hand,%i,%.3Lf\n", i+1, timeElapsed(start_time, end_time));
        fflush(ofptr);
    }

    // tidy up
    free(h_list_3);
    h_list_3 = NULL;
    free(h_args);
    h_args = NULL;

    // done
    printf("\nhand over hand tests done\n");

    fclose(ofptr); // close outputfile
}