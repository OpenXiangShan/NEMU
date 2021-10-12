#include <stdio.h>
#include <math.h>

/* f(x) = 1/(1+25x^2) */
static double f(double x) {
  return 1 / (1 + 25 * x * x);
}

int run_integral() {
  int n = 10;
  double a = -1, b = 1;
  int k;
  double s, h;
  h = (b - a) / n;
  s = (f(a) + f(b)) / 2;

  for (k = 1; k < n; k ++) {
    s += f(a + h * k);
  }
  s *= h;

  printf("result = %lf\n", s);

  double ans = 0.551222;
  return (fabs(s - ans) > 1e-5) ? 1 : 0;
}

