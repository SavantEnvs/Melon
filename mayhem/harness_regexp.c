/* Fuzz Melon's regular-expression engine.
 * Input: pattern and subject separated by the first NUL byte (no NUL: whole input is the pattern). */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "mln_regexp.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint8_t *nul = memchr(data, 0, size);
    size_t plen = nul ? (size_t)(nul - data) : size;
    size_t tlen = nul ? size - plen - 1 : 0;
    mln_string_t exp, text;
    mln_reg_match_result_t *res;
    int rc;

    if (plen == 0) return -1;
    mln_string_nset(&exp, (mln_u8ptr_t)data, plen);
    mln_string_nset(&text, (mln_u8ptr_t)(nul ? nul + 1 : data + size), tlen);

    if ((res = mln_reg_match_result_new(4)) == NULL) return -1;
    rc = mln_reg_match(&exp, &text, res);
    mln_reg_match_result_free(res);
    (void)mln_reg_equal(&exp, &text);
    return rc < 0 ? -1 : 0;
}
