# Sorting on Encrypted Data using BFV Homomorphic Encryption

Comparison of **bitonic sort** and **rank-based sort** on BFV-encrypted integers using Microsoft SEAL 4.1.2. Each ciphertext encrypts a single integer (scalar/non-batched encoding) in the range [0, 128] with plaintext modulus p = 257.

## Algorithms

### Bitonic Sort
- Standard bitonic sorting network with oblivious compare-and-swap
- Comparison via Paterson-Stockmeyer polynomial evaluation of the sign function over Z_257
- Multiplicative depth grows with input size: O(log^2 n) rounds, each consuming ~220 bits of noise budget
- Parameters: N=32768, 40 primes x 60 bits = 2400-bit coefficient modulus

### Rank-Based Sort
- Computes each element's rank (number of smaller elements) via pairwise comparisons
- Places elements at their ranked positions using Fermat's Little Theorem-based is_zero indicator
- **Constant multiplicative depth of 18** regardless of input size
- Only works with **distinct** input values (the flag polynomial is undefined for zero differences)
- Optimized parameters: N=16384, 12 primes x 60 bits = 720-bit coefficient modulus (sufficient due to constant depth)
- Also benchmarked with N=32768 (2400-bit) for fair per-operation cost comparison with bitonic

### Parallelization (OpenMP)
Both algorithms are parallelized with OpenMP:
- **Rank sort**: Phase 1 (all n(n-1)/2 pairwise comparisons) and Phase 3 (all n position placements) are embarrassingly parallel
- **Bitonic sort**: Within each round, independent compare-and-swap pairs run in parallel, but rounds are sequential

Serial execution is achieved by setting `OMP_NUM_THREADS=1`.

## Files

| File | Description |
|------|-------------|
| `rank_sort.cpp` | Rank-based sort with N=16384, 720-bit coeff modulus |
| `bitonic_sort.cpp` | Bitonic sort with N=32768, 2400-bit coeff modulus |
| `rank_sort_32k.cpp` | Rank-based sort with N=32768, 2400-bit coeff modulus (for fair per-operation comparison with bitonic) |
| `compute_sign_poly.py` | Generates Q(y) polynomial coefficients for sign function over Z_257 |
| `CMakeLists.txt` | Build configuration |
| `benchmark_results_consolidated.csv` | Consolidated benchmark results (56 rows) |

## Prerequisites

Install Microsoft SEAL 4.1.2 from your home directory (outside of this repository):

```bash
cd ~
git clone --branch v4.1.2 https://github.com/microsoft/SEAL.git
cd SEAL
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$HOME/seal_install
cmake --build build -j8
cmake --install build
cd ~
```

## Building

Clone and build the project:

```bash
git clone https://github.com/JosephZacharyGawlik/sorting-fhe.git
cd sorting-fhe
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$HOME/seal_install
make -j8
```

This produces three binaries: `rank_parallel`, `bitonic_parallel`, `rank_32k`.

## Running

```bash
# Serial (1 thread)
OMP_NUM_THREADS=1 ./rank_parallel
OMP_NUM_THREADS=1 ./bitonic_parallel
OMP_NUM_THREADS=1 ./rank_32k

# Parallel (e.g., 8 threads)
OMP_NUM_THREADS=8 ./rank_parallel
OMP_NUM_THREADS=8 ./bitonic_parallel
OMP_NUM_THREADS=8 ./rank_32k
```

Each binary benchmarks n=2, 4, 8, 16 elements and writes results to `benchmark_results.csv`.

## Benchmark Results

All benchmarks were run on LRZ CoolMUC-4 compute nodes via SLURM batch jobs. Full results are in `benchmark_results_consolidated.csv`.
