#include <stdio.h>
#include <kickstart/kickstart.h>
#include <kickstart/log.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


static int udp_sink_write(int fd, const void *buf, size_t count)
{
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8000);
    saddr.sin_addr.s_addr = inet_addr("198.0.0.255");

    printf ("udp_sink_write\n");
    return sendto(fd, buf, count, 0, (struct sockaddr *) &saddr, sizeof(saddr));
}


static int udp_output_formatter(struct ks_log_sink *sink,
                                   uint16_t sz,
                                   uint16_t *new_sz,
                                   struct ks_log_entry_header *hdr)
{
    int len;
    memcpy(sink->tmp, sink->buf, sz);

    len = snprintf(sink->buf, KS_LOG_OUTPUT_BUF_SZ,
             "%li.%9.9li %s %s: ",
             hdr->ts.tv_sec, hdr->ts.tv_nsec,
             ks_log_level_to_string(hdr->lvl),
             ks_log_source_id_to_string(sink->log, hdr->source_id));
    
    if (len && (len + sz < KS_LOG_OUTPUT_BUF_SZ))
    {
        memcpy(sink->buf + len, sink->tmp, sz);
        sink->buf[len+sz] = 0;
    }
    else
    {
        return KS_ERR;
    }

    (*new_sz) = strlen(sink->buf);

    return KS_OK;
}

int ks_log_init_udp_sink(struct ks_log *log, struct ks_log_sink **sink,
                         const char *addr, int port)
{
    int rc;
    int socket_fd;
    int broadcast_permission;
    struct sockaddr_in saddr;

    socket_fd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);

    if (socket_fd < 0)
        return KS_ERR;

    broadcast_permission = 1;
    rc = setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, (void *)
                    &broadcast_permission, sizeof(broadcast_permission));

    if (rc < 0)
        return KS_ERR;
    int sock_reuse_flag = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &sock_reuse_flag,
                     sizeof(int));

    memset(&saddr, 0, sizeof(saddr));

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = inet_addr(addr);
/*
    if (rc < 0)
    {
        ks_log_printf(log, KS_LOG_LEVEL_ERR, "could not connect socket: %s\n",
                        strerror(errno));
        rc = KS_ERR;
        goto err_close_socket;
    }
  */ 

    rc = ks_log_add_sink(log, sink, socket_fd);

    if (rc != KS_OK)
        goto err_close_socket;

    (*sink)->write_cb = udp_sink_write;
    rc = ks_log_set_output_formatter(*sink, udp_output_formatter);

    if (rc != KS_OK)
        goto err_free_sink;

    return KS_OK;

err_free_sink:
    ks_log_free_sink(*sink);
err_close_socket:
    close(socket_fd);
    return KS_ERR;
}

int ks_log_free_udp_sink(struct ks_log_sink *sink)
{
    return ks_log_free_sink(sink);
}
