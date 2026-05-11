#!/usr/bin/env bash
# corre los dos modulos en secuencia y deja todo listo para revisar
set -e

cd "$(dirname "$0")/.."

echo ">> compilando..."
make -s

echo ""
echo ">> modulo 1: escaner de entrada"
./escaner 50 4 20

echo ""
echo ">> modulo 2: demo de race condition"
./race 8 1000000

echo ""
echo ">> listo. revisa results/logs/"
