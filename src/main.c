#include <stdio.h>
#include <unistd.h>
#include <termios.h>

int main(int argc, char **argv)
{
    struct termios ctrl;

    tcgetattr(STDIN_FILENO, &ctrl);
    ctrl.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ctrl);

    printf ("Kickstart %s starting...\n",VERSION);

    while (1)
    {
        int c = getc(stdin);

        if (c == 'r')
        {
            printf ("Rebooting...\n");
            reboot(0x1234567);
        }
    }
}
