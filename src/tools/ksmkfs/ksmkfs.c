/**
 * Kickstart Image creation tool
 *
 * Copyright (C) 2019 Jonas Persson <jonpe960@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <openssl/pem.h>
#include <pkcs11-helper-1.0/pkcs11h-certificate.h>
#include <pkcs11-helper-1.0/pkcs11h-openssl.h>
#include <libcryptsetup.h>
#include <kickstart/kickstart.h>
#include <kickstart/squashfs.h>
#include "ksmkfs.h"

static EVP_PKEY *evp;
static pkcs11h_certificate_id_list_t issuers, certs, temp;
static pkcs11h_certificate_t cert;
static pkcs11h_openssl_session_t session = NULL;
static struct kickstart_block ksb;
static const char *ksfs_input_fn = NULL;
static uint8_t copy_buffer[1024];


static PKCS11H_BOOL _pkcs11h_hooks_token_prompt (
	IN void * const global_data,
	IN void * const user_data,
	IN const pkcs11h_token_id_t token,
	IN const unsigned retry)
{
	char buf[1024];
	PKCS11H_BOOL fValidInput = FALSE;
	PKCS11H_BOOL fRet = FALSE;

	while (!fValidInput) {
		fprintf (stderr, "Please insert token '%s' 'ok' or 'cancel': ", token->display);
		if (fgets (buf, sizeof (buf), stdin) == NULL) {
			printf("fgets failed");
		}
		buf[sizeof (buf)-1] = '\0';
		fflush (stdin);

		if (buf[strlen (buf)-1] == '\n') {
			buf[strlen (buf)-1] = '\0';
		}
		if (buf[strlen (buf)-1] == '\r') {
			buf[strlen (buf)-1] = '\0';
		}

		if (!strcmp (buf, "ok")) {
			fValidInput = TRUE;
			fRet = TRUE;
		}
		else if (!strcmp (buf, "cancel")) {
			fValidInput = TRUE;
		}
	}

	return fRet;
}

static PKCS11H_BOOL _pkcs11h_hooks_pin_prompt (
	IN void * const global_data,
	IN void * const user_data,
	IN const pkcs11h_token_id_t token,
	IN const unsigned retry,
	OUT char * const pin,
	IN const size_t pin_max)
{
	char prompt[1124];
    struct termios oflags, nflags;
    char password[64];

	snprintf (prompt, sizeof (prompt)-1, "Please enter '%s' PIN or 'cancel': ", 
                                                            token->display);

#if defined(_WIN32)
	{
		size_t i = 0;
		char c;
		while (i < pin_max && (c = getch ()) != '\r') {
			pin[i++] = c;
		}
	}

	fprintf (stderr, "\n");
#else
   /* disabling echo */
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        return EXIT_FAILURE;
    }

    printf("%s",prompt);

    char *r = fgets(password, sizeof(password), stdin);
    if (r == NULL)
        return false;
    password[strlen(password) - 1] = 0;

    /* restore terminal */
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        perror("tcsetattr");
        return EXIT_FAILURE;
    }
