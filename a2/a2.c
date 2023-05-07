#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include "a2_helper.h"


/////////////////////FUNCTII PENTRU PROCESUL 7/////////////////////////////////////////////////////////////

// pentru creerea proceselor m-am inspirat cu precadere din laboratorul 6 si din cursul 6
//mi-am facut cate o functie separata pentru a creea fiecare proces si l-am apekat in locul necesar

pthread_barrier_t t7_barrier;
pthread_mutex_t t7_mutex;
pthread_cond_t t7_2_4_cond;
int t7_2_started = 0, t7_4_ended = 0;

//in functiaasta imi creez cele 5 threaduri
void* t7_thread_function(void* arg) {
    int thread_num = *((int *) arg);

    pthread_mutex_lock(&t7_mutex);

    ///se face o verificare la mutex pentru a vedea dacă threadul curent este threadul 4 și dacă threadul 2 nu a
    ///început încă execuția. În cazul în care aceste condiții sunt îndeplinite, threadul 4 așteaptă la variabila
    /// de condiție t7_2_4_cond până când threadul 2 a fost pornit.
    if (thread_num == 4 && !t7_2_started) {
        pthread_cond_wait(&t7_2_4_cond, &t7_mutex);
    }

    info(BEGIN, 7, thread_num);

   //. Dacă acesta este threadul 2, se setează t7_2_started la 1 și se emite semnalul la variabila de condiție t7_2_4_cond
   // pentru a trezi threadul 4, în cazul în care acesta așteaptă.
    if (thread_num == 2) {
        t7_2_started = 1;
        pthread_cond_signal(&t7_2_4_cond);
    }
    pthread_mutex_unlock(&t7_mutex);

    //se verifică dacă threadul curent este threadul 2 sau 4 și, în caz afirmativ, se așteaptă la barieră.
    if (thread_num == 2 || thread_num == 4) {
        pthread_barrier_wait(&t7_barrier);
    }
  ///  După ce toate threadurile au trecut de barieră, se afișează mesajul "END"
  /// pentru threadul curent și se face o verificare suplimentară la mutex pentru a vedea dacă acesta este threadul 4.
  /// În caz afirmativ, se setează t7_4_ended la 1 și se emite semnalul de broadcast la variabila de condiție
  /// t7_2_4_cond pentru a trezi toate threadurile așteptând acolo. În caz contrar, dacă acesta este threadul 2,
  /// se așteaptă la variabila de condiție t7_2_4_cond până când threadul 4 s-a terminat și a emis semnalul de broadcast.

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

///Funcția create_process_7 este apelată în procesul principal și creează cele cinci threaduri prin intermediul
/// apelurilor la pthread_create. După aceea, se așteaptă ca toate threadurile să se termine prin apelurile la
/// pthread_join. În cele din urmă, se distruge barierea prin apelul la pthread_barrier_destroy și se afișează mesajul
/// "END" pentru procesul 7.
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

void create_process_5() {// aici e functia care imi creaza procesul 5
    info(BEGIN, 5, 0);
    info(END, 5, 0);
}

void create_process_4() {// aici e functia care imi creaza procesul 4
    info(BEGIN, 4, 0);

    pid_t p5 = fork();//aici imi creez proceul 5 care trebuir sa ii fie copil procesului 4
    if (p5 == 0) {
        create_process_5();
        exit(0);
    }

    wait(NULL);
    info(END, 4, 0);
}

/////////////////////////////////////FUNCTII PENTRU PROCESUL 6/////////////////////////////////////////////////////////

////Pentru a limita numarul de thread-uri care ruleaza simultan, s-a folosit un semafor cu o valoare initiala de
/// 5 (pentru a permite rularea a maxim 5 thread-uri in acelasi timp). Fiecare thread reprezinta un proces in sine,
/// cu un numar de la 1 la 42. Thread-urile asteapta un semnal de la semafor, apoi incep executia. La finalul executiei,
/// thread-ul incrementeaza numarul de thread-uri terminate si intra intr-o logica de sincronizare cu celelalte
/// thread-uri (pentru a garanta ca toate thread-urile au terminat executia). Daca thread-ul este cu numarul 10,
/// asteapta ca celelalte thread-uri sa termine, altfel semnalizeaza faptul ca a terminat executia.
/// Dupa ce toate thread-urile au terminat executia, se distruge semaforul si se creaza procesul 7
/// ca si copil al procesului 6.

sem_t p6_semaphore;
//pthread_barrier_t p6_barrier;
pthread_mutex_t p6_mutex;
pthread_cond_t p6_10_cond;
int p6_threads_ended = 0;
pthread_cond_t p6_all_threads_ended;
void* p6_thread_function(void* arg) {
    int thread_num = *((int *) arg);

    sem_wait(&p6_semaphore);

    info(BEGIN, 6, thread_num);
    info(END, 6, thread_num);

    pthread_mutex_lock(&p6_mutex);
    p6_threads_ended++;

    if (thread_num == 10 && p6_threads_ended < 41) {
        pthread_cond_wait(&p6_all_threads_ended, &p6_mutex);
    } else if (thread_num != 10 && p6_threads_ended == 41) {
        pthread_cond_signal(&p6_all_threads_ended);
    }

    pthread_mutex_unlock(&p6_mutex);

    sem_post(&p6_semaphore);

    return NULL;
}

void create_process_6() {// aici e functia care imi creaza procesul 6
    info(BEGIN, 6, 0);

    pthread_t threads[42];
    int thread_nums[42];
    for (int i = 0; i < 42; ++i) {
        thread_nums[i] = i + 1;
    }

    sem_init(&p6_semaphore, 0, 5);

    for (int i = 0; i < 42; ++i) {
        pthread_create(&threads[i], NULL, p6_thread_function, &thread_nums[i]);
    }

    for (int i = 0; i < 42; ++i) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&p6_semaphore);


    pid_t p7 = fork();//aici imi creez proceul 7 care trebuir sa ii fie copil procesului 6
    if (p7 == 0) {
        create_process_7();
        exit(0);
    }
    wait(NULL);

    info(END, 6, 0);
}

