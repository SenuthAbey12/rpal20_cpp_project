# RPAL Programming Project - C++ Implementation

This project implements a manual lexical analyzer, recursive-descent parser, AST printer, standardizer, and evaluator for the RPAL language.

## Files

- `rpal20.cpp` - main C++ source code
- `Makefile` - builds executable named `rpal20`
- `sample.rpal` - sample program from the project PDF
- `REPORT_TEMPLATE.md` - report content template

## Build

```bash
make
```

If `make` is not available on Windows, use:

```bash
mingw32-make
```

## Run

```bash
./rpal20 sample.rpal
```

Expected output:

```text
15
```

## Debug AST/ST

```bash
./rpal20 -ast sample.rpal
./rpal20 -st sample.rpal
```

## VS Code Steps

1. Open this folder in VS Code.
2. Install the C/C++ extension if not already installed.
3. Open the VS Code terminal: Terminal -> New Terminal.
4. Run `make` on Linux/macOS, or `mingw32-make` on Windows with MinGW/MSYS2.
5. Run `./rpal20 sample.rpal`.
6. For Windows PowerShell with MinGW/MSYS2, run `mingw32-make` if `make` is unavailable.

## Submission

Keep the `Makefile` directly in the root of the zip/tar folder. Do not put the files inside nested folders.
