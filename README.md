# Cynide

Cynide (.cy) is a AoT - programming language compiler being developed in C++ using LLVM as its backend.

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
make
```

## Usage

Run the compiled executable with a `.cy` source file:

```bash
./cynide <input.cy> [options]
```

## License

See the [LICENSE](LICENSE) file for more details.
