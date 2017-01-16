#!/bin/bash
g++ /users/ugrad/delvalh/BST/gperftools/lib/libtcmalloc.so helper.cpp main.cpp -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -pthread -o bst-with-tcmalloc -mrtm -mrdrnd
g++ helper.cpp main.cpp -pthread -o bst -mrtm -mrdrnd
