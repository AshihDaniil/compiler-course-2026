// RUN: %clang_cc1 -load %llvmshlibdir/libashihmin_d_lab1_ClangAST%pluginext -plugin ashihmin_d_analizator -fsyntax-only %s 2>&1 | FileCheck %s

extern "C" void* malloc(unsigned long size);
extern "C" void free(void* ptr);
typedef struct FILE FILE;
extern "C" FILE* fopen(const char* filename, const char* mode);
extern "C" int fclose(FILE* stream);

// Тест 1: Утечка памяти через malloc
void test_malloc_leak() {
    int* data = (int*)malloc(1024);
    // CHECK: warning: Память или ресурс для переменной 'data' не освобождены!
}

// Тест 2: Утечка через оператор new
void test_new_leak() {
    int* p = new int[10];
    // CHECK: warning: Память или ресурс для переменной 'p' не освобождены!
}

// Тест 3: Утечка файлового дескриптора
void test_file_leak() {
    FILE* f = fopen("config.txt", "r");
    if (!f) return;
    // CHECK: warning: Память или ресурс для переменной 'f' не освобождены!
}

// Тест 4: Нет утечки (все освобождено правильно)
void test_no_leak() {
    int* ptr = (int*)malloc(4);
    free(ptr);

    FILE* file = fopen("log.txt", "w");
    if (file) {
        fclose(file);
    }
    
    // CHECK-NOT: warning
}

// Тест 5: Утечка при досрочном выходе (return)
void test_return_leak(int x) {
    int* p = new int;
    if (x > 0) {
        return; 
        // CHECK: warning: Ресурс для переменной 'p' может быть не освобожден (не гарантированное освобождение при return)!
    }
    delete p;
}