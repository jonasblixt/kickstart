#ifndef __KICKSTART_H__
#define __KICKSTART_H__

#include <stdint.h>

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

#define ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]) \
     + sizeof(typeof(int[1 - 2 * \
           !!__builtin_types_compatible_p(typeof(arr), \
                 typeof(&arr[0]))])) * 0)

#endif
