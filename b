clang++ -c -std=c++11 -Wall -Ofast eqmem.cpp bin.cpp bucket.cpp memmanager.cpp
ar rc libeqmem.a eqmem.o bin.o bucket.o memmanager.o
rm *.o
clang++ -std=c++11 -Wall -L. -l eqmem -o test -Ofast main.cpp
