#include <stdio.h>
#include <math.h>

#define k 1.6

static double f(int j, int i) {
  double dj = j - k * 10;
  double di = i * k;
  return sqrt(dj * dj + di * di);
}

// draw the figure of TaiJi
int run_taiji() {
  int i, j;
  char b[61];
  for (i = 0; i < 23; i ++) {
    for (j = 0; j < 60; j ++) {
      b[j] = ' ';
      if ((int)f(j, i-10) <= k * 10 &&
            ((f(j, i-5) >= 2.5*k && f(j, i-5) <= 5*k) ||
             (f(j, i-15) >= 5*k  && f(j, i-5) >= 5*k && j <= 10*k) ||
             (i >= 10 && f(j, i-15) <= 2.5*k) ||
             (int)(f(j, i-10) + 0.5) == k*10)
            ) {
        b[j] = '#';
      }
    }
    b[60] = '\0';
    printf("\t%s\n", b);
  }
  return 0;
}
