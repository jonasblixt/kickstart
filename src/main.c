#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    printf ("Kickstart %s starting...\n",VERSION);

    while (1)
        sleep(1);
}
