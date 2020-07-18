#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>

struct list_s {
   int num;
   struct list_s *next;
};

struct list_s *head = NULL;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct list_s *allocate_node(int num, struct list_s *next) {
   struct list_s *node = malloc(sizeof *node);
   node->num = num;
   node->next = next;
   return node;
}

void add(int num) {
   pthread_mutex_lock(&lock);
   struct list_s **prev = &head;
   struct list_s *curr = head;
   while (curr != NULL && num > curr->num) {
      prev = &curr->next;
      curr = curr->next;
   }

   *prev = allocate_node(num, curr);
   pthread_cond_signal(&cond);
   pthread_mutex_unlock(&lock);
}

int del()
{
   pthread_mutex_lock(&lock);
   while (head == NULL) {
      pthread_cond_wait(&cond, &lock);
   }
   int num = head->num;
   head = head->next;
   pthread_mutex_unlock(&lock);
   return num;
}

uint64_t get_time_ms() {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000000ull + tv.tv_usec;
}

const int count = 40000;
const int thread_count = 4;

void populate(int seed)
{
   for (int i = 0; i < count/thread_count; i++) {
      int r = rand_r(&seed) % count;
      add(r);
   }
   // count is not a valid value, so we will use that as a sentinel that we finished
   add(count);
}

void *start_thread(void *v)
{
   populate((long)v);
    return 0;
}

int main()
{
   uint64_t start = get_time_ms();
   //uint32_t seed = rand();

   //populate(seed);
   pthread_t threads[thread_count];
   for (int i = 0; i < thread_count; i++) {
      pthread_create(&threads[i], NULL, start_thread, (void*)(long)rand());
   }

   int found = 0;
   int sentinels_seen = 0;
   while (sentinels_seen != thread_count) {
      if (del() == count) {
         sentinels_seen++;
      } else {
         found++;
      }
   }

   uint64_t  finish = get_time_ms();
   printf("took %f s\n", (double)(finish - start)/1000000.0);


   printf("found %d out of %d\n", found, count);

   return 0;
}
