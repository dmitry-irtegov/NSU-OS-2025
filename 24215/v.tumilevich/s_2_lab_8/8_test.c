/* gcc -O2 -o 8_test 8_test.c && ./test_8c > out.csv */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    long steps[] = {50000000, 200000000, 500000000};
    int threads[] = {1, 2, 3, 4, 5, 8, 16, 32, 64};
    
    printf("steps,threads,time\n");
    
    for (int s = 0; s < 3; s++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "gcc -O2 -o pi_test 8.c -pthread -DNUM_STEPS=%ld 2>/dev/null", steps[s]);
        if (system(cmd)) continue;
        
        for (int t = 0; t < 9; t++) {
            snprintf(cmd, sizeof(cmd), "./pi_test %d 2>/dev/null | grep time | awk '{print $3}'", threads[t]);
            FILE *fp = popen(cmd, "r");
            if (fp) {
                char time[32] = "-";
                if (fgets(time, sizeof(time), fp)) printf("%ld,%d,%s", steps[s], threads[t], time);
                pclose(fp);
            }
        }
    }
    unlink("pi_test");
    return 0;
}