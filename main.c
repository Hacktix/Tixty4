#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("No ROM file selected!\n\n");
        getchar();
        return 0;
    }

    FILE* romf = fopen(argv[1], "r");
    if (romf == NULL) {
        printf("Could not open ROM File!\n\n");
        getchar();
        return 0;
    }
}