#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>

static int ks_ls(const char *path_str)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(path_str);

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            printf("%s\n", dir->d_name);
        }

        closedir(d);
        return 0;
    }
    else
    {
        return -1;
    }
}

static int ks_cat(const char *fn)
{
    FILE *fp = fopen(fn,"r");
    char buf[64];
    size_t read_sz = 0;

    if (fp == NULL)
        return -1;

    do
    {
        read_sz = fread(buf, 1, 64, fp);
        if (fwrite(buf, 1, read_sz, stdout) != read_sz)
            break;
    }
    while (read_sz);
    fflush(stdout);
    fclose(fp);
    return 0;
}

int main(int argc, char **argv)
{
    struct termios ctrl;

    tcgetattr(STDIN_FILENO, &ctrl);
    ctrl.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ctrl);

    printf (" --- sdf aKickstart %s ---\n",VERSION);

    while (1)
    {
        int c = getc(stdin);

        if (c == 'l')
        {
            if (ks_cat("/proc/crypto") != 0)
            {
                printf ("ks_cat failed\n");
            }
        }

        if (c == 'r')
        {
            printf ("Rebooting...\n");
            reboot(0x1234567);
        }
    }
}
