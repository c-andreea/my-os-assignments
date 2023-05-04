#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include "a2_helper.h"

pthread_mutex_t t7_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t7_cond = PTHREAD_COND_INITIALIZER;
int t7_2_started = 0;
int t7_4_ended = 0;

void* t7_thread_function(void* arg) {
    int thread_num = *((int*)arg);

    if (thread_num == 2) {
        pthread_mutex_lock(&t7_mutex);
        t7_2_started = 1;
        pthread_cond_signal(&t7_cond);
        pthread_mutex_unlock(&t7_mutex);
    }

    if (thread_num == 4) {
        pthread_mutex_lock(&t7_mutex);
        while (!t7_2_started) {
            pthread_cond_wait(&t7_cond, &t7_mutex);
        }
        pthread_mutex_unlock(&t7_mutex);
    }

    info(BEGIN, 7, thread_num);
    info(END, 7, thread_num);

    if (thread_num == 4) {
        pthread_mutex_lock(&t7_mutex);
        t7_4_ended = 1;
        pthread_cond_signal(&t7_cond);
        pthread_mutex_unlock(&t7_mutex);
    }

    return NULL;
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

void create_process_7() {
    info(BEGIN, 7, 0);

    pthread_t threads[5];
    int thread_nums[5] = {1, 2, 3, 4, 5};

    for (int i = 0; i < 5; ++i) {
        pthread_create(&threads[i], NULL, t7_thread_function, &thread_nums[i]);
    }

    pthread_mutex_lock(&t7_mutex);
    while (!t7_4_ended) {
        pthread_cond_wait(&t7_cond, &t7_mutex);
    }
    pthread_mutex_unlock(&t7_mutex);

    for (int i = 0; i < 5; ++i) {
        pthread_join(threads[i], NULL);
    }

    info(END, 7, 0);
}

void create_process_6() {
    info(BEGIN, 6, 0);

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

    init();

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

    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    info(END, 1, 0);

    return 0;
}
