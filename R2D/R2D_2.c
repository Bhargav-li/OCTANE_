#include <stdio.h>

int main() {
    int Motors = 0, AS = 0 , Brake = 0 , RES = 0 , APPS = 0;
    int SDC = 0 , Rain_test = 0 , R2D = 0 , Time_AS_on = 0 , Brake_testing = 0;

    // Compute R2D in one line
    R2D = (RES == 1) &&
          (AS == 1) &&
          (Brake == 1) &&
          (Time_AS_on >= 5) &&
          !(SDC || Rain_test || Brake_testing || AS == 2);

    // Print result
    printf("R2D = %d\n", R2D);

    return 0;
}