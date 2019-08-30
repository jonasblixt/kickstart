#ifndef __KICKSTART_H__
#define __KICKSTART_H__

#define KS_OK 0
#define KS_ERR 1


enum
{
    KS_SIGN_INVALID = 0,
    KS_SIGN_NIST256p = 1,
    KS_SIGN_NIST384p = 2,
    KS_SIGN_NIST521p = 3,
};

enum
{
    KS_HASH_INVALID = 0,
    KS_HASH_SHA256 = 1,
    KS_HASH_SHA384 = 2,
    KS_HASH_SHA512 = 3,
};

#endif
