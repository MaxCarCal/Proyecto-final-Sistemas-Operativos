# Sistema de Escaner de Entrada a la Uni

Proyecto final de Sistemas Operativos — Primavera 2026
**Equipo:** Juan Diego Acosta, Maximiliano Carmona, Patricio Kaim

## Qué es esto

Una simulación del control de acceso a la uni en hora pico. Llegan un montón de
alumnos al mismo tiempo y solo hay unos pocos torniquetes, entonces se forma
cuello de botella. El programa modela eso con hilos (cada alumno es un hilo) y
semáforos (los torniquetes son el recurso limitado). El log de accesos lo
protegemos con mutex para que dos hilos no escriban al mismo tiempo y se
corrompa el archivo.

Hay dos módulos:

- **`escaner`** — la simulación principal: alumnos compitiendo por torniquetes,
  con aforo máximo controlado por una variable de condición.
- **`race`** — una demo aparte que enseña *por qué* hace falta el mutex. Hace lo
  mismo dos veces (sumar un contador desde varios hilos) una vez sin
  sincronización y otra con mutex, y se ve clarísimo cómo en la primera se
  pierden incrementos.

## Cómo correrlo

Usamos `gcc` y `make`.

```bash
sudo pacman -S --needed base-devel
```

Después, desde la raíz del proyecto:

```bash
make                # compila los dos módulos
./escaner           # corre con defaults: 50 alumnos, 4 torniquetes, aforo 20
./race              # corre con defaults: 8 hilos, 1M incrementos cada uno
```

## Parámetros

Los dos programas aceptan argumentos por línea de comandos:

```bash
./escaner [num_alumnos] [num_torniquetes] [aforo_max]
./race    [num_hilos]   [incrementos_por_hilo]
```

Ejemplos:

```bash
./escaner 100 2 15      # 100 alumnos, 2 torniquetes, aforo 15 (más cuello de botella)
./race 16 5000000       # 16 hilos sumando 5M veces (más race visible)
```

Para el test de estrés (200 alumnos):

```bash
./scripts/stress.sh
```

## Resultados esperados

### Módulo 1 (escaner)

Sale algo así en consola:

```
simulacion ok: 50 alumnos, 4 torniquetes, aforo=20
tiempo total: 0.495 s
log -> results/logs/accesos.log
```

Y el log queda en `results/logs/accesos.log` con cada evento timestamped:

```
[18:53:36.412] alumno=22  torniquete=2  escanea credencial  (aforo=2)
[18:53:36.412] alumno=22  torniquete=2  ENTRA al campus  (aforo=3)
[18:53:36.697] alumno=22  torniquete=2  sale del campus  (aforo=2)
```

Si el aforo se llena, vas a ver líneas de `espera (aforo lleno)` antes de que
el alumno entre — esa es la variable de condición trabajando.

### Módulo 2 (race)

```
[SIN MUTEX]  resultado = 2458813   (perdidos = 5541187)   tiempo = 0.022 s
[CON MUTEX]  resultado = 8000000   (perdidos = 0)         tiempo = 0.171 s
```

El número "perdidos" cambia cada vez que lo corres (porque depende del scheduler
del kernel), y eso *es justamente* la condición de carrera. Con mutex siempre
sale 8000000 exacto.

## Estructura

```
proyecto-so/
├── src/
│   ├── escaner.c        # módulo 1: simulación principal
│   └── race.c           # módulo 2: demo de race condition
├── scripts/
│   ├── run_all.sh       # corre todo de un jalón
│   └── stress.sh        # 200 alumnos para la demo
├── results/
│   ├── logs/            # logs de cada corrida
│   ├── tables/          # tabla con números de las pruebas
│   └── screenshots/     # capturas para el reporte
├── Makefile
├── README.md
└── reporte.pdf          # reporte técnico de 6 páginas
```

## Conceptos de SO que cubre

- **Hilos POSIX** (`pthread_create`, `pthread_join`)
- **Semáforos** (`sem_init`, `sem_wait`, `sem_post`) — para limitar el número de
  alumnos pasando al mismo tiempo
- **Mutex** (`pthread_mutex_lock/unlock`) — para proteger la escritura al log y
  el contador de aforo
- **Variable de condición** (`pthread_cond_wait`, `pthread_cond_signal`) — para
  que los alumnos esperen cuando el campus está lleno
- **Race condition** demostrada antes/después de poner el mutex
- **Sección crítica** identificada y protegida (escritura al log, contador
  compartido)

## Problemas que resuelve y cómo

Problema | Solución 

- Race condition en el log: Mutex (`mtx_log`) en cada `fprintf`. 
- Cuello de botella en torniquetes: Semáforo contador inicializado en K. 
- Aforo máximo del campus: Variable de condición + contador `aforo_actual`. 
- Deadlock | Orden fijo: primero `sem_wait` torniquete, luego mutex aforo, nunca al revés.

## Notas
 
- Probado en CachyOS (kernel 6.x), pero corre en cualquier Linux con gcc y
  pthreads.
- El módulo `race` se compila a propósito con `-O0` (sin optimización) y el
  contador es `volatile`. Si dejas `-O2` el compilador puede meter la suma
  entera a un registro y la condición de carrera "desaparece" — lo cual es un
  resultado interesante de por sí, pero lo que queremos demostrar es el caso
  del libro de texto.
- Los timestamps en el log usan `clock_gettime(CLOCK_REALTIME)` así que tienen
  resolución de milisegundos.

## Limitaciones

- Es una simulación, no se conecta a hardware real ni a una credencial.
- El log no tiene rotación, si lo dejas correr horas crece sin parar.
- No manejamos credenciales inválidas más allá de mencionarlo en la propuesta
  (lo dejamos para "trabajo futuro").
