#include <stdio.h>

int main() {
    int Motors = 0, AS = 0, Brake = 0 , RES = 0, APPS = 0, SDC = 0, Rain_test = 0, 
    R2D = 0, Time_AS_on = 0, Brake_testing = 0;
// Brake = 1 = Brake engaged
// AS = 0 = AS off
// AS = 1 = AS ready mode
// AS = 2 = AS driving mode
// RES = 1 = go signal
// Motors = 1 = Motors give respond to APPS
// Brake_testing = 1 = Brake testing mode

/*  Not sure if this is related to R2D or AS...

    if((AS == 0 && Brake == 0) || (AS==1 && Brake == 1) ){
        R2D = 1;
        printf("R2D is ON\n");
    }

    if(AS == 2){
        R2D = 0;
        printf("R2D is OFF\n");
    }
*/

    if(((RES == 1) && (AS == 1) && (Time_AS_on >= 5)) || (Motors ==1) ){
        R2D = 1;
        printf("%d\n",R2D);
    }

    if((SDC == 1) || (Rain_test == 1) || (Brake_testing == 1)){
        R2D = 0;
        printf("%d\n",R2D);
    }

    return 0;
}