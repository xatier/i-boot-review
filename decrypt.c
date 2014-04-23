#include <stdio.h>

int main (int argc, char **argv) {
    FILE *fin;
    char *buf;

    if (argc != 2)
        return 1;

    buf = malloc(10);
    fin = fopen(argv[1], "r");

    if (fin) {
        // in function ide_write(), the read buffer only encrypt on even bytes
        // thanks to jyhsu
        // static int ide_sector_buffer_stride = 2;

        while (fread(buf, 1, 1, fin)) {
            putchar(buf[0] ^ '1');
            if(!fread(buf, 1, 1, fin))
                break;
            putchar(buf[0]);
        }
        fclose(fin);
    }
    putchar('\n');
    free(buf);

    return 0;
}
