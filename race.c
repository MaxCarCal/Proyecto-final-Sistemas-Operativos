/*
 * Modulo 2: Demo de condicion de carrera (race condition)
 *
 * Simula un contador de aforo compartido por N hilos. Cada hilo lo incrementa
 * M veces. El total esperado es N*M. Sin mutex el resultado se "pierde" por
 * la carrera; con mutex sale exacto.
 *
 * Compilar:  gcc -Wall -O2 -o race race.c -pthread
 * Ejecutar:  ./race [num_hilos] [incrementos_por_hilo]
 *            ./race                 -> defaults: 8 hilos, 1000000 incrementos
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

static int NUM_HILOS = 8;
static int INCREMENTOS = 1000000;

static volatile long contador_sin_mutex = 0;
static volatile long contador_con_mutex = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void *suma_sin_mutex(void *arg) {
    (void)arg;
    for (int i = 0; i < INCREMENTOS; i++) {
        contador_sin_mutex++;   /* lectura-modificacion-escritura no atomica */
    }
    return NULL;
}

static void *suma_con_mutex(void *arg) {
    (void)arg;
    for (int i = 0; i < INCREMENTOS; i++) {
        pthread_mutex_lock(&mtx);
        contador_con_mutex++;
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

static double correr(void *(*f)(void *)) {
    pthread_t *h = malloc(sizeof(pthread_t) * NUM_HILOS);
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < NUM_HILOS; i++) pthread_create(&h[i], NULL, f, NULL);
    for (int i = 0; i < NUM_HILOS; i++) pthread_join(h[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    free(h);
    return (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
}

int main(int argc, char **argv) {
    if (argc >= 2) NUM_HILOS = atoi(argv[1]);
    if (argc >= 3) INCREMENTOS = atoi(argv[2]);

    long esperado = (long)NUM_HILOS * INCREMENTOS;

    printf("== demo de race condition ==\n");
    printf("hilos=%d  incrementos_por_hilo=%d  esperado=%ld\n\n",
           NUM_HILOS, INCREMENTOS, esperado);

    double t_sin = correr(suma_sin_mutex);
    printf("[SIN MUTEX]  resultado = %ld   (perdidos = %ld)   tiempo = %.3f s\n",
           contador_sin_mutex, esperado - contador_sin_mutex, t_sin);

    double t_con = correr(suma_con_mutex);
    printf("[CON MUTEX]  resultado = %ld   (perdidos = %ld)   tiempo = %.3f s\n",
           contador_con_mutex, esperado - contador_con_mutex, t_con);

    /* tambien guardamos el resultado a un log */
    FILE *f = fopen("results/logs/race.log", "w");
    if (f) {
        fprintf(f, "hilos=%d  incrementos=%d  esperado=%ld\n",
                NUM_HILOS, INCREMENTOS, esperado);
        fprintf(f, "sin_mutex=%ld  perdidos=%ld  t=%.3fs\n",
                contador_sin_mutex, esperado - contador_sin_mutex, t_sin);
        fprintf(f, "con_mutex=%ld  perdidos=%ld  t=%.3fs\n",
                contador_con_mutex, esperado - contador_con_mutex, t_con);
        fclose(f);
    }

    pthread_mutex_destroy(&mtx);
    return 0;
}
