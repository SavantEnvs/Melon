# Melang: null `mln_lang_exp_t` dereference in `mln_lang_generate_jump_ptr`

- **Reproducer:** `reproducer.mln` (`;0=l;`); run `/mayhem/fuzz_lang-standalone reproducer.mln`.
- **Cause:** the script parses an assignment whose left-hand side is the constant `0`; code generation then runs `mln_lang_generate_jump_ptr()` (`src/mln_lang.c`, via `mln_lang_stack_handler_stm`) on a statement whose `mln_lang_exp_t` is NULL and accesses a member of it (UBSan: member access within null pointer of type `mln_lang_exp_t`).
- **Impact:** a crash of the interpreter from a 5-byte script (denial of service for any application embedding Melang with untrusted scripts).
- **Fix:** return early from `mln_lang_generate_jump_ptr()` (or reject the assignment at parse time) when the expression pointer is NULL.
