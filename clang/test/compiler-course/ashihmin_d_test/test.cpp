// RUN: split-file %s %t
// RUN: %clang_cc1 -load %llvmshlibdir/ashihmin_d_lab1_ClangAST%pluginext -plugin ashihmin_d_analizator -fsyntax-only -verify %t/leaks.cpp
// RUN: %clang_cc1 -load %llvmshlibdir/ashihmin_d_lab1_ClangAST%pluginext -plugin ashihmin_d_analizator -fsyntax-only -verify %t/clean.cpp

//--- leaks.cpp
extern "C" {
    void* malloc(unsigned long size);
    void* fopen(const char* filename, const char* mode);
}

void test_malloc_leak() {
    int* data = (int*)malloc(1024); // expected-warning {{не освобождены}}
}

void test_new_leak() {
    int* p = new int[10]; // expected-warning {{не освобождены}}
}

void test_file_leak() {
    void* f = fopen("config.txt", "r"); // expected-warning {{не освобождены}}
}

void test_return_leak(int x) {
    int* p = new int; 
    if (x > 0) {
        return; // expected-warning {{не гарантированное освобождение при return}}
    }
    delete p;
}


//--- clean.cpp
// expected-no-diagnostics

extern "C" {
    void* malloc(unsigned long size);
    void free(void* ptr);
    typedef struct FILE FILE;
    FILE* fopen(const char* filename, const char* mode);
    int fclose(FILE* stream);
}

void test_clean_malloc() {
    int* p = (int*)malloc(64);
    free(p);
}

void test_clean_new() {
    int* a = new int;
    delete a;
}

void test_clean_fopen() {
    FILE* f = fopen("test.txt", "r");
    if (f) {
        fclose(f);
    }
}

void test_clean_reassign() {
    int* p = (int*)malloc(10);
    free(p);
    p = (int*)malloc(20);
    free(p);
}

void test_clean_nested() {
    {
        int* d = (int*)malloc(8);
        free(d);
    }
}