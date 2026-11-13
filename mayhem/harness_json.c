/* Fuzz Melon's JSON decoder, encoder and path fetch.
 * Input: JSON text; an optional NUL byte splits it from a mln_json_fetch() path expression. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "mln_json.h"

static int fetch_cb(mln_json_t *j, void *data)
{
    (void)data;
    return mln_json_is_none(j) ? -1 : 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint8_t *nul = memchr(data, 0, size);
    size_t jlen = nul ? (size_t)(nul - data) : size;
    mln_string_t text, path, *enc;
    mln_json_t j;

    mln_string_nset(&text, (mln_u8ptr_t)data, jlen);
    mln_json_init(&j);
    if (mln_json_decode(&text, &j, NULL) < 0) return -1;

    if (nul != NULL) {
        mln_string_nset(&path, (mln_u8ptr_t)nul + 1, size - jlen - 1);
        mln_json_fetch(&j, &path, fetch_cb, NULL);
    }

    if ((enc = mln_json_encode(&j, M_JSON_ENCODE_DEFAULT)) != NULL) {
        mln_json_t j2;
        mln_json_init(&j2);
        if (mln_json_decode(enc, &j2, NULL) == 0) mln_json_destroy(&j2);
        mln_string_free(enc);
    }
    if ((enc = mln_json_encode(&j, M_JSON_ENCODE_UNICODE)) != NULL) mln_string_free(enc);

    mln_json_destroy(&j);
    return 0;
}
