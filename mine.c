/**
 * mine.c
 *
 * Parallelizes the hash inversion technique used by cryptocurrencies such as
 * bitcoin.
 *
 * Input:    Number of threads, block difficulty, and block contents (string)
 * Output:   Hash inversion solution (nonce) and timing statistics.
 *
 * Compile:  gcc -g -Wall mine.c -o mine
 *              (or run make)
 *           When your code is ready for performance testing, you can add the
 *           -O3 flag to enable all compiler optimizations.
 *
 // * Run:      ./mine 4 24 'Hello CS 220!!!'
 *
 * Difficulty Mask: 00000000000000000000000011111111
 * Number of threads: 4
 * . (NOTE: a '.' appears every 1,000,000 hashes to track progress)
 * Solution found by thread 1:
 * Nonce: 1011686
 * Hash: 000000B976A3E2B94CB9AB668E0C9C727782787B
 * 1016000 hashes in 0.26s (3960056.52 hashes/sec)
 */

#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "sha1.c"

#define NONCES_PER_TASK 120

pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task_staging = PTHREAD_COND_INITIALIZER;
pthread_cond_t task_ready = PTHREAD_COND_INITIALIZER;

uint32_t difficulty_mask;
char *bitcoin_block_data;
uint64_t *task_pointer = NULL;
bool solution_found = false;

// used to allow us to associate more attributes per thread
struct thread_info {
    // actual thread
    pthread_t thread_handle;
    unsigned int thread_id;
    uint64_t num_inversions;
    uint64_t nonce;
    char solution_hash[41];
};

/** Function Prototypes */
double get_time();
void *mine(void *arg);
void print_binary32(uint32_t num);
uint32_t get_difficulty(int diff);
void print_results(struct thread_info **threads, int num_threads, double total_time);

int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Usage: %s threads difficulty 'block data (string)'\n", argv[0]);
        return EXIT_FAILURE;
    }

    difficulty_mask = get_difficulty(atoi(argv[2]));

    printf("\nDifficulty Mask: ");
    print_binary32(difficulty_mask);
    printf("\n");

    bitcoin_block_data = argv[3];

    /* Check to make sure the user entered a valid (non-empty) string */
    if(strcmp(bitcoin_block_data, "") == 0){
      printf("ERROR: The string passed as the block data is empty.\n");
      return EXIT_FAILURE;
    }

    unsigned int num_threads = 5;
    if(atoi(argv[1]) < 1)
      printf("ERROR: Invalid number of threads, defaulting to 5\n");
    else
      num_threads = atoi(argv[1]);

    struct thread_info *threads[num_threads];
    int i;
    for(i = 0; i < num_threads; i++){
      threads[i] = calloc(1, sizeof(struct thread_info));
      threads[i]->thread_id = i;
      pthread_create(&(threads[i]->thread_handle), NULL, mine, threads[i]);
    }

    double start_time = get_time();

    uint64_t current_nonce = 0;
    while (current_nonce < UINT64_MAX) {

        uint64_t *nonces = malloc(sizeof(uint64_t) * NONCES_PER_TASK);
        int i;
        for (i = 0; i < NONCES_PER_TASK; ++i) {
            nonces[i] = current_nonce++; //initializes nonces and increments current_nonce after

            if (current_nonce % 1000000 == 0) {
                /* Print out '.' to show progress every 1m hashes: */
                printf(".");
                fflush(stdout);
            }
        }

        /* Nonces are ready to be consumed. Will wait for a consumer thread
         * to pick up the job. */
        pthread_mutex_lock(&task_mutex);
        while (task_pointer != NULL && solution_found == false)
            pthread_cond_wait(&task_staging, &task_mutex);

        if (solution_found == true)
            break;

        /* We have acquired a mutex on task_mutex. We can now update the pointer
         * to point to the new task we just generated */
        task_pointer = nonces;

        /* Tell the consumer a new task is ready */
        pthread_cond_signal(&task_ready);

        /* Let a consumer have the mutex to get the task */
        pthread_mutex_unlock(&task_mutex);
    } //while current_nonce < UINT64_MAX

    printf("\n");

    /* If we reach this point, one of the threads found a solution. We will
     * signal any waiting worker threads so they will wake up and see
     * that solution_found is true */
    pthread_cond_broadcast(&task_ready);

    /* Since the loop will break before unlocking the task_mutex
     * we have to call it here */
    pthread_mutex_unlock(&task_mutex);

    double end_time = get_time();

    /* Tell all of the threads to stop */
    for(i = 0; i < num_threads; i++)
      pthread_join(threads[i]->thread_handle, NULL);

    print_results(threads, num_threads, end_time - start_time);

    return 0;
}

