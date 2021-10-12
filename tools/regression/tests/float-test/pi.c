#include <stdio.h>
#include <math.h>

// calculate pi/4 = 1 - 1/3 + 1/5 - 1/7 + ...
int run_pi() {
  double s = 0, x = 1;
  int sign = 1;
  long k = 1;
  while (fabs(x) > 4e-7) {
    s += x;
    k += 2;
    sign *= -1;
    x = sign/(double)k;
  }
  s *= 4;

  printf("pi = %lf\n", s);

  return (fabs(s - M_PI) > 1e-6) ? 1 : 0;
}
