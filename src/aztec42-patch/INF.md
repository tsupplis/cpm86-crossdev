# Infinity support

Infinity support is included in the single `runtime.patch` and installed math
libraries. No incremental overlay is required.

The acceptance target is the complete cpm86-ports/expr regression, including
its infinity tests. Run from the crossdev repository:

    make -C src/aztecreg inf-check
    make -C src/aztecreg expr-inf-check

The second target defaults to the sibling cpm86-ports/expr directory. Override
it with `EXPR_SOURCE=/absolute/path/to/expr`. Both targets compile test sources in a temporary directory and link the
installed libraries directly. The expr target defines REQUIRE_INFINITY and rejects
skipped infinity tests, as well as running the examples and arithmetic probe
from expr's normal make test target.

## Changes

- Software binary32/binary64 transport preserves infinity and NaN, and preserves
  signed zero when loading, storing and negating.
- Addition, subtraction, multiplication and division handle special operands:
  finite nonzero / zero gives signed infinity; 0/0, infinity/infinity,
  infinity-infinity and infinity*zero give NaN; NaNs propagate.
- Overflow detection reserves exponent 2047 for infinity; the internal
  classification entry point now identifies infinity and NaN.
- Signed zeros compare equal. Infinity compares correctly with finite numbers.
- log/log10 return negative infinity for zero, NaN for negative inputs, and
  preserve positive infinity and NaN.

## Validation and limits

The arithmetic probe and existing runtime regression pass on CP/M-86, DOS 1.1
and all four DOS 2 memory models. The combined 8087/software source assembles
in all models, but hardware execution has not been validated.

The strengthened expr suite passes 11,298 checks on CP/M-86 and the host,
including 36 infinity checks. The CP/M make test examples and arithmetic probe
also pass. Small-model DOS 1.1 and DOS 2 expr smoke tests pass too. Expr stores
function pointers in data pointers; its existing mixed-code/data-model casts
are unsuitable for claiming coverage of those models. The runtime itself is
covered across all six targets by inf-check.

**This is not full IEEE-754 support.** Aztec's software `$dcmp` interface returns
an ordered integer. It has no unordered result that makes all four relational
operators false while making NaN unequal to itself. Consequently ordinary C
NaN comparisons remain incorrect; use isnan for classification. The expr
cross-checks now assert NaN classification instead of using x != x to skip
those cases, and explicitly assert infinity rather than accepting any value
for which x-x is nonzero. No regression assertions are disabled.

    make -C src/aztecreg inf-unordered-check

This diagnostic intentionally fails until the compiler/comparison convention
is addressed. Gradual underflow, complete special-value behavior in every
math function, compiler-folded constant expressions, and standard inf/nan text
formatting remain outside the supported runtime behavior. The vintage formatter uses stars for
infinity and question marks for NaN. The arithmetic probe uses volatile
operands so it tests the runtime rather than compiler constant folding.