#endif

	strncpy (pin, password, pin_max);
	pin[pin_max-1] = '\0';

	return strcmp (pin, "cancel") != 0;
}
uint32_t ksmkfs_prepare(uint32_t key_index,
                         uint32_t hash_kind,
                         uint32_t sign_kind,
                         const char *key_source,
                         const char *pkcs11_provider,
                         const char *pkcs11_key_id,
                         const char *input_fn)
{

	CK_RV rv;

    bzero(&ksb, sizeof(ksb));
    ksb.magic = KICKSTART_BLOCK_MAGIC;
    ksb.version = KICKSTART_BLOCK_VERSION;
    ksb.key_index = key_index;
    ksb.hash_kind = hash_kind;
    ksb.sign_kind = sign_kind;
    
    ksfs_input_fn = input_fn;

    if (strstr(key_source, "PKCS11"))
    {

        rv = pkcs11h_initialize();

        if (rv != CKR_OK)
        {
            printf ("pkcs11h_initialize failed\n");
            return KS_ERR;
        }

        rv = pkcs11h_setTokenPromptHook (_pkcs11h_hooks_token_prompt, NULL);

        if (rv != CKR_OK)
        {
            printf ("pkcs11h_setTokenPromptHook failed\n");
            return KS_ERR;
        }
        
        rv = pkcs11h_setPINPromptHook (_pkcs11h_hooks_pin_prompt, NULL);

        if (rv != CKR_OK)
        {
            printf ("pkcs11h_setPINPromptHook failed\n");
            return KS_ERR;
        }

        printf (" PKCS11 provider '%s'\n", pkcs11_provider);
        
        rv = pkcs11h_addProvider(
                pkcs11_provider,
                pkcs11_provider,
                FALSE,
                PKCS11H_PRIVATEMODE_MASK_AUTO,
                PKCS11H_SLOTEVENT_METHOD_AUTO,
                0,
                FALSE);

        if (rv != CKR_OK)
        {
            printf ("pkcs11h_addProvider failed\n");
            return KS_ERR;
        }
        
        rv = pkcs11h_certificate_enumCertificateIds (
                PKCS11H_ENUM_METHOD_CACHE,
                NULL,
                PKCS11H_PROMPT_MASK_ALLOW_ALL,
                &issuers,
                &certs);

        if (rv != CKR_OK)
        {
            printf ("pkcs11h_certificate_enumCertificateIds failed\n");
            return KS_ERR;
        }

        for (temp = certs;temp != NULL;temp = temp->next)
        {
            printf (" Certificate: %s\n", temp->certificate_id->displayName);

            printf ("attrCKA_ID: ");
            for (uint32_t n = 0 ; n < temp->certificate_id->attrCKA_ID_size; n++)
            {
                printf("%02x ", temp->certificate_id->attrCKA_ID[n]);
            }
            printf ("\n");
        }

        if (certs == NULL)
        {
            printf ("Error: No certificates found\n");
            return KS_ERR;
        }

        rv = pkcs11h_certificate_create (
                certs->certificate_id,
                NULL,
                PKCS11H_PROMPT_MASK_ALLOW_ALL,
                PKCS11H_PIN_CACHE_INFINITE,
                &cert);

        if (rv != CKR_OK)
        {
            printf ("pkcs11h_certificate_create failed\n");
            return KS_ERR;
        }

        session = pkcs11h_openssl_createSession(cert);

        if (session == NULL)
        {
            printf("pkcs11h_openssl_createSession failed\n");
            return KS_ERR;
        }

        evp = pkcs11h_openssl_session_getEVP(session);

        if (evp == NULL) 
        {
            printf("pkcs11h_openssl_session_getEVP failed\n");
            return KS_ERR;
        }
    }
    else
    {
        evp = EVP_PKEY_new();
        FILE *fp = fopen(key_source, "r");
        if (PEM_read_PrivateKey(fp, &evp, NULL, NULL) == NULL)
        {
            printf ("Error: Could not read private key\n");
            return KS_ERR;
        }

        fclose(fp);
    }

    return KS_OK;
}

void ksmkfs_cleanup(void)
{
    CK_RV rv;

    if (session != NULL)
    {
        pkcs11h_openssl_freeSession(session);
        pkcs11h_certificate_freeCertificateIdList (issuers);
        pkcs11h_certificate_freeCertificateIdList (certs);
        
        rv = pkcs11h_terminate ();

        if (rv != CKR_OK)
        {
            printf ("pkcs11h_terminate failed");
        }
    }

}

/* Hack to extract root hash and salt
 *  libcryptsetup does not expose the crypt_device struct
 *  and therefore it is not possible to access the generated
 *  root hash and salt. This uses the log call back and filters
 *  out the result.
 * */

static bool decode_hash = false;
static bool decode_salt = false;
static int hash_counter = 0;
static int salt_counter = 0;

void verity_decoder(int level, const char *msg,
                    void *usrptr __attribute__((unused)))
{
    //fprintf(stdout, "%s", msg);

    if (decode_hash)
    {
        ksb.hash[hash_counter] = strtol(msg,NULL,16);       
        hash_counter++;

        if (hash_counter == 32)
            decode_hash = false;
    }

    if (decode_salt)
    {
        ksb.salt[salt_counter] = strtol(msg,NULL,16);       
        salt_counter++;

        if (salt_counter == 32)
            decode_salt = false;
    }

    if (strstr(msg, "Root hash:") != NULL)
    {
        decode_hash = true;
    }

    if (strstr(msg, "Salt:") != NULL)
    {
        decode_salt = true;
    }

}

