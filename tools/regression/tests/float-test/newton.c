#include <stdio.h>
#include <math.h>

// s_{n+1} = (s_n + x/s_n)/2
// calculate the root of f(x) = \sqrt(x) when x = 2
int run_newton() {
  double s1, s2 = 1;
  double x = 2;
  do {
    s1 = s2;
    s2 = (s1 + x / s1) / 2;
  } while (fabs(s2 - s1) >= 1e-6);

  printf("sqrt(%lf) = %lf\n", x, s2);

  double ans = 1.4142135623730951;
  return (fabs(s2 - ans) > 1e-6) ? 1 : 0;
}
