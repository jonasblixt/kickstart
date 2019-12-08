#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <linux/reboot.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/route/link.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <kickstart/eventloop.h>
#include <kickstart/log.h>

static int callback(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct ifinfomsg *iface = NLMSG_DATA(nlh);
    struct rtattr *hdr = IFLA_RTA(iface);
    struct ks_log *log = (struct ks_log *) arg;
    int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));


    while (RTA_OK(hdr, remaining))
    {
        if (hdr->rta_type == IFLA_IFNAME)
        {
            ks_log_printf(log, KS_LOG_LEVEL_INFO,
                        "Found network interface %d: %s\n", iface->ifi_index,
                        (char *) RTA_DATA(hdr));
        }

        hdr = RTA_NEXT(hdr, remaining);
    }

    return NL_OK;
}

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
    } else {
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
    } while (read_sz);

    fflush(stdout);
    fclose(fp);
    return 0;
}

void timer_cb(void *data, struct ks_eventloop_io *io)
{

	struct itimerspec ts;
    struct sysinfo info;
	int msec = 500;
    struct ks_log *log = (struct ks_log *) io->data;

    sysinfo(&info);

    ks_log_printf(log, KS_LOG_LEVEL_INFO, "free: %ikb, share: %ikb, " \
        "buf: %ikb, load %li, %li, %li\n",
        info.freeram/1024,
        info.sharedram/1024,
        info.bufferram/1024,
        info.loads[0],
        info.loads[1],
        info.loads[2]);

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = msec / 1000;
	ts.it_value.tv_nsec = (msec % 1000) * 1000000;

	timerfd_settime(io->fd, 0, &ts, NULL);
}

int main(int argc, char **argv)
{
    struct ks_eventloop_ctx *el;
    struct ks_log *log;
    struct termios ctrl;
    int err = 0;
   

    err = ks_eventloop_init(&el);

    ks_log_init(&log, el, 16*KS_KB);
    
	struct itimerspec ts;
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
	int msec = 500;

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = msec / 1000;
	ts.it_value.tv_nsec = (msec % 1000) * 1000000;

	err = timerfd_settime(tfd, 0, &ts, NULL);
    struct ks_eventloop_io *io;
    err = ks_eventloop_alloc_io(el, &io);
    int i = 0;

    io->fd = tfd;
    io->cb = timer_cb;
    io->data = log;
    io->flags = EPOLLIN;

    err = ks_eventloop_add(el, io);

    struct ks_log_sink *stdout_sink;
    struct ks_log_sink *udp_sink;

    ks_log_init_stdout_sink(log, &stdout_sink);

    ks_log_printf(log, KS_LOG_LEVEL_INFO, "Kickstart init %s\n", VERSION);

    tcgetattr(STDIN_FILENO, &ctrl);
    ctrl.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ctrl);

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

    // Open socket to kernel.
    struct nl_sock *socket = nl_socket_alloc();  // Allocate new netlink socket in memory.
    nl_connect(socket, NETLINK_ROUTE);  // Create file descriptor and bind socket.

    // Send request for all network interfaces.
    struct rtgenmsg rt_hdr = { .rtgen_family = AF_PACKET, };
    int ret = nl_send_simple(socket, RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP,
                                &rt_hdr, sizeof(rt_hdr));

    // Retrieve the kernel's answer.
    nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, callback, log);
    nl_recvmsgs_default(socket);

    struct rtnl_link *link, *change;
    struct nl_cache *cache;
    struct nl_addr *local_addr, *bcast_addr;
    struct rtnl_addr *addr;
    rtnl_link_alloc_cache(socket, AF_UNSPEC, &cache);

    link = rtnl_link_get_by_name(cache, "usb0");

    /* Enable interface */
    change = rtnl_link_alloc();
	rtnl_link_set_flags(change, IFF_UP);
    rtnl_link_change(socket, link, change, 0);
    rtnl_link_put(change);
    
    /* Set ip address */
    addr = rtnl_addr_alloc();
    rtnl_addr_set_family(addr, AF_INET);
    rtnl_addr_set_link(addr, link);
    nl_addr_parse("198.0.0.1/24", AF_INET, &local_addr);
    nl_addr_parse("198.0.0.255", AF_INET, &bcast_addr);
    rtnl_addr_set_local(addr, local_addr);
    rtnl_addr_set_broadcast(addr, bcast_addr);
    err = rtnl_addr_add(socket, addr, NLM_F_REPLACE);

    if (err < 0)
        printf ("addr_add error: %s\n", nl_geterror(err));
/*
    err = ks_log_init_udp_sink(log, &udp_sink, "198.0.0.255", 8000);

    if (err != KS_OK)
        ks_log_printf(log, KS_LOG_LEVEL_ERR, "Could not initialise log UDP server\n");
*/
    ks_eventloop_loop(el);
/*
    while (1)
    {
        int c = getc(stdin);
        if (c == 'p')
        {
            f = fopen("/sys/power/state","w");
            fprintf(f,"mem");
            fclose(f);
        }
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
*/
}
