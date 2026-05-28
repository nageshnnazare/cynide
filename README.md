# Cynide


Cynide (.cy) is an AoT programming language compiler being developed in C++ using LLVM as its backend. `cylang` is the compiler for Cynide.

## Documentation

The API reference and class documentation are automatically generated using Doxygen and hosted via GitHub Pages.

👉 **[View Doxygen API Documentation](https://nageshnnazare.github.io/cynide/)**

[![Documentation Status](https://github.com/nageshnnazare/cynide/actions/workflows/doxygen.yml/badge.svg)](https://github.com/nageshnnazare/cynide/actions/workflows/doxygen.yml)

## Prerequisites

*   C++17 Compiler (e.g., GCC, Clang)
*   CMake (>= 3.16)
*   LLVM (configured and accessible by CMake)

## Building the Compiler

Cynide uses CMake as its build system. You can build it using the following steps:

```bash
mkdir build
cd build
cmake ..
make cylang
```

## Usage

Run the compiled executable with a `.cy` source file:

```bash
cylang <input.cy> [options]
```

## License

See the [LICENSE](LICENSE) file for more details.
