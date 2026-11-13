# JSON decoder: signed integer overflow on a long integer literal

- **Reproducer:** `reproducer.json` (`88888888888888888888`); run `/mayhem/fuzz_json-standalone reproducer.json`.
- **Cause:** `mln_json_parse_digit()` (`src/mln_json.c`) accumulates the integer part with `n = n * 10 + d` in an `mln_s64_t` and never checks for overflow, so a number with 20 digits overflows a signed 64-bit integer (UBSan: `signed integer overflow: 8888888888888888888 * 10`).
- **Impact:** undefined behaviour on attacker-controlled JSON text; the stored value is wrong (wraps in practice).
- **Fix:** bail out (or fall back to the real/double representation) when `n > (INT64_MAX - d) / 10` before the multiply-add.
