#include <stdio.h>

#define MAP(c, f) c(f)
#define FUN_LIST(f) \
  f(run_pi) \
  f(run_newton) \
  f(run_integral) \
  f(run_taiji) \
  f(run_newton_recursion) \
  f(run_curve) \

#define RUN(f) do { \
  puts("Running " #f "..."); \
  int ret = (f)(); \
  if (ret != 0) puts("Error in " #f); \
} while (0);

int run_pi();
int run_newton();
int run_integral();
int run_curve();
int run_taiji();
int run_newton_recursion();

int main() {
  MAP(FUN_LIST, RUN);
}
