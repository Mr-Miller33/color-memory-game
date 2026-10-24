#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

#define TIME_LIMIT_MS 4000

static struct termios	g_original_terminal;
static volatile sig_atomic_t	g_stop;

static void	restore_terminal(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &g_original_terminal);
	printf("\033[0m\033[?25h\n");
	fflush(stdout);
}

static void	stop_game(int signum)
{
	g_stop = signum;
}

static int	setup_terminal(void)
{
	struct termios	settings;
	struct sigaction	action;

	if (tcgetattr(STDIN_FILENO, &g_original_terminal) == -1)
		return (-1);
	if (atexit(restore_terminal) != 0)
		return (-1);
	action = (struct sigaction){0};
	action.sa_handler = stop_game;
	sigemptyset(&action.sa_mask);
	if (sigaction(SIGINT, &action, NULL) == -1
		|| sigaction(SIGTERM, &action, NULL) == -1)
		return (-1);
	settings = g_original_terminal;
	settings.c_lflag &= ~(ICANON | ECHO);
	settings.c_cc[VMIN] = 1;
	settings.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == -1)
		return (-1);
	printf("\033[?25l");
	fflush(stdout);
	return (0);
}

static void	show_timer(int remaining_ms, int row, int col)
{
	int filled;
	int i;

	filled = remaining_ms * 20 / TIME_LIMIT_MS;
	if (filled < 0)
		filled = 0;
	if (filled > 20)
		filled = 20;
	printf ("\033[%d;%dH[", row, col);
	i = 0;
	while (i < 20)
	{
		if (i < filled)
			printf("#");
		else
			printf("-");
		i++;
	}
	printf("]  %.1f s\033[K", remaining_ms / 1000.0);
	fflush(stdout);
}

static int	get_key_timeout(int timeout_ms)
{
	struct pollfd pfd;
	char	c;
	int	result;

	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN;
	result = poll(&pfd, 1, timeout_ms);
	if (result == 0 || (result < 0 && errno == EINTR))
		return (-1);
	if (result < 0 || !(pfd.revents & POLLIN))
		return (-2);
	if (read(STDIN_FILENO, &c, 1) != 1)
		return (-2);
	return ((unsigned char)c);
}

static void	show_color(int level, const char *color)
{
	struct winsize	ws;
	int		row;
	int		col;
	int		i;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
		return ;
	row = (ws.ws_row - BLOCK_H) / 2;
	col = (ws.ws_col - BLOCK_W) / 2;
	if (row < 1)
		row = 1;
	if (col < 1)
		col = 1;
	printf("\033[2J\033[H");
	printf("\033[2;2Hlevel %d", level);
	i = 0;
	while (i < BLOCK_H)
	{
		printf("\033[%d;%dH%s", row + i, col, color);
		printf("%*s\033[0m", BLOCK_W, "");
		i++;
	}
	fflush(stdout);
}

static void	move_cursor(int row, int col)
{
	printf("\033[%d;%dH", row, col);
}

/* The same start time is used for every key in this level. */
static int	remaining_time(const struct timespec *start)
{
	struct timespec	now;
	long			elapsed_ms;

	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
		return (-1);
	elapsed_ms = (now.tv_sec - start->tv_sec) * 1000
		+ (now.tv_nsec - start->tv_nsec) / 1000000;
	if (elapsed_ms >= TIME_LIMIT_MS)
		return (0);
	return (TIME_LIMIT_MS - elapsed_ms);
}

static int	color_index(int key)
{
	if (key == 'r')
		return (0);
	if (key == 'g')
		return (1);
	if (key == 'y')
		return (2);
	if (key == 'b')
		return (3);
	return (-1);
}

static int	read_sequence(const int *sequence, int level)
{
	struct timespec	start;
	int			remaining;
	int			key;
	int			i;
	int			wait_ms;

	printf("\033[2J\033[HLevel %d: repeat the whole sequence in 4 seconds!\n", level);
	printf("r = Red, g = Green, y = Yellow, b = Blue (no Enter needed)\n");
	/* Discard keys pressed while the colors were being displayed. */
	if (tcflush(STDIN_FILENO, TCIFLUSH) == -1
		|| clock_gettime(CLOCK_MONOTONIC, &start) == -1)
		return (-1);
	i = 0;
	while (i < level && !g_stop)
	{
		remaining = remaining_time(&start);
		if (remaining < 0)
			return (-1);
		show_timer(remaining, 4, 2);
		if (remaining == 0)
		{
			printf("\033[6;1H\033[KTime's up! Try again :(\n");
			return (0);
		}
		/* Refresh the display at least every 50 ms, even without a key. */
		wait_ms = 50;
		if (remaining < wait_ms)
			wait_ms = remaining;
		key = get_key_timeout(wait_ms);
		if (g_stop)
			return (0);
		if (key == -2)
			return (-1);
		if (key == -1 || key == '\n' || key == '\r')
			continue ;
		/* Never accept a key after the deadline. */
		remaining = remaining_time(&start);
		if (remaining < 0)
			return (-1);
		if (remaining == 0)
			continue ;
		show_timer(remaining, 4, 2);
		if (color_index(key) == -1)
		{
			printf("\033[6;1H\033[KWrong command: use r, g, y or b!\n");
			return (0);
		}
		if (color_index(key) != sequence[i])
		{
			printf("\033[6;1H\033[KEehh you lost, try again :(\n");
			return (0);
		}
		i++;
		printf("\033[6;1H\033[KCorrect: %d/%d", i, level);
		fflush(stdout);
	}
	printf("\033[7;1H");
	return (!g_stop);
}

int main(void)
{
	//Colors printing and rendomizing section

    const char *colors[4] = {
        "\033[41m",
        "\033[42m",
        "\033[43m",
		"\033[44m"
    };

    int	i;
	int	level = 1;
    int	sequence[10];
	int	rst;

	if (setup_terminal() == -1)
	{
		fprintf(stderr, "Could not set up the terminal. Run in a terminal window.\n");
		return (1);
	}
    srand(time(NULL));

	//Leveling up and game developemen loop

	while (level <= 10 && !g_stop)
	{
		printf("\033[2J\033[H");
		move_cursor(2, 5);
		printf("Level %d", level);
		move_cursor(10, 35);
		printf("Level %d\n", level);
		printf("\n");
		printf("\n");
    	i = 0;
    	while (i < level)
    	{
        	sequence[i] = rand() % 4;
			i++;
		}
		
		//Display loop

		i = 0;
		while (i < level)
		{
			printf("\033[2J\033[H");

			show_color(level, colors[sequence[i]]);
			sleep(1);
			if (g_stop)
				return (128 + g_stop);

			printf("\033[2J\033[H");
			fflush(stdout);
			usleep(100000);
			i++;
		}
		
		// One four-second deadline for the entire answer sequence.
		if (g_stop)
			return (128 + g_stop);
		rst = read_sequence(sequence, level);
		if (g_stop)
			return (128 + g_stop);
		if (rst < 0)
		{
			fprintf(stderr, "\nCould not read input or the clock.\n");
			return (1);
		}
		if (rst == 0)
			return (0);

		//Clearing a level and passing to the next

		printf("Level completed moving to next level \n");
		sleep(2);
		level++;
	}
	if (g_stop)
		return (128 + g_stop);
	printf("woow your memory is stronger than my game ,bravooo :)\n");
    return (0); 
}