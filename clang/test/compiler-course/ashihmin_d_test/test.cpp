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

//added

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