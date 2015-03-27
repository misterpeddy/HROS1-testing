#include <stdio.h>

int main(int argc, char *argv[]) {
int a = *argv[1] - '0';
printf("number from %s is %d\n",argv[1], a);
printf("%d\n", map(-10,10,1,10,a));
return(0);
}

//maps val from (a_min,a_max) to (b_min, b_max)
int map(int a_min, int a_max, int b_min, int b_max, int val) {
  float new_val = (b_max - b_min + 0.0)/(a_max - a_min + 0.0) * (val - a_min) + b_min;
  return (int)new_val;
}
