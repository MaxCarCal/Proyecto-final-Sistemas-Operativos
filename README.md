# Sistema de Escaner de Entrada a la Uni

Proyecto Final de Sistemas Operativos — Primavera 2026  
**Equipo:** Juan Diego Acosta, Maximiliano Carmona, Patricio Kaim  
**Universidad de las Américas Puebla**



## Descripción

Simulación del control de acceso a una universidad en hora pico. Modela a N
alumnos (hilos POSIX) compitiendo por K torniquetes (semáforos), con aforo
máximo del campus controlado por una variable de condición. El log de accesos
se protege con mutex para evitar corrupción por escritura concurrente.

Hay dos módulos independientes:

- **`escaner`** — simulación principal: alumnos compitiendo por torniquetes,
  con aforo máximo controlado por variable de condición.
- **`race`** — demo didáctica de condición de carrera: suma un contador desde
  N hilos, primero sin sincronización (resultado incorrecto y no determinista)
  y luego con mutex (resultado exacto siempre).


## Dependencias

### Arch Linux / CachyOS
```bash
sudo pacman -S --needed base-devel
```

### Ubuntu / Debian
```bash
sudo apt update && sudo apt install build-essential
```

### Fedora / RHEL
```bash
sudo dnf install gcc make
```

Requisitos mínimos: `gcc`, `make`, cabeceras de pthreads (incluidas en
`glibc-devel` / `libc6-dev`). No se requieren librerías externas.


## Compilación

Desde la raíz del proyecto:

```bash
make
```

Esto genera dos binarios: `escaner` y `race`.

Para limpiar los binarios:

```bash
make clean
```

> El módulo `race` se compila con `-O0` a propósito para que la condición de
> carrera sea observable. Con `-O2` el compilador puede eliminar la carrera.



## Ejecución

### Módulo 1 — Escaner

```bash
./escaner                    # defaults: 50 alumnos, 4 torniquetes, aforo 20
./escaner [alumnos] [torniquetes] [aforo_max]
./escaner 100 2 15           # más cuello de botella
```

### Módulo 2 — Race condition

```bash
./race                       # defaults: 8 hilos, 1 000 000 incrementos
./race [num_hilos] [incrementos_por_hilo]
./race 16 5000000            # más hilos, race más visible
```

### Correr todo de un jalón

```bash
./scripts/run_all.sh         # compila y ejecuta ambos módulos en secuencia
```

### Test de estrés

```bash
./scripts/stress.sh          # 200 alumnos, verifica que no haya corrupción en el log
```



## Resultados esperados

### Módulo 1 (escaner)

Salida en consola:

```
simulacion ok: 50 alumnos, 4 torniquetes, aforo=20
tiempo total: 0.495 s
log -> results/logs/accesos.log
```

El log `results/logs/accesos.log` registra cada evento con timestamp de
milisegundos:

```
[18:53:36.412] alumno=22  torniquete=2  escanea credencial  (aforo=2)
[18:53:36.412] alumno=22  torniquete=2  ENTRA al campus     (aforo=3)
[18:53:36.697] alumno=22  torniquete=2  sale del campus     (aforo=2)
```

Cuando el campus está lleno aparecen líneas de `espera (aforo lleno)` — ahí
entra en juego la variable de condición.

### Módulo 2 (race condition)

```
[SIN MUTEX]  resultado = 2458813   (perdidos = 5541187)   tiempo = 0.022 s
[CON MUTEX]  resultado = 8000000   (perdidos = 0)         tiempo = 0.171 s
```

El número "perdidos" varía en cada ejecución (depende del scheduler del
kernel) — eso es exactamente la condición de carrera. Con mutex el resultado
siempre es exacto y el tiempo ~7× mayor por el overhead de sincronización.



## Estructura del repositorio

```
proyecto-so/
├── src/
│   ├── escaner.c          # módulo 1: simulación principal
│   └── race.c             # módulo 2: demo de race condition
├── scripts/
│   ├── run_all.sh         # ejecuta ambos módulos en secuencia
│   └── stress.sh          # prueba de estrés con 200 alumnos
├── results/
│   ├── logs/
│   │   ├── accesos.log    # log de ejemplo del módulo 1
│   │   └── race.log       # log de ejemplo del módulo 2
│   ├── tables/
│   │   └── resultados_race.txt   # tabla comparativa de corridas
│   └── screenshots/       # capturas de terminal de cada módulo
├── docs/
│   └── reporte.pdf        # reporte técnico (6 páginas)
├── Makefile
├── README.md
└── .gitignore
```



## Conceptos de Sistemas Operativos demostrados

 Concepto  Dónde se usa 

 Hilos POSIX (`pthread_create/join`) Ambos módulos — cada alumno/hilo es un `pthread` |
 Semáforo contador (`sem_wait/post`)  `escaner` — limita cuántos alumnos pasan al torniquete simultáneamente |
 Mutex (`pthread_mutex_lock/unlock`)  Ambos módulos — protege el log y el contador de aforo |
 Variable de condición (`pthread_cond_wait/signal`) | `escaner` — bloquea alumnos cuando el campus está lleno |
 Condición de carrera  `race` — demostrada antes/después de agregar el mutex |
 Sección crítica  Escritura al log y actualización del contador `aforo_actual` |
 Orden de adquisición de locks  Siempre: `sem_wait` torniquete → mutex aforo (evita deadlock) |


## Notas técnicas

- Probado en CachyOS (kernel 6.x), compatible con cualquier Linux con gcc y pthreads.
- Los timestamps usan `clock_gettime(CLOCK_REALTIME)` con resolución de milisegundos.
- El contador de `race` es `volatile` para evitar que el compilador lo optimice.

## Limitaciones y trabajo futuro

- Es una simulación; no se conecta a hardware real ni a credenciales físicas.
- El log no tiene rotación de archivos.
- Credenciales inválidas no están modeladas (trabajo futuro).
