#include <stdio.h>

int main (int argc, char **argv) {
    FILE *fin;
    char *buf;

    if (argc != 2)
        return 1;

    buf = malloc(10);
    fin = fopen(argv[1], "r");

    if (fin) {
        while (fread(buf, 1, 1, fin)) {
            putchar(buf[0] ^ '1');
        }
        fclose(fin);
    }
    free(buf);

    return 0;
}
