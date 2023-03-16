# What
Imagine if brainfuck and a spreadsheet program had a child. This is a stack machine, automaton, spreadsheet toy interpreter.

# Why?
It funny.

# Example
The program file is a description of a table, rather than a series of instructions. It interpreter always starts executing at the top-left corner (the origin).

```
0 0 right
1 0 select
2 0 'H'
3 0 'e'
4 0 'l'
5 0 'l'
6 0 'o'
7 0 ','
8 0 '\s'
9 0 'w'
10 0 'o'
11 0 'r'
12 0 'l'
13 0 'd'
14 0 '!'
15 0 '\n'
16 0 select
17 0 print
```

# How to compile

```
$ cc bs.c -o bs
```

# TODO
- [ ] Add arithmetic operations
- [ ] Add conditional jumps
- [ ] GUI? TUI? CLI?