/*
 * retrieves the current system time (in seconds).
 */
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* Function: print_binary32
 * ------------------------
 * prints a 32 bit number in binary using bit manipulation
 *
 * num: 32 bit int to print
*/
void print_binary32(uint32_t num) {
    int i;
    for (i = 31; i >= 0; --i) {
        uint32_t position = 1 << i;
        printf("%c", ((num & position) == position) ? '1' : '0');
    }
}

/* Function: get_difficulty
 * ------------------------
 * Returns the number of bits based off int
 *
 * diff: int used to set mask
 *
 * returns: 32 bit mask with diff amount of leading zeros
*/
uint32_t get_difficulty(int diff){
  uint32_t mask = 0;

  int i;
  for(i = 0; i < 32 - diff; i++){
    uint32_t position = 1 << i;
    mask = mask | position;
  }

  return mask;
}

/* Function: mine
 * --------------
 *
 * Uses mutex and condition waiting to communicate to producer and other threads.
 * Mines for a solution based off bitcoin_block_data and difficulty_mask.
 *
 * arg: thread to create
 */
void *mine(void *arg) {

    struct thread_info *info = (struct thread_info *) arg;
    uint64_t *task_nonces = NULL;

    /* We'll keep on working until a solution for our bitcoin block is found */
    while (true) {

      pthread_mutex_lock(&task_mutex);
      while (task_pointer == NULL && solution_found == false) {
        //printf("thread %d is waiting for consumer\n", info->thread_id);
        pthread_cond_wait(&task_ready, &task_mutex);
      }

        if (solution_found) {
          //printf("Thread %d sees that a solution has been found\n", info->thread_id);
          pthread_cond_signal(&task_staging);
          pthread_mutex_unlock(&task_mutex);

            return NULL;
        }

        /* Copy over task */
        task_nonces = task_pointer;

        /* Empty out our task_pointer so another thread can receive a task. */
        task_pointer = NULL;

        /* let main know to stage a new task */
        pthread_cond_signal(&task_staging);
        pthread_mutex_unlock(&task_mutex);

        int i;
        for (i = 0; i < NONCES_PER_TASK; ++i) {
            /* Convert the nonce to a string */
            char buf[21];
            sprintf(buf, "%llu", task_nonces[i]);

            /* Create a new string by concatenating the block and nonce string.
             * For example, if we have 'Hello World!' and '10', the new string
             * is: 'Hello World!10' */
            size_t str_size = strlen(buf) + strlen(bitcoin_block_data);
            char *tmp_str = malloc(sizeof(char) * str_size + 1);
            strcpy(tmp_str, bitcoin_block_data);
            strcat(tmp_str, buf);

            /* Hash the temporary (combined) string */
            uint8_t digest[20];
            sha1sum(digest, tmp_str);

            /* Clean up the temporary string */
            free(tmp_str);

            /* Take the front 32 bits of the hash (spit across four 8-bit
             * unsigned integers and combine them */
            uint32_t hash_front = 0;
            hash_front |= digest[0] << 24;
            hash_front |= digest[1] << 16;
            hash_front |= digest[2] << 8;
            hash_front |= digest[3];

            /* Check to see if we've found a solution to our block. We perform a
             * bitwise AND operation and check to see if we get the same result
             * back. */
            if ((hash_front & difficulty_mask) == hash_front) {
                solution_found = true;
                info->nonce = task_nonces[i];
                sha1tostring(info->solution_hash, digest);

                // To wake up main from waiting
                pthread_cond_signal(&task_staging);
                return NULL;
            }
        }

        free(task_nonces);
        info->num_inversions += NONCES_PER_TASK;
    }

    return NULL;
}

void print_results(struct thread_info **threads, int num_threads, double total_time){
  uint64_t total_inversions = 0;

  int i;
  for(i = 0; i < num_threads; i++){
    if(strlen(threads[i]->solution_hash) > 0){
      printf("Solution found by thread %d:\n", threads[i]->thread_id);
      printf("Nonce: %llu\n", threads[i]->nonce);
      printf("Hash: %s\n", threads[i]->solution_hash);
    }
    total_inversions += threads[i]->num_inversions;
  }

  printf("%llu hashes in %.2fs (%.2f hashes/sec)\n",
          total_inversions, total_time, total_inversions / total_time);
}
