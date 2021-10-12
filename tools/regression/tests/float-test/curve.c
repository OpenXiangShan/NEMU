#include <stdio.h>
#include <math.h>

// draw the curve of sin(x) and cos(x)
int run_curve() {
  int i, isin, icos;
  double x;
  double step = 2 * M_PI / 50;
  char line[62];
  for (x = 0; x < 6.28319; x += step) {
    for (i = 1; i < 61; line[i ++] = ' ');
    isin = 29 * sin(x) + 31;
    icos = 29 * cos(x) + 31;
    line[isin] = 's';
    line[icos] = 'c';
    line[0] = line[31] = line[60] = '|';
    line[61] = '\0';
    printf("%8.5f\t%s\n", x, line);
  }

  return 0;
}
