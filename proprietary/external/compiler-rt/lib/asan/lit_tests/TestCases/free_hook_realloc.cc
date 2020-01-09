// Check that free hook doesn't conflict with Realloc.
// RUN: %clangxx_asan -O2 %s -o %t
// RUN: %t 2>&1 | FileCheck %s
#include <stdlib.h>
#include <unistd.h>

static void *glob_ptr;

extern "C" {
void __asan_free_hook(void *ptr) {
  if (ptr == glob_ptr) {
    *(int*)ptr = 0;
    write(1, "FreeHook\n", sizeof("FreeHook\n"));
  }
}
}

int main() {
  int *x = (int*)malloc(100);
  x[0] = 42;
  glob_ptr = x;
  int *y = (int*)realloc(x, 200);
  // Verify that free hook was called and didn't spoil the memory.
  if (y[0] != 42) {
    _exit(1);
  }
  write(1, "Passed\n", sizeof("Passed\n"));
  // CHECK: FreeHook
  // CHECK: Passed
  return 0;
}
