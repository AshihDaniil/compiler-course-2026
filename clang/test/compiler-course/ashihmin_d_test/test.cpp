// RUN: %clang_cc1 -load %llvmshlibdir/ashihmin_d_lab1_ClangAST%pluginext -plugin ashihmin_d_analizator -fsyntax-only -verify %s

extern "C" void* malloc(unsigned long size);
extern "C" void free(void* ptr);
typedef struct FILE FILE;
extern "C" FILE* fopen(const char* filename, const char* mode);
extern "C" int fclose(FILE* stream);

void test_malloc_leak() {
    int* data = (int*)malloc(1024); // expected-warning {{не освобождены}}
}

void test_new_leak() {
    int* p = new int[10]; // expected-warning {{не освобождены}}
}

void test_file_leak() {
    FILE* f = fopen("config.txt", "r"); // expected-warning {{не освобождены}}
}

void test_no_leak() {
    int* ptr = (int*)malloc(4);
    free(ptr);
}

void test_return_leak(int x) {
    int* p = new int; 
    if (x > 0) {
        return; // expected-warning {{не гарантированное освобождение при return}}
    }
    delete p;
}