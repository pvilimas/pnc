# pnc - Prefix Notation Calculator
A shell command that evaluates simple math expressions using prefix notation. Built using [libgmp](https://gmplib.org/gmp-man-6.3.0.pdf).

## Still in development, currently supports:
- Arbitrary precision for all number types, including:
    - Integers
    - Rational numbers `3/5`
    - Real numbers
- Functions: `(+ 5 6)` evaluates to 11
- Lists of numbers: `(list 1 2 3)`
- List operations: `range`
