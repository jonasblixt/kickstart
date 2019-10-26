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

    printf (" --- Kickstart %s ---\n",VERSION);

    mkdir("/sys/kernel/config/usb_gadget/g1",0777);
    mkdir("/sys/kernel/config/usb_gadget/g1/strings/0x409",0777);
    mkdir("/sys/kernel/config/usb_gadget/g1/configs/c.1",0777);
    mkdir("/sys/kernel/config/usb_gadget/g1/functions/ecm.usb0",0777);
    symlink("/sys/kernel/config/usb_gadget/g1/functions/ecm.usb0",
            "/sys/kernel/config/usb_gadget/g1/configs/c.1/ecm.usb0");
    
    FILE *f = NULL;
    f = fopen("/sys/kernel/config/usb_gadget/g1/idProduct","w");
    fprintf(f,"0x1234");
    fclose(f);

    f = fopen("/sys/kernel/config/usb_gadget/g1/idVendor","w");
    fprintf(f,"0x4321");
    fclose(f);

    f = fopen("/sys/kernel/config/usb_gadget/g1/bcdUSB","w");
    fprintf(f,"0x0200");
    fclose(f);

    f = fopen("/sys/kernel/config/usb_gadget/g1/configs/c.1/MaxPower","w");
    fprintf(f,"100");
    fclose(f);

    f = fopen("/sys/kernel/config/usb_gadget/g1/UDC","w");
    fprintf(f,"ci_hdrc.0");
    fclose(f);

    while (1)
    {
        int c = getc(stdin);

        if (c == 'l')
        {
            if (ks_ls("/sys/class/net") != 0)
            {
                printf ("ks_cat failed\n");
            }
        }

        if (c == 's')
        {
            int status = -1;
            while (status != 0)
            {
                pid_t p = fork();

                if (p == 0)
                {
                    printf ("Starting shell...");
                    ctrl.c_lflag |= (ICANON | ECHO);
                    tcsetattr(STDIN_FILENO, TCSANOW, &ctrl);
                    execl("/bin/toybox","sh",NULL);
                }

                if (p)
                    waitpid(p, &status,0);
            }

            printf("Shell exit...\n");
            tcgetattr(STDIN_FILENO, &ctrl);
            ctrl.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &ctrl);
        }

        if (c == 't')
        {
            pid_t p = fork();

            if (p == 0)
            {
                printf ("Starting tee-supplicant...\n");
                int rc = execl("/tee-supplicant",NULL);
                printf ("rc = %i\n",rc);
            }
            
            if (p)
            {
                printf ("tee-supplicant started: pid=%i\n",p);
            }
        }

        if (c == 'x')
        {
            pid_t p = fork();
            if (p == 0)
            {
                printf ("Starting xtest\n");
                int rc = execl("/xtest", NULL);
                printf ("rc = %i\n",rc);
            }
        }

        if (c == 'r')
        {
            printf ("Rebooting...\n");
            reboot(0x1234567);
        }
    }
}
