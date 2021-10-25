#include <stdio.h>
#include <math.h>

static double newton(double a, double x1) {
  double x2 = (x1 + a / x1) / 2;
  if (fabs(x2 - x1) < 1e-6) return x2;
  else return newton(a, x2);
}

// s_{n+1} = (s_n + x/s_n)/2
// calculate the root of f(x) = \sqrt(x) when x = 2
int run_newton_recursion() {
  double x = 2;
  double result = newton(x, 1);
  printf("sqrt(%lf) = %lf\n", x, result);
  double ans = 1.4142135623730951;
  return (fabs(result - ans) > 1e-6) ? 1 : 0;
}
