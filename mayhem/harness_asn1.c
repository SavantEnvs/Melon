/* Fuzz Melon's ASN.1 BER/DER decoder and encoders.
 * Input byte 0 selects the mode: odd = decode the rest (copying and by reference),
 * even = feed the rest to one of the encoders, then decode what it produced. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mln_alloc.h"
#include "mln_asn1.h"

static mln_u64_t walk(mln_asn1_deresult_t *r, int depth)
{
    mln_u64_t sum = mln_asn1_deresult_ident_get(r) + mln_asn1_deresult_code_length_get(r);
    mln_u32_t i, n = mln_asn1_deresult_content_num(r);

    if (depth > 64) return sum;
    for (i = 0; i < n; ++i) {
        mln_asn1_deresult_t *c = mln_asn1_deresult_content_get(r, i);
        if (c != NULL) sum += walk(c, depth + 1);
    }
    return sum;
}

static int decode_all(mln_alloc_t *pool, void *buf, mln_u64_t len)
{
    int err = 0;
    mln_asn1_deresult_t *r;
    int ok = -1;

    /* mln_asn1_decode() takes ownership of a pool-allocated buffer and frees it with the result. */
    void *owned = mln_alloc_m(pool, len);
    if (owned != NULL) {
        memcpy(owned, buf, len);
        if ((r = mln_asn1_decode(owned, len, &err, pool)) != NULL) {
            (void)walk(r, 0);
            mln_asn1_deresult_free(r);
            ok = 0;
        } else {
            mln_alloc_free(owned);
        }
    }
    err = 0;
    if ((r = mln_asn1_decode_ref(buf, len, &err, pool)) != NULL) {
        (void)walk(r, 0);
        mln_asn1_deresult_free(r);
        ok = 0;
    }
    return ok;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    mln_alloc_t *pool;
    int ret = -1;

    if (size < 2) return -1;
    if ((pool = mln_alloc_init(NULL, 0)) == NULL) return -1;

    if (data[0] & 1) {
        ret = decode_all(pool, (void *)(data + 1), size - 1);
    } else {
        mln_asn1_enresult_t res;
        mln_u8ptr_t body = (mln_u8ptr_t)data + 2;
        mln_u64_t blen = size - 2, outlen = 0;
        mln_u8ptr_t out = NULL;
        int rc;

        if (mln_asn1_enresult_init(&res, pool) == M_ASN1_RET_OK) {
            switch (data[1] % 16) {
                case 0:  rc = mln_asn1_encode_boolean(&res, (mln_u8_t)blen); break;
                case 1:  rc = mln_asn1_encode_integer(&res, body, blen); break;
                case 2:  rc = mln_asn1_encode_enumerated(&res, body, blen); break;
                case 3:  rc = mln_asn1_encode_numericstring(&res, (mln_s8ptr_t)body, blen); break;
                case 4:  rc = mln_asn1_encode_bmpstring(&res, body, blen); break;
                case 5:  rc = mln_asn1_encode_bitstring(&res, body, blen); break;
                case 6:  rc = mln_asn1_encode_octetstring(&res, body, blen); break;
                case 7:  rc = mln_asn1_encode_object_identifier(&res, body, blen); break;
                case 8:  rc = mln_asn1_encode_utf8string(&res, body, blen); break;
                case 9:  rc = mln_asn1_encode_printablestring(&res, (mln_s8ptr_t)body, blen); break;
                case 10: rc = mln_asn1_encode_t61string(&res, body, blen); break;
                case 11: rc = mln_asn1_encode_ia5string(&res, body, blen); break;
                case 12: rc = mln_asn1_encode_utctime(&res, (time_t)(blen * 86400)); break;
                case 13: rc = mln_asn1_encode_generalized_time(&res, (time_t)(blen * 86400)); break;
                case 14: rc = mln_asn1_encode_null(&res); break;
                default: rc = mln_asn1_encode_octetstring(&res, body, blen) == M_ASN1_RET_OK
                                  ? mln_asn1_encode_sequence(&res) : M_ASN1_RET_ERROR; break;
            }
            if (rc == M_ASN1_RET_OK
                && mln_asn1_enresult_get_content(&res, 0, &out, &outlen) == M_ASN1_RET_OK
                && out != NULL && outlen > 0)
            {
                ret = decode_all(pool, out, outlen);
            }
            mln_asn1_enresult_destroy(&res);
        }
    }

    mln_alloc_destroy(pool);
    return ret;
}
