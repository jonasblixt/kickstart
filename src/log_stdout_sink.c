#include <stdio.h>
#include <unistd.h>
#include <kickstart/log.h>


static int stdout_output_formatter(struct ks_log_sink *sink,
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

int ks_log_init_stdout_sink(struct ks_log *log, struct ks_log_sink **sink)
{
    int rc;

    rc = ks_log_add_sink(log, sink, STDOUT_FILENO);

    if (rc != KS_OK)
        return rc;

    rc = ks_log_set_output_formatter(*sink, stdout_output_formatter);

    if (rc != KS_OK)
        goto err_free_sink;

    return KS_OK;

err_free_sink:
    ks_log_free_sink(*sink);
    return KS_ERR;
}

int ks_log_free_stdout_sink(struct ks_log_sink *sink)
{
    return ks_log_free_sink(sink);
}
