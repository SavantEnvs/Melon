#!/usr/bin/env bash
#
# mayhem/test.sh — RUN Melon's own test programs (built by mayhem/build.sh into bin/, against lib/libmelon.so)
# and map each program's own summary line to CTRF counts. A program that exits non-zero, or never prints
# its summary line (a crash, or a patch that makes it exit(0) early), counts as a failure.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}


LD_LIBRARY_PATH="$SRC/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH

PASSED=0
FAILED=0

# run_suite <program> <summary ERE> <mode>
#   mode pf: groups (passed)(failed)   pt: groups (passed)(total)   p: group (passed)   one: whole-line match = 1 pass
run_suite() {
  local prog="$1" ere="$2" mode="$3" out rc line p=0 f=0
  if [ ! -x "bin/$prog" ]; then
    echo "FAIL $prog: bin/$prog missing (build.sh should have built it)" >&2
    FAILED=$((FAILED + 1)); return
  fi
  out="$(timeout 300 "bin/$prog" 2>&1)"; rc=$?
  line="$(grep -E "$ere" <<<"$out" | tail -n 1)"
  if [ -z "$line" ] || ! [[ "$line" =~ $ere ]]; then
    echo "FAIL $prog: no summary line matching /$ere/ (rc=$rc)" >&2
    tail -n 5 <<<"$out" >&2
    FAILED=$((FAILED + 1)); return
  fi
  case "$mode" in
    pf)  p="${BASH_REMATCH[1]}"; f="${BASH_REMATCH[2]}" ;;
    pt)  p="${BASH_REMATCH[1]}"; f=$(( BASH_REMATCH[2] - BASH_REMATCH[1] )) ;;
    p)   p="${BASH_REMATCH[1]}" ;;
    one) p=1 ;;
  esac
  if [ "$rc" -ne 0 ]; then f=$((f + 1)); echo "FAIL $prog: exit status $rc" >&2; fi
  if [ "$p" -eq 0 ]; then f=$((f + 1)); echo "FAIL $prog: summary reports zero passing tests" >&2; fi
  echo "$prog: $p passed, $f failed"
  PASSED=$((PASSED + p)); FAILED=$((FAILED + f))
}

run_suite json             'Results: ([0-9]+) passed, ([0-9]+) failed'      pf
run_suite string           'Results: ([0-9]+) passed, ([0-9]+) failed'      pf
run_suite event            'Results: ([0-9]+) passed, ([0-9]+) failed'      pf
run_suite lang             'Results: ([0-9]+) passed, ([0-9]+) failed'      pf
run_suite expr             'Results: ([0-9]+)/([0-9]+) passed'              pt
run_suite hash             'Results: ([0-9]+)/([0-9]+) passed'              pt
run_suite regexp           'Results: ([0-9]+)/([0-9]+) tests passed'        pt
run_suite tools            'tools test: ([0-9]+) passed, ([0-9]+) failed'   pf
run_suite alloc            'alloc tests: ([0-9]+) passed, ([0-9]+) failed'  pf
run_suite aes              'AES Tests: ([0-9]+) passed, ([0-9]+) failed'    pf
run_suite parser_generator '[0-9]+ tests, ([0-9]+) passed, ([0-9]+) failed'   pf
run_suite cron             'Passed: ([0-9]+)'                               p
run_suite lex              'All ([0-9]+) tests passed'                      p
run_suite md5              'All ([0-9]+) tests passed'                      p
run_suite sha              'All ([0-9]+) tests passed'                      p
run_suite queue            'All ([0-9]+) tests passed'                      p
run_suite rbtree           'All ([0-9]+) tests passed'                      p
run_suite rc               'All ([0-9]+) tests passed'                      p
run_suite fec              'All ([0-9]+) tests passed'                      p
run_suite span             'All ([0-9]+)/([0-9]+) tests passed'             pt
run_suite stack            'All ([0-9]+)/([0-9]+) tests passed'             pt
run_suite asn1             '\[asn1\] all tests passed'                      one

emit_ctrf "melon-tests" "$PASSED" "$FAILED" 0
