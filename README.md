# cppincludeclean [![Build Status](https://travis-ci.org/krzyk240/cppincludeclean.svg?branch=master)](https://travis-ci.org/krzyk240/cppincludeclean)

__cppincludeclean__ is a simple tool for remove unnecessary #include declarations from C/C++ files.

## How it works
It comments out every #include one by one and checks if file/project still compiles
successfully (if not, uncomments back).

## How to use it
```
Usage: cppincludeclean [options] <path>...
Comment out unnecessarily #include

Options:
  --compiler COMPILER
                         Use COMPILER instead of $CXX
  -c COMMAND, --command COMMAND
                         Use shell COMMAND instead of compile command: COMPILER -c file
  -h, --help             Display this information
  -v, --verbose          Verbose mode
```
### Examples
```
cppincludeclean foo.cpp foo.h --compiler clang++
```

Comment out every unnecessary #include from files foo.cpp and foo.h using
clang++ as the compiler (to check if file compile correctly)

```
cppincludeclean src/*.c -c "make -j"
```

Comment out every unnecessary #include from C source files in src/ directory and
run command `make -j` to check if project compiles correctly
