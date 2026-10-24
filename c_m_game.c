#include "colors.h"

int main(void)
{
    const char *colors[4] = {
        "\033[41m    \033[0m",
        "\033[42m    \033[0m",
        "\033[43m    \033[0m",
        "\033[44m    \033[0m"
    };

    int i;
    int sequence[10];

    srand(time(NULL));
    i = 0;
    while (i < 10)
    {
        sequence[i] = rand() % 4;
        printf("Remember this color: %s\n", colors[sequence[i]]);
        i++;
    }
    //int answer;
    
    //scanf("%d", &answer);
    return (0); 
}