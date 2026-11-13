# JSON encoder: out-of-range double cast to a 64-bit integer

- **Reproducer:** `reproducer.json` (`[1e50]`); run `/mayhem/fuzz_json-standalone reproducer.json`. The decoder accepts the number; the failure is in encoding it back.
- **Cause:** `mln_json_write_content()` (`src/mln_json.c`, line 1305) does `mln_s64_t i = (mln_s64_t)(j->data.m_j_number);` before testing whether the double is integral. For a value outside the `int64` range (or `inf`/`nan`, e.g. `[1e531]`) the conversion is undefined behaviour (UBSan: `1e+50 is outside the range of representable values of type 'long'`; Mayhem classifies it as Numeric Truncation Error).
- **Impact:** undefined behaviour when encoding any decoded JSON that holds a large number; on x86-64 the cast yields `INT64_MIN`, so the integral check then fails and the `%f` fallback prints the right text, but the behaviour is not guaranteed.
- **Fix:** test the range first, e.g. `if (d >= -9223372036854775808.0 && d < 9223372036854775808.0 && (mln_s64_t)d == d)`, and use the `%f` path otherwise.
