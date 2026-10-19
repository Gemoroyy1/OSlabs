#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
mkdir -p build results
{
  date -Is
  uname -a
  cat /etc/os-release
  g++ --version
  lscpu
  df -T /tmp
  lsblk -o NAME,MODEL,TYPE,SIZE,FSTYPE
} > results/environment.txt
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic aio_copy.cpp -o build/aio_copy -lrt -pthread 2>results/build.txt
for size in 1 16 64; do
  python3 benchmark.py --exe ./build/aio_copy --mib "$size" --repeats 3 --out "results/${size}MiB" > "results/${size}MiB-console.txt"
done
printf 'Build and all three benchmark suites completed successfully.\n'
