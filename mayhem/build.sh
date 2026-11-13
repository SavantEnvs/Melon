#!/usr/bin/env bash
#
# mayhem/build.sh — builds Melon three ways:
#   1. a sanitized + SanCov-instrumented static libmelon (mayhem/fuzz-build/libmelon_fuzz.a)
#      linked into each libFuzzer harness and its -standalone reproducer,
#   2. the harnesses themselves (/mayhem/fuzz_*, /mayhem/fuzz_*-standalone),
#   3. upstream's own build and test programs with NORMAL flags (lib/, bin/) for mayhem/test.sh.
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
OUT=/mayhem
FB="$SRC/mayhem/fuzz-build"

# 1) Sanitized library. The same -D set upstream's ./configure picks on Linux, minus
#    -DMLN_CONSTRUCTOR: that constructor phones home to a remote server at process start.
#    SanCov goes on the library unconditionally (-fsanitize=fuzzer-no-link) so the fuzzer sees its edges.
rm -rf "$FB"
mkdir -p "$FB/obj"
MLN_DEFS=(-DMLN_ROOT=\"/usr/local/melon\" -DMLN_NULL=\"/dev/null\" -DMLN_LANG_LIB=\"/usr/local/lib/melang\"
          -DMLN_LANG_DYLIB=\"/usr/local/lib/melang_dynamic\" -DMLN_EPOLL -DMLN_SENDFILE -DMLN_WRITEV
          -DMLN_USE_UNIX98 -DMLN_MMAP)
# Relaxed UBSan checks (everything else, and all of ASan, stays fatal):
#  - function: Melon casts handlers like mln_alloc_m/mln_array free callbacks to a different pointer type
#    all over the library; the check halts on the very first input of every target.
#  - alignment: mln_alloc_m() hands out blocks at arbitrary byte offsets, so every pool-allocated struct is
#    "misaligned" at its first member access (harmless on x86, and it halts the first input).
#  - array-bounds, mln_alloc.c only: `pool->mgr_tbl + M_ALLOC_MGR_LEN` expands (unparenthesised macro) to
#    `mgr_tbl + 18*G-(G-1)`, a transient out-of-range pointer that halts every mln_alloc_destroy().
SAN="$SANITIZER_FLAGS -fno-sanitize=function,alignment"
pids=()
for f in src/*.c; do
  extra=""
  [ "$(basename "$f")" = mln_alloc.c ] && extra="-fno-sanitize=array-bounds"
  while [ "$(jobs -rp | wc -l)" -ge "$MAYHEM_JOBS" ]; do wait -n || true; done
  "$CC" -Iinclude -O1 -fPIC -Wall "${MLN_DEFS[@]}" $SAN $extra $DEBUG_FLAGS -fsanitize=fuzzer-no-link \
        -c "$f" -o "$FB/obj/$(basename "${f%.c}").o" &
  pids+=($!)
done
for pid in "${pids[@]}"; do wait "$pid"; done
ar rcs "$FB/libmelon_fuzz.a" "$FB"/obj/*.o

# 2) Harnesses: fuzzer binary + non-fuzzer standalone reproducer; LSan is switched off at build time.
$CXX $SAN $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$FB/lsan_off.o"
$CC $SAN $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$FB/standalone_main.o"
for h in json expr lang asn1 regexp; do
  $CC -Iinclude -O1 $SAN $DEBUG_FLAGS $LIB_FUZZING_ENGINE \
      "mayhem/harness_$h.c" "$FB/lsan_off.o" "$FB/libmelon_fuzz.a" -lpthread -ldl -lm \
      -o "$OUT/fuzz_$h"
  $CC -Iinclude -O1 $SAN $DEBUG_FLAGS \
      "mayhem/harness_$h.c" "$FB/standalone_main.o" "$FB/lsan_off.o" "$FB/libmelon_fuzz.a" -lpthread -ldl -lm \
      -o "$OUT/fuzz_$h-standalone"
done

# 3) Upstream's own build and test programs, NORMAL flags (no sanitizers), for mayhem/test.sh.
#    (env -u: keep the fuzz flags out of upstream's configure; make's objs/lib/bin/Makefile are gitignored upstream.)
rm -rf objs lib bin Makefile
env -u LDFLAGS -u CXXFLAGS CFLAGS="$COVERAGE_FLAGS" ./configure --disable-macro=constructor_flag >/dev/null
make compile -j"$MAYHEM_JOBS" >/dev/null
make test -j"$MAYHEM_JOBS" >/dev/null
