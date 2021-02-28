#include <stdio.h>

int main() {
    char phone_num[12];
    int num;
    while (scanf("%11s %d", phone_num, &num) != EOF) {
        phone_num[10] = '\0';
        if (num == 0) {
            printf("%s\n", phone_num);
        } else if (1 <= num && num <= 9) {
            printf("%c\n", phone_num[num]);
        } else {
            printf("%c\n", phone_num[9]);
        }
    }
    return 0;
}