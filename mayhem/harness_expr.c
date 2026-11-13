/* Fuzz Melon's expression evaluator (mln_expr_run) with a resolver that supplies typed values. */
#include <stddef.h>
#include <stdint.h>
#include "mln_expr.h"

static mln_expr_val_t *resolve(mln_string_t *ns, mln_string_t *name, int is_func,
                               mln_array_t *args, void *data)
{
    mln_expr_val_t *v;
    mln_string_t *s;
    mln_s64_t i;
    double r;
    mln_u8_t b;
    (void)ns; (void)data;

    if (is_func) {
        i = (mln_s64_t)mln_array_nelts(args);
        return mln_expr_val_new(mln_expr_type_int, &i, NULL);
    }
    if (name->len > 0) {
        switch (name->data[0]) {
            case 'b': b = (mln_u8_t)(name->len & 1); return mln_expr_val_new(mln_expr_type_bool, &b, NULL);
            case 'i': i = (mln_s64_t)name->len * 7; return mln_expr_val_new(mln_expr_type_int, &i, NULL);
            case 'r': r = (double)name->len / 4.0; return mln_expr_val_new(mln_expr_type_real, &r, NULL);
            case 'n': return mln_expr_val_new(mln_expr_type_null, NULL, NULL);
            default: break;
        }
    }
    if ((s = mln_string_dup(name)) == NULL) return NULL;
    v = mln_expr_val_new(mln_expr_type_string, s, NULL);
    mln_string_free(s);
    return v;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    mln_string_t exp;
    mln_expr_val_t *res;

    mln_string_nset(&exp, (mln_u8ptr_t)data, size);
    res = mln_expr_run(&exp, resolve, NULL);
    if (res == NULL) return -1;
    mln_expr_val_free(res);
    return 0;
}
