// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Plain C99 program with no CoralNPU-specific code: checks that ordinary C
// (libc, libm, heap, 64-bit and double arithmetic) runs bare-metal.

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int checks_total = 0;
int checks_passed = 0;

#define CHECK(name, cond)                                         \
  do {                                                            \
    int ok = (cond);                                              \
    checks_total++;                                               \
    checks_passed += ok;                                          \
    printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);              \
  } while (0)

typedef struct {
  const char* name;
  int score;
} Entry;

static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

static int cmp_int(const void* a, const void* b) {
  return *(const int*)a - *(const int*)b;
}

static int cmp_entry(const void* a, const void* b) {
  return ((const Entry*)b)->score - ((const Entry*)a)->score;
}

int main(void) {
  // recursion
  CHECK("fib(20) == 6765", fib(20) == 6765);

  // qsort (libc) on ints and on structs
  int v[8] = {42, 7, 19, 3, 88, 1, 56, 23};
  qsort(v, 8, sizeof(int), cmp_int);
  CHECK("qsort ints", v[0] == 1 && v[7] == 88 && v[4] == 23);
  Entry e[3] = {{"lemon", 3}, {"lime", 9}, {"orange", 5}};
  qsort(e, 3, sizeof(Entry), cmp_entry);
  CHECK("qsort structs", strcmp(e[0].name, "lime") == 0);

  // heap
  int* buf = (int*)malloc(1000 * sizeof(int));
  long sum = 0;
  for (int i = 0; i < 1000; i++) buf[i] = i;
  for (int i = 0; i < 1000; i++) sum += buf[i];
  free(buf);
  CHECK("malloc/free + sum 0..999", buf != NULL && sum == 499500);

  // string / formatting
  char s[64];
  snprintf(s, sizeof(s), "%s-%04d-%x", "id", 42, 0xBEEF);
  CHECK("snprintf", strcmp(s, "id-0042-beef") == 0);
  CHECK("strstr/strlen", strstr("coral npu", "npu") != NULL && strlen(s) == 12);

  // 64-bit integer arithmetic on a 32-bit core
  uint64_t big = 0x123456789ULL * 1000003ULL;
  CHECK("uint64 mul/div", big / 1000003ULL == 0x123456789ULL);

  // single and double precision floating point, libm
  float f = 1.0f;
  for (int i = 1; i <= 10; i++) f *= 1.5f;
  CHECK("float pow", fabsf(f - 57.665039f) < 1e-3f);
  double d = sqrt(2.0) * sqrt(2.0);
  CHECK("double sqrt", fabs(d - 2.0) < 1e-12);
  CHECK("sin(pi/2)", fabs(sin(M_PI / 2) - 1.0) < 1e-12);

  printf("%d / %d checks passed\n", checks_passed, checks_total);
  return checks_passed == checks_total ? 0 : 1;
}
