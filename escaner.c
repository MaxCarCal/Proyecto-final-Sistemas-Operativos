/*
 * Modulo 1: Escaner de entrada a la uni
 * Simula N alumnos (hilos) compitiendo por K torniquetes (semaforos).
 * El log de accesos se protege con mutex. El aforo del campus se controla
 * con una variable de condicion.
 *
 * Compilar:  gcc -Wall -O2 -o escaner escaner.c -pthread
 * Ejecutar:  ./escaner [num_alumnos] [num_torniquetes] [aforo_max]
 *            ./escaner                -> defaults: 50 alumnos, 4 torniquetes, 20 aforo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#define LOG_PATH "results/logs/accesos.log"

static int  NUM_ALUMNOS    = 50;
static int  NUM_TORNIQUETES = 4;
static int  AFORO_MAX      = 20;

static sem_t        sem_torniquetes;     /* limita cuantos pasan a la vez */
static pthread_mutex_t mtx_log = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_aforo = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cv_aforo  = PTHREAD_COND_INITIALIZER;
static int aforo_actual = 0;

static FILE *flog = NULL;

/* timestamp simple HH:MM:SS.mmm para los logs */
static void ts(char *buf, size_t n) {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    struct tm lt;
    localtime_r(&t.tv_sec, &lt);
    snprintf(buf, n, "%02d:%02d:%02d.%03ld",
             lt.tm_hour, lt.tm_min, lt.tm_sec, t.tv_nsec / 1000000);
}

/* escribir en el log -> seccion critica protegida con mutex */
static void log_acceso(int id_alumno, int id_torniquete, const char *evento) {
    char t[32];
    ts(t, sizeof t);
    pthread_mutex_lock(&mtx_log);
    fprintf(flog, "[%s] alumno=%-3d torniquete=%d  %s  (aforo=%d)\n",
            t, id_alumno, id_torniquete, evento, aforo_actual);
    fflush(flog);
    pthread_mutex_unlock(&mtx_log);
}

typedef struct {
    int id;
} alumno_t;

static void *alumno_func(void *arg) {
    alumno_t *a = (alumno_t *)arg;

    /* simula que cada alumno llega en un instante distinto */
    usleep((rand() % 200) * 1000);

    char t[32]; ts(t, sizeof t);
    pthread_mutex_lock(&mtx_log);
    fprintf(flog, "[%s] alumno=%-3d            llega a la cola\n", t, a->id);
    fflush(flog);
    pthread_mutex_unlock(&mtx_log);

    /* espera torniquete libre */
    sem_wait(&sem_torniquetes);

    int torniquete = (a->id % NUM_TORNIQUETES) + 1;

    log_acceso(a->id, torniquete, "escanea credencial");

    /* checa aforo: si esta lleno, espera */
    pthread_mutex_lock(&mtx_aforo);
    while (aforo_actual >= AFORO_MAX) {
        log_acceso(a->id, torniquete, "espera (aforo lleno)");
        pthread_cond_wait(&cv_aforo, &mtx_aforo);
    }
    aforo_actual++;
    pthread_mutex_unlock(&mtx_aforo);

    log_acceso(a->id, torniquete, "ENTRA al campus");

    /* libera el torniquete cuanto antes para que pase el siguiente */
    sem_post(&sem_torniquetes);

    /* el alumno anda en el campus */
    usleep((50 + rand() % 150) * 1000);

    /* se sale del campus, libera espacio */
    pthread_mutex_lock(&mtx_aforo);
    aforo_actual--;
    pthread_cond_signal(&cv_aforo);
    pthread_mutex_unlock(&mtx_aforo);

    log_acceso(a->id, torniquete, "sale del campus");

    free(a);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc >= 2) NUM_ALUMNOS    = atoi(argv[1]);
    if (argc >= 3) NUM_TORNIQUETES = atoi(argv[2]);
    if (argc >= 4) AFORO_MAX      = atoi(argv[3]);

    if (NUM_ALUMNOS <= 0 || NUM_TORNIQUETES <= 0 || AFORO_MAX <= 0) {
        fprintf(stderr, "parametros invalidos\n");
        return 1;
    }

    srand((unsigned)time(NULL));

    flog = fopen(LOG_PATH, "w");
    if (!flog) {
        fprintf(stderr, "no se pudo abrir %s: %s\n", LOG_PATH, strerror(errno));
        return 1;
    }

    fprintf(flog, "=== simulacion: %d alumnos, %d torniquetes, aforo max=%d ===\n",
            NUM_ALUMNOS, NUM_TORNIQUETES, AFORO_MAX);
    fflush(flog);

    sem_init(&sem_torniquetes, 0, NUM_TORNIQUETES);

    pthread_t *hilos = malloc(sizeof(pthread_t) * NUM_ALUMNOS);
    if (!hilos) { fclose(flog); return 1; }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < NUM_ALUMNOS; i++) {
        alumno_t *a = malloc(sizeof(alumno_t));
        a->id = i + 1;
        if (pthread_create(&hilos[i], NULL, alumno_func, a) != 0) {
            fprintf(stderr, "fallo pthread_create en alumno %d\n", i + 1);
            free(a);
        }
    }

    for (int i = 0; i < NUM_ALUMNOS; i++) {
        pthread_join(hilos[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double seg = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    fprintf(flog, "=== fin: %d alumnos procesados en %.3f s ===\n",
            NUM_ALUMNOS, seg);
    fclose(flog);

    printf("simulacion ok: %d alumnos, %d torniquetes, aforo=%d\n",
           NUM_ALUMNOS, NUM_TORNIQUETES, AFORO_MAX);
    printf("tiempo total: %.3f s\n", seg);
    printf("log -> %s\n", LOG_PATH);

    free(hilos);
    sem_destroy(&sem_torniquetes);
    pthread_mutex_destroy(&mtx_log);
    pthread_mutex_destroy(&mtx_aforo);
    pthread_cond_destroy(&cv_aforo);
    return 0;
}
