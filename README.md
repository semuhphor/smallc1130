# Small C 1130

This project is aimed to create a C languange cross compiler for the IBM 1130.


## The Development Environment

- Fedora: 39
- Wine: 9.1
- Dosbox: dosbox-staging-0.80.1
- clang: version 17.0.6
- Make: GNU Make 4.4.1

None of that should be critical; the code is quite vanilla. THe only special 
requirement is that the C compiler be able to generate 32 bit code ("-m32" for
gcc and clang).

## Building cc1130:

>  make

## Building cctest1.ASM

>  dosbox cctest1



