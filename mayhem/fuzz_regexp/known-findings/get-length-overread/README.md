# Regexp: one-byte heap over-read in `mln_get_length`

- **Reproducer:** `reproducer.bin` (the pattern `-?`, no subject); run `/mayhem/fuzz_regexp-standalone reproducer.bin`.
- **Cause:** `mln_get_length(char *s, int len)` (`src/mln_regexp.c`) reads `s[0]` and compares it with `'\\'` before it knows `len > 0`; `mln_match_here` calls it with `len == 0` for a pattern that ends right after an operator, so it reads one byte past the pattern buffer (ASan heap-buffer-overflow READ of size 1).
- **Impact:** out-of-bounds read of attacker-controlled pattern memory.
- **Fix:** test the length first: `if (len > 1 && s[0] == '\\')`, and return 1 only when `len > 0`.
