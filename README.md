# Image Filter Optimization

A performance optimization project in C++ that improves the throughput of an image convolution filter pipeline by applying low-level code optimization techniques. Starting from a naive five-nested-loop baseline, the goal was to reduce cycles-per-pixel (CPE) on a benchmark image processing workload by targeting cache behavior, loop overhead, and arithmetic cost.

---

## Overview

Image filters work by applying a convolution operation to every pixel in an image — multiplying a small matrix (the filter) against the surrounding pixels to produce effects like blurring, edge detection, and embossing. The baseline implementation uses five nested loops and is computationally expensive at scale.

This project optimizes that pipeline using techniques from computer architecture and systems programming, measured using hardware cycle counters (`rdtscll`) for precise performance benchmarking.

---

## Optimization Techniques Applied

### 1. Hoisting Constants Out of Loops
Pre-computed values like `input->width`, `input->height`, `filter->getSize()`, and `filter->getDivisor()` were initialized as `const` variables before the loop structure. This eliminates repeated pointer dereferencing and function calls that would otherwise execute millions of times inside the hot path.

### 2. Accumulating Into a Local Sum Variable
Instead of writing directly to `output->color[plane][row][col]` on every inner iteration, results accumulate into a local `sum` variable. This allows the compiler to keep `sum` in a register rather than repeatedly reading and writing to memory, and reduces the total number of array dereferences from O(n²) per pixel to one write per pixel.

### 3. Row Pointer Caching
Rather than recomputing `input->color[plane][row+i-1]` on every inner loop iteration, row pointers (`r0`, `r1`, `r2` for the 3x3 case, `inRow` for the general case) are computed once per outer iteration. This replaces repeated pointer chasing and address arithmetic with simple base + offset indexing inside the hot loop.

### 4. Coefficient Caching
Filter coefficients (`filter->get(i, j)`) are loaded into a flat `coeff[]` array before the column loop. This eliminates repeated virtual function calls and pointer indirection from the innermost loop, and gives the compiler the opportunity to place these values in registers.

### 5. Loop Reordering for Cache Locality
The loop order was restructured from `col → row → plane` to `row → plane → col` to match the memory layout of the pixel array (`color[plane][row][col]`). Since `col` varies fastest in memory, making it the innermost loop produces sequential (linear) memory access — maximizing spatial locality and dramatically reducing cache misses.

### 6. Full Loop Unrolling for 3×3 Filters (Special Case)
For the common case of 3×3 filters, the inner two filter loops are completely unrolled into nine explicit multiply-add operations using pre-hoisted constants (`f00` through `f22`). This eliminates:
- Loop overhead (branch checking, iteration incrementing)
- Repeated `filter->get()` calls
- Load effective address (LEA) arithmetic on every tap

The compiler can now treat the entire convolution as straight-line arithmetic, assigning register values to each coefficient and row pointer for maximum throughput.

### 7. Output Row Pointer Caching
An `outRow` pointer is initialized before the innermost loop so the final result write is a simple indexed store (`outRow[col] = sum`) rather than a full multi-level array dereference on every iteration.

### 8. Division Optimization
Integer division is the most expensive arithmetic operation in the hot loop. Three cases replace the naive `sum /= filterDiv`:
- **Divisor == 1:** Skip division entirely
- **Divisor is a power of 2:** Use arithmetic right shift (`>> __builtin_ctz(filterDiv)`) instead of division — significantly cheaper on all hardware
- **Otherwise:** Fall back to standard integer division

### 9. Compiler Optimization Flags
The Makefile was updated to use `-O3 -march=native -mtune=native -fomit-frame-pointer -funroll-loops` which enables aggressive compiler auto-vectorization, native instruction set tuning, and additional loop optimizations on top of the manual changes.

---

## Performance Results

| Implementation | Median CPE (cycles/pixel) |
|---|---|
| Baseline | ~4000 |
| Optimized | ~[your result] |
| Speedup | ~[your speedup]x |

*Benchmarked on `blocks-small.bmp` using Gaussian blur, average, horizontal line detection, and emboss filters. Median CPE computed across 6 runs per filter via the Judge script.*

---

## Build & Run

```bash
# Standard build
make

# Optimized build (matches grading configuration)
make CXXFLAGS="-O3 -march=native -mtune=native -fomit-frame-pointer -funroll-loops -DNDEBUG"

# Run a single filter on an image
./filter hline.filter boats.bmp

# Run multiple times for average CPE
./filter hline.filter boats.bmp boats.bmp

# Run full benchmark suite
make judge

# Verify correctness against reference output
make test

# Clean build artifacts
make clean
```

---

## Project Structure
├── FilterMain.cpp            # Entry point, filter application loop, optimization log
├── Filter.h / Filter.cpp     # Filter class — matrix representation and accessors
├── cs1300bmp.h / cs1300bmp.cpp   # BMP image read/write and pixel data structures
├── rdtsc.h                   # Hardware cycle counter instrumentation
├── Makefile                  # Build system with judge and test targets
└── *.filter                  # Convolution filter definitions (gauss, hline, emboss, avg)



---

## Key Concepts

**Convolution** applies a filter matrix to an image by computing a weighted sum of each pixel's neighborhood. For an n×n filter, this requires O(n²) multiplications per pixel — making the inner loop extremely hot and a high-value optimization target.

**Cycles per pixel (CPE)** is the performance metric — the average number of CPU clock cycles required to process one pixel. Lower is better. Hardware cycle counters (`rdtscll`) give precise, repeatable measurements independent of wall-clock time.

**Cache locality** is the primary bottleneck in the baseline. The original loop order caused frequent cache misses by accessing the 3D pixel array out of order. Restructuring traversal to match memory layout (spatial locality) and reusing recently accessed values (temporal locality) drives the largest performance gains.

**Register allocation** becomes possible when the compiler can see that a value is constant and heavily reused within a tight loop. Hoisting coefficients and row pointers out of inner loops gives the compiler the information it needs to assign those values to registers rather than re-loading from memory on every iteration.

---

## Skills Demonstrated

- C++ performance engineering and low-level optimization
- CPU architecture: cache hierarchy, spatial and temporal locality, register allocation
- Instruction-level optimization: loop unrolling, strength reduction, branch elimination
- Hardware performance measurement using cycle counters
- Profiling-driven development — measuring and explaining every optimization
- Systems-level debugging and benchmarking on Linux
