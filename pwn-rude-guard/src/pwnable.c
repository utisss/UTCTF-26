#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <immintrin.h>

int main(int argc, char* argv[]) {
    if (argc == 1){
        printf("Are you not going to say hello?\n");
        return 0;
    }

    int f = atoi(argv[1]) - 0x68656C6C6F;
    
    if (f){
        printf("Hi. Go away.\n");
        return 0;
    }

    printf("Hi. What do you want.\n");
    read_input(f);

    return 0;
}

int read_input(int fd){
    char buf[32];
    read(fd, buf, 100);

    if (!strcmp(buf, "givemeflag\n")){
        printf("How rude! utflag{you're going to need a sneakier way in...}\n");
    } else {
        printf("I won't let you pass. No matter what.\n");
    }

    return 0;
}

int secret_function() {
    unsigned char key = 0x32;

    unsigned char data[] = {
        0x47, 0x46, 0x54, 0x5e, 0x53, 0x55, 0x49, 0x55, 0x47, 0x06, 
        0x40, 0x56, 0x6d, 0x45, 0x06, 0x41, 0x6d, 0x45, 0x01, 0x06, 
        0x59, 0x57, 0x40, 0x6d, 0x46, 0x5a, 0x06, 0x5c, 0x6d, 0x5b, 
        0x6d, 0x46, 0x5a, 0x02, 0x47, 0x55, 0x5a, 0x46, 0x4f
    };

    int length = sizeof(data) / sizeof(data[0]);
    
    for (int i = 0; i < length; i++){
        printf("%c", data[i] ^ key);
    }
    return 0;
}
