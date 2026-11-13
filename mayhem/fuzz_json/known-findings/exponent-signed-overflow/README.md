# JSON decoder: signed `int` overflow on a long exponent

- **Reproducer:** `reproducer.json` (`[1e9999999999]`); run `/mayhem/fuzz_json-standalone reproducer.json`.
- **Cause:** `mln_json_digit_process()` (`src/mln_json.c`, reported at line 945) accumulates the exponent digits with `i *= 10; i += d` in an `int` and never checks for overflow, so an exponent of ten or more digits overflows (UBSan: `signed integer overflow: 999999999 * 10 cannot be represented in type 'int'`). This is a different root cause from `digit-signed-overflow` (that one is the 64-bit mantissa in `mln_json_parse_digit()`).
- **Impact:** undefined behaviour on attacker-controlled JSON text; the wrapped exponent then drives the `for (j = 0; j < i; ++j)` loop, so the result is wrong, and an exponent just below `INT_MAX` makes that loop run about two billion iterations.
- **Fix:** clamp the exponent while accumulating (stop adding digits once `i` exceeds a few hundred, the double range is exhausted by then) or compute the factor with `pow()`.
