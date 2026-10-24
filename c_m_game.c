#include "colors.h"

int main(void)
{
    const char *colors[4] = {
        "\033[41m          \033[0m",
        "\033[42m          \033[0m",
        "\033[43m          \033[0m",
		"\033[44m          \033[0m"
    };

    int	i;
	int	level = 1;
    int	sequence[10];
	int	rst;
    char	answer;

    srand(time(NULL));
	while (level <= 10)
	{
		printf("\033[2J\033[H");
		printf("Level %d\n", level);
		printf("\n");
		printf("\n");
    	i = 0;
    	while (i < level)
    	{
        	sequence[i] = rand() % 4;
			i++;
		}
		i = 0;
		while (i < level)
		{
			printf("Remember this color: %s\n", colors[sequence[i]]);
			fflush(stdout);
			sleep(2);
			printf("\033[2J\033[H");
			fflush(stdout);
			usleep(100000);
			i++;
		}
		i = 0;
		while (i < level)
		{
			rst = -1;
			printf("press the correct colors in the right sequence \n");
			printf("r = Red, g = Green, y = Yellow, b = Blue\n");
			if (scanf(" %c", &answer) != 1)
				return (1);
			if (answer == 'r')
				rst = 0;
			else if (answer == 'g')
				rst = 1;
			else if (answer == 'y')
				rst = 2;
			else if (answer == 'b')			
				rst = 3;
			if (rst == -1)
			{
				printf("wrong command ,you should chose one of the four color command -_-!\n");
				return (1);
			}
			if (rst != sequence[i])
			{
				printf("eehh you lost ,try again :(\n");
				return (0);
			}
			printf("noiceee :)\n");
			i++;
		}
		printf("Level completed moving to next level \n");
		sleep(2);
		level++;
	}
	printf("woow your memory is stronger than my game ,bravooo :)\n");
    return (0); 
}