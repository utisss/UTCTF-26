#include <stdio.h>
#include <string.h>

void setup() {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
}

void print_flag() {
    unsigned char enc[] = {0x37, 0x36, 0x24, 0x2e, 0x23, 0x25, 0x39, 0x24, 0x72, 0x30, 0x2f, 0x76, 0x36, 0x1d, 0x31, 0x36, 0x30, 0x73, 0x2c, 0x25, 0x1d, 0x2e, 0x71, 0x76, 0x29, 0x71, 0x26, 0x3f};
    for (int i = 0; i < 28; i++) {
        putchar(enc[i] ^ 0x42);
    }
    putchar('\n');
}

int main() {
    setup();

    volatile unsigned int secret = 0xdeadbeef;
    char name[64];

    printf("What is your name? ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    printf("Hello, ");
    printf(name);      // <-- format string vulnerability
    printf("!\n");

    printf("Enter the secret code: ");
    unsigned int guess;
    scanf("%u", &guess);

    if (guess == secret) {
        print_flag();
    } else {
        puts("Wrong! Nice try.");
    }

    return 0;
}