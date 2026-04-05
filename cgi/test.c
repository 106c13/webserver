#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *fp;
    char  buf[4096];
    size_t n;

    if (argc != 2)
    {
        printf("Content-Type: text/plain\r\n\r\n");
        printf("Usage: file_cgi <file>\n");
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp)
    {
        printf("Content-Type: text/plain\r\n\r\n");
        perror("fopen");
        return 1;
    }

    /* HTTP header */
    printf("Content-Type: text/html\r\n\r\n");

    /* Stream file to stdout */
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        fwrite(buf, 1, n, stdout);

    fclose(fp);
    return 0;
}