//////////////////////////////FUNCTII PENTRU PROCESUL 2////////////////////////////////////////////////////////////////
pthread_mutex_t p2_mutex;
pthread_cond_t p2_cond;

void* p2_thread_function(void* arg) {
    int thread_num = *((int *) arg);

    info(BEGIN, 2, thread_num);
    info(END, 2, thread_num);

    pthread_mutex_lock(&p2_mutex);
    if (thread_num == 1) {
        pthread_cond_signal(&p2_cond);
    }
    pthread_mutex_unlock(&p2_mutex);

    return NULL;
}



///aici se creează cinci fire de execuție, numite 1, 2, 3, 4 și 5, prin intermediul apelurilor pthread_create.
/// Aceste fire de execuție apelează funcția p2_thread_function, care afișează mesajul BEGIN pentru fiecare fir de
/// execuție și apoi mesajul END. În plus, firele de execuție cu numerele 2 și 3 primesc ca și argumente semaforul
/// p2_mutex și condiția p2_cond, respectiv.
void create_process_2() {// aici e functia care imi creaza procesul 2
    info(BEGIN, 2, 0);

    pthread_t threads[5];
    int thread_nums[5] = {1, 2, 3, 4, 5};

    pthread_mutex_init(&p2_mutex, NULL);
    pthread_cond_init(&p2_cond, NULL);

    for (int i = 0; i < 5; ++i) {
        pthread_create(&threads[i], NULL, p2_thread_function, &thread_nums[i]);
    }

    pthread_mutex_lock(&p2_mutex);
    pthread_cond_wait(&p2_cond, &p2_mutex);
    pthread_mutex_unlock(&p2_mutex);

    pid_t p6 = fork();//aici imi creez proceul 6 care trebuir sa ii fie copil procesului 2
    if (p6 == 0) {
        create_process_6();
        exit(0);
    }
    pthread_join(threads[0], NULL);

    pthread_mutex_destroy(&p2_mutex);
    pthread_cond_destroy(&p2_cond);

    wait(NULL);
    info(END, 2, 0);
}

/////////////////////////////////RESTUL PROCESELOR////////////////////////////////////////////////////////////////////

void create_process_3() {// aici e functia care imi creaza procesul 3
    info(BEGIN, 3, 0);
    info(END, 3, 0);
}

void create_process_8() {// aici e functia care imi creaza procesul 8
    info(BEGIN, 8, 0);
    info(END, 8, 0);
}
int main(int argc, char** argv) {
    init();

    ///Inainte de a crea celelalte procese, se initializeaza variabilele necesare pentru procesele 6 si 7,
    /// respectiv p6_semaphore, p6_mutex, p6_10_cond, t7_barrier, t7_mutex, t7_2_4_cond.

    pthread_barrier_init(&t7_barrier, NULL, 2);
    pthread_mutex_init(&t7_mutex, NULL);
    pthread_cond_init(&t7_2_4_cond, NULL);

    pthread_mutex_init(&p6_mutex, NULL);
    pthread_cond_init(&p6_10_cond, NULL);

    info(BEGIN, 1, 0);

    pid_t p2, p3, p4, p8;//in main imi creez restul proceselor care trebuie sa fie copii al procesului principal

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
///Dupa crearea proceselor, functia principala asteapta ca toate cele 4 procese copil sa se termine, prin apelul
/// functiei wait de 4 ori. Apoi se distrug variabilele si se afiseaza mesajul corespunzator sfarsitului
/// procesului principal.

    sem_destroy(&p6_semaphore);

    pthread_mutex_destroy(&p6_mutex);
    pthread_cond_destroy(&p6_10_cond);

    pthread_barrier_destroy(&t7_barrier);
    pthread_mutex_destroy(&t7_mutex);
    pthread_cond_destroy(&t7_2_4_cond);
    pthread_cond_destroy(&p6_all_threads_ended);

    info(END, 1, 0);

    return 0;
}
