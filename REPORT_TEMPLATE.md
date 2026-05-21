# CS 3513 Programming Languages - RPAL Project Report

## Student Details

| Student | Name | Student ID |
|---|---|---|
| 1 | Your Name | Your ID |
| 2 | Partner Name | Partner ID |

## Introduction

This project implements a lexical analyzer, parser, AST generator, Standardized Tree transformer, and evaluator for RPAL using C++ without lex/yacc tools.

## Program Structure

### Main Components

1. `Lexer` - reads the RPAL source program and produces tokens.
2. `Parser` - recursive-descent parser following the RPAL grammar.
3. `Node` - tree node structure used for AST and ST.
4. `standardize()` - converts AST forms such as `let`, `where`, `within`, `and`, `rec`, and `fcn_form` into standardized forms.
5. `eval()` - evaluates the parsed RPAL program using environments and closures.

## Important Function Prototypes

```cpp
vector<Token> Lexer::scan();
N Parser::parse();
N standardize(const N &x);
void printTree(const N &n, int depth = 0);
Value eval(N x, EnvPtr env);
unordered_map<string, Value> evalDef(N d, EnvPtr env);
Value apply(Value f, Value arg);
```

## Lexical Analyzer

The lexical analyzer recognizes identifiers, integers, strings, operators, punctuation, spaces, and comments. Spaces and comments are ignored.

## Parser

The parser is implemented using recursive-descent functions that match grammar non-terminals such as `E`, `Ew`, `T`, `Ta`, `Tc`, `B`, `Bt`, `Bs`, `Bp`, `A`, `At`, `Af`, `Ap`, `R`, `Rn`, `D`, `Da`, `Dr`, `Db`, and `Vb`.

## Standardization

The standardizer rewrites AST forms into standard tree forms. Examples:

- `let X = E in P` becomes `gamma(lambda(X, P), E)`
- `P where X = E` becomes `gamma(lambda(X, P), E)`
- `fcn_form` becomes an `=` node with nested `lambda` nodes
- `and` becomes comma/tau form
- `rec` becomes a Y-star based recursive form

## Evaluation

The evaluator uses environment frames, closures, tuples, conditionals, arithmetic operations, boolean operations, and common RPAL built-in functions such as `Print`, `Order`, `Conc`, `Stem`, `Stern`, and type checking functions.

## How to Compile and Run

```bash
make
./rpal20 sample.rpal
```

Expected output:

```text
15
```

## Limitations

The implementation is designed to support the standard RPAL project grammar and common test programs. Extra non-standard RPAL extensions are not supported.