uint32_t ksmkfs_out(const char *fn)
{
    FILE *fp = NULL; 
    FILE *out_fp = NULL;
    unsigned char *asn1_signature = NULL;
    size_t signature_size = 0;
	EVP_MD_CTX* ctx;
    struct stat finfo;
	struct crypt_device *cd = NULL;
	struct crypt_params_verity params = {};
	uint32_t flags = CRYPT_VERITY_CREATE_HASH;
	int r;



    stat(ksfs_input_fn, &finfo);

    fp = fopen(ksfs_input_fn,"rb");

    if (fp == NULL)
    {
        printf ("Could not open '%s' for reading\n", ksfs_input_fn);
        return KS_ERR;
    }

    out_fp = fopen (fn,"wb");

    if (out_fp == NULL)
    {
        printf ("Could not open '%s' for writing\n",fn);
        return KS_ERR;
    }

    do
    {
        r = fread(copy_buffer, 1024, 1, fp);
        if (r != 1)
            break;
        r = fwrite(copy_buffer,1024,1,out_fp);
    } while (r == 1);

    fclose(fp);
    fclose(out_fp);

    r = crypt_init(&cd, fn);

    if (r != 0)
    {
        printf ("crypt_init failed, %i\n",r);
        return KS_ERR;
    }

    params.hash_name = DEFAULT_VERITY_HASH;
    params.data_device = ksfs_input_fn;
    params.salt_size = DEFAULT_VERITY_SALT_SIZE;
	params.salt = NULL;
	params.data_block_size = DEFAULT_VERITY_DATA_BLOCK;
	params.hash_block_size = DEFAULT_VERITY_HASH_BLOCK;
	params.data_size = 0;
	params.hash_area_offset = finfo.st_size + sizeof(ksb);
	//params.fec_area_offset = 0;
	params.hash_type = 1; /* 1 - normal, 0 - chrome OS*/
	params.flags = flags;

    r = crypt_format(cd, CRYPT_VERITY, NULL, NULL, 
            "00000000-0000-0000-0000-000000000000", NULL, 0, &params);

    if (r != 0)
    {
        printf ("crypt_format failed\n");
        return KS_ERR;
    }
	crypt_set_log_callback(NULL, verity_decoder, NULL);
    crypt_dump(cd);

	crypt_free(cd);
	free((void *)params.salt);

    printf (" Hash: ");
    for (uint32_t n = 0; n < 32; n++)
        printf ("%2.2x",ksb.hash[n]);
    printf ("\n");

    printf (" Salt: ");
    for (uint32_t n = 0; n < 32; n++)
        printf ("%2.2x",ksb.salt[n]);
    printf ("\n");

    ctx = EVP_MD_CTX_create();

	if (ctx == NULL) 
    {
		printf("EVP_MD_CTX_create failed\n");
        return KS_ERR;
	}

    fp = fopen(fn, "r+");

    if (fp == NULL)
        return KS_ERR;


    switch (ksb.hash_kind)
    {
        case KS_HASH_SHA256:
            if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) == -1) 
            {
                printf("EVP_DigestInit_ex failed\n");
                return KS_ERR;
            }

            if (EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, evp) == -1) 
            {
                printf("EVP_DigestSignInit failed");
                return KS_ERR;
            }
        break;
        case KS_HASH_SHA384:
            if (EVP_DigestInit_ex(ctx, EVP_sha384(), NULL) == -1) 
            {
                printf("EVP_DigestInit_ex failed\n");
                return KS_ERR;
            }

            if (EVP_DigestSignInit(ctx, NULL, EVP_sha384(), NULL, evp) == -1) 
            {
                printf("EVP_DigestSignInit failed");
                return KS_ERR;
            }
        break;
        case KS_HASH_SHA512:
            if (EVP_DigestInit_ex(ctx, EVP_sha512(), NULL) == -1) 
            {
                printf("EVP_DigestInit_ex failed\n");
                return KS_ERR;
            }

            if (EVP_DigestSignInit(ctx, NULL, EVP_sha512(), NULL, evp) == -1) 
            {
                printf("EVP_DigestSignInit failed");
                return KS_ERR;
            }
        break;
        default:
            return KS_ERR;
    }

	if (EVP_DigestSignUpdate(ctx, &ksb, sizeof(ksb)) == -1) 
    {
		printf("EVP_DigestSignUpdate failed");
        return KS_ERR;
	}

	if (EVP_DigestSignFinal(ctx, NULL, &signature_size) == -1) 
    {
		printf("EVP_DigestSignFinal failed\n");
        return KS_ERR;
	}

	if (signature_size > KICKSTART_BLOCK_SIG_MAX_SIZE) 
    {
		printf("Signature > PB_IMAGE_SIGN_MAX_SIZE\n");
        return KS_ERR;
	}

    asn1_signature = OPENSSL_malloc(signature_size);

	if (asn1_signature == NULL) 
    {
		printf("OPENSSL_malloc failed\n");
        return KS_ERR;
	}

	if (EVP_DigestSignFinal(ctx, asn1_signature, &signature_size) == -1) 
    {
		printf("EVP_DigestSignFinal failed\n");
        return KS_ERR;
	}

    memcpy(ksb.signature, asn1_signature, signature_size);
    ksb.sign_length = signature_size;

    fseek(fp,finfo.st_size, SEEK_SET);
    fwrite(&ksb, sizeof(ksb), 1, fp);

    fclose (fp);

	OPENSSL_free(asn1_signature);
	EVP_MD_CTX_destroy(ctx);
    return KS_OK;
}

