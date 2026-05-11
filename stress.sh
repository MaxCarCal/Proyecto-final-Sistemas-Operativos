#!/usr/bin/env bash
# prueba de estres: muchos hilos para ver que no truene
set -e

cd "$(dirname "$0")/.."

make -s

echo ">> stress test: 200 alumnos, 4 torniquetes, aforo 30"
./escaner 200 4 30

echo ""
echo ">> revisando que no haya logs corruptos..."
LINEAS=$(wc -l < results/logs/accesos.log)
echo "   total de lineas en log: $LINEAS"
echo "   (si fuera race condition, habria lineas mezcladas o perdidas)"
