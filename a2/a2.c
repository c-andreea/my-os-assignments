#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include "a2_helper.h"


pthread_barrier_t t7_barrier;
pthread_mutex_t t7_mutex;
pthread_cond_t t7_2_4_cond;
int t7_2_started = 0, t7_4_ended = 0;
void* t7_thread_function(void* arg) {
    int thread_num = *((int*)arg);

    pthread_mutex_lock(&t7_mutex);
    if (thread_num == 4 && !t7_2_started) {
        pthread_cond_wait(&t7_2_4_cond, &t7_mutex);
    }

    info(BEGIN, 7, thread_num);

    if (thread_num == 2) {
        t7_2_started = 1;
        pthread_cond_signal(&t7_2_4_cond);
    }
    pthread_mutex_unlock(&t7_mutex);

    if (thread_num == 2 || thread_num == 4) {
        pthread_barrier_wait(&t7_barrier);
    }

    pthread_mutex_lock(&t7_mutex);
    info(END, 7, thread_num);

    if (thread_num == 4) {
        t7_4_ended = 1;
        pthread_cond_broadcast(&t7_2_4_cond);
    } else if (thread_num == 2) {
        while (!t7_4_ended) {
            pthread_cond_wait(&t7_2_4_cond, &t7_mutex);
        }
    }
    pthread_mutex_unlock(&t7_mutex);

    return NULL;
}
void create_process_7() {
    info(BEGIN, 7, 0);

    pthread_t threads[5];
    int thread_nums[5] = {1, 2, 3, 4, 5};

    pthread_barrier_init(&t7_barrier, NULL, 2);

    for (int i = 0; i < 5; ++i) {
        pthread_create(&threads[i], NULL, t7_thread_function, &thread_nums[i]);
    }

    for (int i = 0; i < 5; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&t7_barrier);

    info(END, 7, 0);
}
void create_process_5() {
    info(BEGIN, 5, 0);
    info(END, 5, 0);
}

void create_process_4() {
    info(BEGIN, 4, 0);

    pid_t p5 = fork();
    if (p5 == 0) {
        create_process_5();
        exit(0);
    }

    wait(NULL);
    info(END, 4, 0);
}
sem_t p6_semaphore;
pthread_barrier_t p6_barrier;
pthread_mutex_t p6_mutex;
pthread_cond_t p6_10_cond;
int p6_threads_running = 0;
void* p6_thread_function(void* arg) {
    int thread_num = *((int*)arg);

    sem_wait(&p6_semaphore);
    pthread_mutex_lock(&p6_mutex);
    p6_threads_running++;
    if (thread_num == 10 && p6_threads_running == 5) {
        pthread_cond_signal(&p6_10_cond);
    } else if (thread_num == 10) {
        while (p6_threads_running != 5) {
            pthread_cond_wait(&p6_10_cond, &p6_mutex);
        }
    }
    pthread_mutex_unlock(&p6_mutex);

    info(BEGIN, 6, thread_num);
    info(END, 6, thread_num);

    pthread_mutex_lock(&p6_mutex);
    p6_threads_running--;
    if (thread_num == 10) {
        pthread_cond_broadcast(&p6_10_cond);
    }
    pthread_mutex_unlock(&p6_mutex);

    sem_post(&p6_semaphore);
    pthread_barrier_wait(&p6_barrier);

    return NULL;
}
void create_process_6() {
    info(BEGIN, 6, 0);

    pthread_t threads[42];
    int thread_nums[42];
    for (int i = 0; i < 42; ++i){
        thread_nums[i] = i + 1;
    }

    sem_init(&p6_semaphore, 0, 5);
    pthread_barrier_init(&p6_barrier, NULL, 43);

    for (int i = 0; i < 42; ++i) {
        pthread_create(&threads[i], NULL, p6_thread_function, &thread_nums[i]);
    }

    pthread_barrier_wait(&p6_barrier);

    for (int i = 0; i < 42; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&p6_barrier);
    sem_destroy(&p6_semaphore);

    // Adding process 7 as a child of process 6
    pid_t p7 = fork();
    if (p7 == 0) {
        create_process_7();
        exit(0);
    }
    wait(NULL);

    info(END, 6, 0);
}

void create_process_2() {
    info(BEGIN, 2, 0);

    pid_t p6 = fork();
    if (p6 == 0) {
        create_process_6();
        exit(0);
    }

    wait(NULL);
    info(END, 2, 0);
}

void create_process_3() {
    info(BEGIN, 3, 0);
    info(END, 3, 0);
}

void create_process_8() {
    info(BEGIN, 8, 0);
    info(END, 8, 0);
}
int main(int argc, char** argv) {
    init();

    pthread_barrier_init(&t7_barrier, NULL, 2);
    pthread_mutex_init(&t7_mutex, NULL);
    pthread_cond_init(&t7_2_4_cond, NULL);

    pthread_mutex_init(&p6_mutex, NULL);
    pthread_cond_init(&p6_10_cond, NULL);

    info(BEGIN, 1, 0);

    pid_t p2, p3, p4, p8;

    p2 = fork();
    if (p2 == 0) {
        create_process_2();
        exit(0);
    }

    p3 = fork();
    if (p3 == 0) {
        create_process_3();
        exit(0);
    }

    p4 = fork();
    if (p4 == 0) {
        create_process_4();
        exit(0);
    }

    p8 = fork();
    if (p8 == 0) {
        create_process_8();
        exit(0);
    }

    int status;
    for (int i = 0; i < 4; ++i) {
        wait(&status);
    }

    info(END, 1, 0);

    pthread_barrier_destroy(&t7_barrier);
    pthread_mutex_destroy(&t7_mutex);
    pthread_cond_destroy(&t7_2_4_cond);

    pthread_mutex_destroy(&p6_mutex);
    pthread_cond_destroy(&p6_10_cond);

    return 0;
}
