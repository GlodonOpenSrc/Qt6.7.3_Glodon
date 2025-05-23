import { assert } from '../../common/util/util.js';
import { Float16Array } from '../../external/petamoriken/float16/float16.js';

import { kBit, kValue } from './constants.js';
import {
  f32,
  f16,
  floatBitsToNumber,
  i32,
  kFloat16Format,
  kFloat32Format,
  u32,
} from './conversion.js';

/**
 * A multiple of 8 guaranteed to be way too large to allocate (just under 8 pebibytes).
 * This is a "safe" integer (ULP <= 1.0) very close to MAX_SAFE_INTEGER.
 *
 * Note: allocations of this size are likely to exceed limitations other than just the system's
 * physical memory, so test cases are also needed to try to trigger "true" OOM.
 */
export const kMaxSafeMultipleOf8 = Number.MAX_SAFE_INTEGER - 7;

/** Round `n` up to the next multiple of `alignment` (inclusive). */
// MAINTENANCE_TODO: Rename to `roundUp`
export function align(n: number, alignment: number): number {
  assert(Number.isInteger(n) && n >= 0, 'n must be a non-negative integer');
  assert(Number.isInteger(alignment) && alignment > 0, 'alignment must be a positive integer');
  return Math.ceil(n / alignment) * alignment;
}

/** Round `n` down to the next multiple of `alignment` (inclusive). */
export function roundDown(n: number, alignment: number): number {
  assert(Number.isInteger(n) && n >= 0, 'n must be a non-negative integer');
  assert(Number.isInteger(alignment) && alignment > 0, 'alignment must be a positive integer');
  return Math.floor(n / alignment) * alignment;
}

/** Clamp a number to the provided range. */
export function clamp(n: number, { min, max }: { min: number; max: number }): number {
  assert(max >= min);
  return Math.min(Math.max(n, min), max);
}

/** @returns 0 if |val| is a subnormal f64 number, otherwise returns |val| */
export function flushSubnormalNumberF64(val: number): number {
  return isSubnormalNumberF64(val) ? 0 : val;
}

/** @returns if number is within subnormal range of f64 */
export function isSubnormalNumberF64(n: number): boolean {
  return n > kValue.f64.negative.max && n < kValue.f64.positive.min;
}

/** @returns 0 if |val| is a subnormal f32 number, otherwise returns |val| */
export function flushSubnormalNumberF32(val: number): number {
  return isSubnormalNumberF32(val) ? 0 : val;
}

/** @returns if number is within subnormal range of f32 */
export function isSubnormalNumberF32(n: number): boolean {
  return n > kValue.f32.negative.max && n < kValue.f32.positive.min;
}

/** @returns if number is in the finite range of f32 */
export function isFiniteF32(n: number) {
  return n >= kValue.f32.negative.min && n <= kValue.f32.positive.max;
}

/** @returns 0 if |val| is a subnormal f16 number, otherwise returns |val| */
export function flushSubnormalNumberF16(val: number): number {
  return isSubnormalNumberF16(val) ? 0 : val;
}

/** @returns if number is within subnormal range of f16 */
export function isSubnormalNumberF16(n: number): boolean {
  return n > kValue.f16.negative.max && n < kValue.f16.positive.min;
}

/** @returns if number is in the finite range of f16 */
export function isFiniteF16(n: number) {
  return n >= kValue.f16.negative.min && n <= kValue.f16.positive.max;
}

/** Should FTZ occur during calculations or not */
export type FlushMode = 'flush' | 'no-flush';

/** Should nextAfter calculate towards positive infinity or negative infinity */
export type NextDirection = 'positive' | 'negative';

/**
 * Once-allocated ArrayBuffer/views to avoid overhead of allocation when
 * converting between numeric formats
 *
 * Usage of a once-allocated pattern like this makes nextAfterF64 non-reentrant,
 * so cannot call itself directly or indirectly.
 */
const nextAfterF64Data = new ArrayBuffer(8);
const nextAfterF64Int = new BigUint64Array(nextAfterF64Data);
const nextAfterF64Float = new Float64Array(nextAfterF64Data);

/**
 * @returns the next f64 value after |val|, towards +inf or -inf as specified by |dir|.

 * If |mode| is 'flush', all subnormal values will be flushed to 0,
 * before processing and for -/+0 the nextAfterF64 will be the closest normal in
 * the correct direction.

 * If |mode| is 'no-flush', the next subnormal will be calculated when appropriate,
 * and for -/+0 the nextAfterF64 will be the closest subnormal in the correct
 * direction.
 *
 * val needs to be in [min f64, max f64]
 */
export function nextAfterF64(val: number, dir: NextDirection, mode: FlushMode): number {
  if (Number.isNaN(val)) {
    return val;
  }

  if (val === Number.POSITIVE_INFINITY) {
    return kValue.f64.infinity.positive;
  }

  if (val === Number.NEGATIVE_INFINITY) {
    return kValue.f64.infinity.negative;
  }

  assert(
    val <= kValue.f64.positive.max && val >= kValue.f64.negative.min,
    `${val} is not in the range of f64`
  );

  val = mode === 'flush' ? flushSubnormalNumberF64(val) : val;

  // -/+0 === 0 returns true
  if (val === 0) {
    if (dir === 'positive') {
      return mode === 'flush' ? kValue.f64.positive.min : kValue.f64.subnormal.positive.min;
    } else {
      return mode === 'flush' ? kValue.f64.negative.max : kValue.f64.subnormal.negative.max;
    }
  }

  nextAfterF64Float[0] = val;
  const is_positive = (nextAfterF64Int[0] & 0x8000_0000_0000_0000n) === 0n;
  if (is_positive === (dir === 'positive')) {
    nextAfterF64Int[0] += 1n;
  } else {
    nextAfterF64Int[0] -= 1n;
  }

  // Checking for overflow
  if ((nextAfterF64Int[0] & 0x7ff0_0000_0000_0000n) === 0x7ff0_0000_0000_0000n) {
    if (dir === 'positive') {
      return kValue.f64.infinity.positive;
    } else {
      return kValue.f64.infinity.negative;
    }
  }

  return mode === 'flush' ? flushSubnormalNumberF64(nextAfterF64Float[0]) : nextAfterF64Float[0];
}

/**
 * Once-allocated ArrayBuffer/views to avoid overhead of allocation when
 * converting between numeric formats
 *
 * Usage of a once-allocated pattern like this makes nextAfterF32 non-reentrant,
 * so cannot call itself directly or indirectly.
 */
const nextAfterF32Data = new ArrayBuffer(4);
const nextAfterF32Int = new Uint32Array(nextAfterF32Data);
const nextAfterF32Float = new Float32Array(nextAfterF32Data);

/**
 * @returns the next f32 value after |val|, towards +inf or -inf as specified by |dir|.

 * If |mode| is 'flush', all subnormal values will be flushed to 0,
 * before processing and for -/+0 the nextAfterF32 will be the closest normal in
 * the correct direction.

 * If |mode| is 'no-flush', the next subnormal will be calculated when appropriate,
 * and for -/+0 the nextAfterF32 will be the closest subnormal in the correct
 * direction.
 *
 * val needs to be in [min f32, max f32]
 */
export function nextAfterF32(val: number, dir: NextDirection, mode: FlushMode): number {
  if (Number.isNaN(val)) {
    return val;
  }

  if (val === Number.POSITIVE_INFINITY) {
    return kValue.f32.infinity.positive;
  }

  if (val === Number.NEGATIVE_INFINITY) {
    return kValue.f32.infinity.negative;
  }

  assert(
    val <= kValue.f32.positive.max && val >= kValue.f32.negative.min,
    `${val} is not in the range of f32`
  );

  val = mode === 'flush' ? flushSubnormalNumberF32(val) : val;

  // -/+0 === 0 returns true
  if (val === 0) {
    if (dir === 'positive') {
      return mode === 'flush' ? kValue.f32.positive.min : kValue.f32.subnormal.positive.min;
    } else {
      return mode === 'flush' ? kValue.f32.negative.max : kValue.f32.subnormal.negative.max;
    }
  }

  nextAfterF32Float[0] = val; // This quantizes from number (f64) to f32
  if (
    (dir === 'positive' && nextAfterF32Float[0] <= val) ||
    (dir === 'negative' && nextAfterF32Float[0] >= val)
  ) {
    // val is either f32 precise or quantizing rounded in the opposite direction
    // from what is needed, so need to calculate the value in the correct
    // direction.
    const is_positive = (nextAfterF32Int[0] & 0x80000000) === 0;
    if (is_positive === (dir === 'positive')) {
      nextAfterF32Int[0] += 1;
    } else {
      nextAfterF32Int[0] -= 1;
    }
  }

  // Checking for overflow
  if ((nextAfterF32Int[0] & 0x7f800000) === 0x7f800000) {
    if (dir === 'positive') {
      return kValue.f32.infinity.positive;
    } else {
      return kValue.f32.infinity.negative;
    }
  }

  return mode === 'flush' ? flushSubnormalNumberF32(nextAfterF32Float[0]) : nextAfterF32Float[0];
}

/**
 * Once-allocated ArrayBuffer/views to avoid overhead of allocation when
 * converting between numeric formats
 *
 * Usage of a once-allocated pattern like this makes nextAfterF16 non-reentrant,
 * so cannot call itself directly or indirectly.
 */
const nextAfterF16Data = new ArrayBuffer(2);
const nextAfterF16Hex = new Uint16Array(nextAfterF16Data);
const nextAfterF16Float = new Float16Array(nextAfterF16Data);

/**
 * @returns the next f16 value after |val|, towards +inf or -inf as specified by |dir|.

 * If |mode| is 'flush', all subnormal values will be flushed to 0,
 * before processing and for -/+0 the nextAfterF16 will be the closest normal in
 * the correct direction.

 * If |mode| is 'no-flush', the next subnormal will be calculated when appropriate,
 * and for -/+0 the nextAfterF16 will be the closest subnormal in the correct
 * direction.
 *
 * val needs to be in [min f16, max f16]
 */
export function nextAfterF16(val: number, dir: NextDirection, mode: FlushMode): number {
  if (Number.isNaN(val)) {
    return val;
  }

  if (val === Number.POSITIVE_INFINITY) {
    return kValue.f16.infinity.positive;
  }

  if (val === Number.NEGATIVE_INFINITY) {
    return kValue.f16.infinity.negative;
  }

  assert(
    val <= kValue.f16.positive.max && val >= kValue.f16.negative.min,
    `${val} is not in the range of f16`
  );

  val = mode === 'flush' ? flushSubnormalNumberF16(val) : val;

  // -/+0 === 0 returns true
  if (val === 0) {
    if (dir === 'positive') {
      return mode === 'flush' ? kValue.f16.positive.min : kValue.f16.subnormal.positive.min;
    } else {
      return mode === 'flush' ? kValue.f16.negative.max : kValue.f16.subnormal.negative.max;
    }
  }

  nextAfterF16Float[0] = val; // This quantizes from number (f64) to f16
  if (
    (dir === 'positive' && nextAfterF16Float[0] <= val) ||
    (dir === 'negative' && nextAfterF16Float[0] >= val)
  ) {
    // val is either f16 precise or quantizing rounded in the opposite direction
    // from what is needed, so need to calculate the value in the correct
    // direction.
    const is_positive = (nextAfterF16Hex[0] & 0x8000) === 0;
    if (is_positive === (dir === 'positive')) {
      nextAfterF16Hex[0] += 1;
    } else {
      nextAfterF16Hex[0] -= 1;
    }
  }

  // Checking for overflow
  if ((nextAfterF16Hex[0] & 0x7c00) === 0x7c00) {
    if (dir === 'positive') {
      return kValue.f16.infinity.positive;
    } else {
      return kValue.f16.infinity.negative;
    }
  }

  return mode === 'flush' ? flushSubnormalNumberF16(nextAfterF16Float[0]) : nextAfterF16Float[0];
}

/**
 * @returns ulp(x), the unit of least precision for a specific number as a 64-bit float
 *
 * ulp(x) is the distance between the two floating point numbers nearest x.
 * This value is also called unit of last place, ULP, and 1 ULP.
 * See the WGSL spec and http://www.ens-lyon.fr/LIP/Pub/Rapports/RR/RR2005/RR2005-09.pdf
 * for a more detailed/nuanced discussion of the definition of ulp(x).
 *
 * @param target number to calculate ULP for
 * @param mode should FTZ occurring during calculation or not
 */
export function oneULPF64(target: number, mode: FlushMode = 'flush'): number {
  if (Number.isNaN(target)) {
    return Number.NaN;
  }

  target = mode === 'flush' ? flushSubnormalNumberF64(target) : target;

  // For values out of bounds for f64 ulp(x) is defined as the
  // distance between the two nearest f64 representable numbers to the
  // appropriate edge, which also happens to be the maximum possible ULP.
  if (
    target === Number.POSITIVE_INFINITY ||
    target >= kValue.f64.positive.max ||
    target === Number.NEGATIVE_INFINITY ||
    target <= kValue.f64.negative.min
  ) {
    return kValue.f64.max_ulp;
  }

  // ulp(x) is min(after - before), where
  //     before <= x <= after
  //     before =/= after
  //     before and after are f64 representable
  const before = nextAfterF64(target, 'negative', mode);
  const after = nextAfterF64(target, 'positive', mode);
  // Since number is internally a f64, |target| is always f64 representable, so
  // either before or after will be x
  return Math.min(target - before, after - target);
}

/**
 * @returns ulp(x), the unit of least precision for a specific number as a 32-bit float
 *
 * ulp(x) is the distance between the two floating point numbers nearest x.
 * This value is also called unit of last place, ULP, and 1 ULP.
 * See the WGSL spec and http://www.ens-lyon.fr/LIP/Pub/Rapports/RR/RR2005/RR2005-09.pdf
 * for a more detailed/nuanced discussion of the definition of ulp(x).
 *
 * @param target number to calculate ULP for
 * @param mode should FTZ occurring during calculation or not
 */
export function oneULPF32(target: number, mode: FlushMode = 'flush'): number {
  if (Number.isNaN(target)) {
    return Number.NaN;
  }

  target = mode === 'flush' ? flushSubnormalNumberF32(target) : target;

  // For values out of bounds for f32 ulp(x) is defined as the
  // distance between the two nearest f32 representable numbers to the
  // appropriate edge, which also happens to be the maximum possible ULP.
  if (
    target === Number.POSITIVE_INFINITY ||
    target >= kValue.f32.positive.max ||
    target === Number.NEGATIVE_INFINITY ||
    target <= kValue.f32.negative.min
  ) {
    return kValue.f32.max_ulp;
  }

  // ulp(x) is min(after - before), where
  //     before <= x <= after
  //     before =/= after
  //     before and after are f32 representable
  const before = nextAfterF32(target, 'negative', mode);
  const after = nextAfterF32(target, 'positive', mode);
  const converted: number = quantizeToF32(target);
  if (converted === target) {
    // |target| is f32 representable, so either before or after will be x
    return Math.min(target - before, after - target);
  } else {
    // |target| is not f32 representable so taking distance of neighbouring f32s.
    return after - before;
  }
}

/**
 * @returns ulp(x), the unit of least precision for a specific number as a 32-bit float
 *
 * ulp(x) is the distance between the two floating point numbers nearest x.
 * This value is also called unit of last place, ULP, and 1 ULP.
 * See the WGSL spec and http://www.ens-lyon.fr/LIP/Pub/Rapports/RR/RR2005/RR2005-09.pdf
 * for a more detailed/nuanced discussion of the definition of ulp(x).
 *
 * @param target number to calculate ULP for
 * @param mode should FTZ occurring during calculation or not
 */
export function oneULPF16(target: number, mode: FlushMode = 'flush'): number {
  if (Number.isNaN(target)) {
    return Number.NaN;
  }

  target = mode === 'flush' ? flushSubnormalNumberF16(target) : target;

  // For values out of bounds for f16 ulp(x) is defined as the
  // distance between the two nearest f16 representable numbers to the
  // appropriate edge, which also happens to be the maximum possible ULP.
  if (
    target === Number.POSITIVE_INFINITY ||
    target >= kValue.f16.positive.max ||
    target === Number.NEGATIVE_INFINITY ||
    target <= kValue.f16.negative.min
  ) {
    return kValue.f16.max_ulp;
  }

  // ulp(x) is min(after - before), where
  //     before <= x <= after
  //     before =/= after
  //     before and after are f16 representable
  const before = nextAfterF16(target, 'negative', mode);
  const after = nextAfterF16(target, 'positive', mode);
  const converted: number = quantizeToF16(target);
  if (converted === target) {
    // |target| is f16 representable, so either before or after will be x
    return Math.min(target - before, after - target);
  } else {
    // |target| is not f16 representable so taking distance of neighbouring f16s.
    return after - before;
  }
}

/**
 * Calculate the valid roundings when quantizing to 64-bit floats
 *
 * TS/JS's number type is internally a f64, so the supplied value will be
 * quanitized by definition. The only corner cases occur if a non-finite value
 * is provided, since the valid roundings include the appropriate min or max
 * value.
 *
 * @param n number to be quantized
 * @returns all of the acceptable roundings for quantizing to 64-bits in
 *          ascending order.
 */
export function correctlyRoundedF64(n: number): number[] {
  assert(!Number.isNaN(n), `correctlyRoundedF32 not defined for NaN`);
  // Above f64 range
  if (n === Number.POSITIVE_INFINITY) {
    return [kValue.f64.positive.max, Number.POSITIVE_INFINITY];
  }

  // Below f64 range
  if (n === Number.NEGATIVE_INFINITY) {
    return [Number.NEGATIVE_INFINITY, kValue.f64.negative.min];
  }

  return [n];
}

/**
 * Calculate the valid roundings when quantizing to 32-bit floats
 *
 * TS/JS's number type is internally a f64, so quantization needs to occur when
 * converting to f32 for WGSL. WGSL does not specify a specific rounding mode,
 * so if a number is not precisely representable in 32-bits, but in the
 * range, there are two possible valid quantizations. If it is precisely
 * representable, there is only one valid quantization. This function calculates
 * the valid roundings and returns them in an array.
 *
 * This function does not consider flushing mode, so subnormals are maintained.
 * The caller is responsible to flushing before and after as appropriate.
 *
 * Out of bounds values need to consider how they interact with the overflow
 * rules.
 *  * If a value is OOB but not too far out, an implementation may choose to round
 * to nearest finite value or the correct infinity. This boundary is at
 * 2^(f32.emax + 1) and -(2^(f32.emax + 1)) respectively.
 * Values that are at or beyond these limits must be rounded towards the
 * appropriate infinity.
 *
 * @param n number to be quantized
 * @returns all of the acceptable roundings for quantizing to 32-bits in
 *          ascending order.
 */
export function correctlyRoundedF32(n: number): number[] {
  if (Number.isNaN(n)) {
    return [n];
  }

  // Greater than or equal to the upper overflow boundry
  if (n >= 2 ** (kValue.f32.emax + 1)) {
    return [Number.POSITIVE_INFINITY];
  }

  // OOB, but less than the upper overflow boundary
  if (n > kValue.f32.positive.max) {
    return [kValue.f32.positive.max, Number.POSITIVE_INFINITY];
  }

  // f32 finite
  if (n <= kValue.f32.positive.max && n >= kValue.f32.negative.min) {
    const n_32 = new Float32Array([n])[0];
    const converted: number = n_32;
    if (n === converted) {
      // n is precisely expressible as a f32, so should not be rounded
      return [n];
    }

    if (converted > n) {
      // n_32 rounded towards +inf, so is after n
      const other = nextAfterF32(n_32, 'negative', 'no-flush');
      return [other, converted];
    } else {
      // n_32 rounded towards -inf, so is before n
      const other = nextAfterF32(n_32, 'positive', 'no-flush');
      return [converted, other];
    }
  }

  // OOB, but greater the lower overflow boundary
  if (n > -(2 ** (kValue.f32.emax + 1))) {
    return [Number.NEGATIVE_INFINITY, kValue.f32.negative.min];
  }

  // Less than or equal to the lower overflow boundary
  return [Number.NEGATIVE_INFINITY];
}

/**
 * Calculate the valid roundings when quantizing to 16-bit floats
 *
 * TS/JS's number type is internally a f64, so quantization needs to occur when
 * converting to f16 for WGSL. WGSL does not specify a specific rounding mode,
 * so if a number is not precisely representable in 16-bits, but in the
 * range, there are two possible valid quantizations. If it is precisely
 * representable, there is only one valid quantization. This function calculates
 * the valid roundings and returns them in an array.
 *
 * This function does not consider flushing mode, so subnormals are maintained.
 * The caller is responsible to flushing before and after as appropriate.
 *
 * Out of bounds values need to consider how they interact with the overflow
 * rules.
 *  * If a value is OOB but not too far out, an implementation may choose to round
 * to nearest finite value or the correct infinity. This boundary is at
 * 2^(f16.emax + 1) and -(2^(f16.emax + 1)) respectively.
 * Values that are at or beyond these limits must be rounded towards the
 * appropriate infinity.
 *
 * @param n number to be quantized
 * @returns all of the acceptable roundings for quantizing to 16-bits in
 *          ascending order.
 */
export function correctlyRoundedF16(n: number): number[] {
  if (Number.isNaN(n)) {
    return [n];
  }

  // Greater than or equal to the upper overflow boundry
  if (n >= 2 ** (kValue.f16.emax + 1)) {
    return [Number.POSITIVE_INFINITY];
  }

  // OOB, but less than the upper overflow boundary
  if (n > kValue.f16.positive.max) {
    return [kValue.f16.positive.max, Number.POSITIVE_INFINITY];
  }

  // f16 finite
  if (n <= kValue.f16.positive.max && n >= kValue.f16.negative.min) {
    const n_16 = new Float16Array([n])[0];
    const converted: number = n_16;
    if (n === converted) {
      // n is precisely expressible as a f16, so should not be rounded
      return [n];
    }

    if (converted > n) {
      // n_16 rounded towards +inf, so is after n
      const other = nextAfterF16(n_16, 'negative', 'no-flush');
      return [other, converted];
    } else {
      // n_16 rounded towards -inf, so is before n
      const other = nextAfterF16(n_16, 'positive', 'no-flush');
      return [converted, other];
    }
  }

  // OOB, but greater the lower overflow boundary
  if (n > -(2 ** (kValue.f16.emax + 1))) {
    return [Number.NEGATIVE_INFINITY, kValue.f16.negative.min];
  }

  // Less than or equal to the lower overflow boundary
  return [Number.NEGATIVE_INFINITY];
}

/**
 * Once-allocated ArrayBuffer/views to avoid overhead of allocation in frexp
 *
 * This makes frexp non-reentrant due to shared state between calls.
 */
const frexpData = new ArrayBuffer(4);
const frexpDataU32 = new Uint32Array(frexpData);
const frexpDataF32 = new Float32Array(frexpData);

/**
 * Calculates WGSL frexp
 *
 * Splits val into a fraction and an exponent so that
 * val = fraction * 2 ^ exponent.
 * The fraction is 0.0 or its magnitude is in the range [0.5, 1.0).
 *
 * Inspired by golang's implementation of frexp.
 *
 * This code is non-reentrant due to the use of a non-local data buffer and
 * views.
 *
 * @param val the f32 to split
 * @returns the results of splitting val
 */
export function frexp(val: number): { fract: number; exp: number } {
  frexpDataF32[0] = val;
  // Do not directly use val after this point, so that changes are reflected in
  // both the f32 and u32 views.

  // Handles 0 and -0
  if (frexpDataF32[0] === 0) {
    return { fract: frexpDataF32[0], exp: 0 };
  }

  // Covers NaNs, OOB and Infinities
  if (!isFiniteF32(frexpDataF32[0])) {
    return { fract: frexpDataF32[0], exp: 0 };
  }

  // Normalize if subnormal
  let exp = 0;
  if (isSubnormalNumberF32(frexpDataF32[0])) {
    frexpDataF32[0] = frexpDataF32[0] * (1 << 23);
    exp = -23;
  }
  exp += ((frexpDataU32[0] >> 23) & 0xff) - 126; // shift & mask, minus the bias + 1

  frexpDataU32[0] &= 0x807fffff; // mask the exponent bits
  frexpDataU32[0] |= 0x3f000000; // extract the mantissa bits
  const fract = frexpDataF32[0]; // Convert from bits to number
  return { fract, exp };
}

/**
 * Calculates the linear interpolation between two values of a given fractional.
 *
 * If |t| is 0, |a| is returned, if |t| is 1, |b| is returned, otherwise
 * interpolation/extrapolation equivalent to a + t(b - a) is performed.
 *
 * Numerical stable version is adapted from http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0811r2.html
 */
export function lerp(a: number, b: number, t: number): number {
  if (!Number.isFinite(a) || !Number.isFinite(b)) {
    return Number.NaN;
  }

  if ((a <= 0.0 && b >= 0.0) || (a >= 0.0 && b <= 0.0)) {
    return t * b + (1 - t) * a;
  }

  if (t === 1.0) {
    return b;
  }

  const x = a + t * (b - a);
  return t > 1.0 === b > a ? Math.max(b, x) : Math.min(b, x);
}

/**
 * Version of lerp that operates on bigint values
 *
 * lerp was not made into a generic or to take in (number|bigint), because that
 * introduces a bunch of complexity overhead related to type differentiation
 */
export function lerpBigInt(a: bigint, b: bigint, idx: number, steps: number): bigint {
  assert(Math.trunc(idx) === idx);
  assert(Math.trunc(steps) === steps);

  // This constrains t to [0.0, 1.0]
  assert(idx >= 0);
  assert(steps > 0);
  assert(idx < steps);

  if (steps === 1) {
    return a;
  }
  if (idx === 0) {
    return a;
  }
  if (idx === steps - 1) {
    return b;
  }

  const min = (x: bigint, y: bigint): bigint => {
    return x < y ? x : y;
  };
  const max = (x: bigint, y: bigint): bigint => {
    return x > y ? x : y;
  };

  // For number the variable t is used, there t = idx / (steps - 1),
  // but that is a fraction on [0, 1], so becomes either 0 or 1 when converted
  // to bigint, so need to expand things out.
  const big_idx = BigInt(idx);
  const big_steps = BigInt(steps);
  if ((a <= 0n && b >= 0n) || (a >= 0n && b <= 0n)) {
    return (b * big_idx) / (big_steps - 1n) + (a - (a * big_idx) / (big_steps - 1n));
  }

  const x = a + (b * big_idx) / (big_steps - 1n) - (a * big_idx) / (big_steps - 1n);
  return !(b > a) ? max(b, x) : min(b, x);
}

/** @returns a linear increasing range of numbers. */
export function linearRange(a: number, b: number, num_steps: number): number[] {
  if (num_steps <= 0) {
    return [];
  }

  // Avoid division by 0
  if (num_steps === 1) {
    return [a];
  }

  return Array.from(Array(num_steps).keys()).map(i => lerp(a, b, i / (num_steps - 1)));
}

/**
 * Version of linearRange that operates on bigint values
 *
 * linearRange was not made into a generic or to take in (number|bigint),
 * because that introduces a bunch of complexity overhead related to type
 * differentiation
 */
export function linearRangeBigInt(a: bigint, b: bigint, num_steps: number): Array<bigint> {
  if (num_steps <= 0) {
    return [];
  }

  // Avoid division by 0
  if (num_steps === 1) {
    return [a];
  }

  return Array.from(Array(num_steps).keys()).map(i => lerpBigInt(a, b, i, num_steps));
}

/**
 * @returns a non-linear increasing range of numbers, with a bias towards the beginning.
 *
 * Generates a linear range on [0,1] with |num_steps|, then squares all the values to make the curve be quadratic,
 * thus biasing towards 0, but remaining on the [0, 1] range.
 * This biased range is then scaled to the desired range using lerp.
 * Different curves could be generated by changing c, where greater values of c will bias more towards 0.
 */
export function biasedRange(a: number, b: number, num_steps: number): number[] {
  const c = 2;
  if (num_steps <= 0) {
    return [];
  }

  // Avoid division by 0
  if (num_steps === 1) {
    return [a];
  }

  return Array.from(Array(num_steps).keys()).map(i => lerp(a, b, Math.pow(i / (num_steps - 1), c)));
}

/**
 * @returns an ascending sorted array of numbers spread over the entire range of 32-bit floats
 *
 * Numbers are divided into 4 regions: negative normals, negative subnormals, positive subnormals & positive normals.
 * Zero is included.
 *
 * Numbers are generated via taking a linear spread of the bit field representations of the values in each region. This
 * means that number of precise f32 values between each returned value in a region should be about the same. This allows
 * for a wide range of magnitudes to be generated, instead of being extremely biased towards the edges of the f32 range.
 *
 * This function is intended to provide dense coverage of the f32 range, for a minimal list of values to use to cover
 * f32 behaviour, use sparseF32Range instead.
 *
 * @param counts structure param with 4 entries indicating the number of entries to be generated each region, entries
 *               must be 0 or greater.
 */
export function fullF32Range(
  counts: {
    neg_norm?: number;
    neg_sub?: number;
    pos_sub: number;
    pos_norm: number;
  } = { pos_sub: 10, pos_norm: 50 }
): Array<number> {
  counts.neg_norm = counts.neg_norm === undefined ? counts.pos_norm : counts.neg_norm;
  counts.neg_sub = counts.neg_sub === undefined ? counts.pos_sub : counts.neg_sub;

  // Generating bit fields first and then converting to f32, so that the spread across the possible f32 values is more
  // even. Generating against the bounds of f32 values directly results in the values being extremely biased towards the
  // extremes, since they are so much larger.
  const bit_fields = [
    ...linearRange(kBit.f32.negative.min, kBit.f32.negative.max, counts.neg_norm),
    ...linearRange(
      kBit.f32.subnormal.negative.min,
      kBit.f32.subnormal.negative.max,
      counts.neg_sub
    ),
    0,
    ...linearRange(
      kBit.f32.subnormal.positive.min,
      kBit.f32.subnormal.positive.max,
      counts.pos_sub
    ),
    ...linearRange(kBit.f32.positive.min, kBit.f32.positive.max, counts.pos_norm),
  ].map(Math.trunc);
  return bit_fields.map(reinterpretU32AsF32);
}

/**
 * @returns an ascending sorted array of numbers.
 *
 * The numbers returned are based on the `full32Range` as described above. The difference comes depending
 * on the `source` parameter. If the `source` is `const` then the numbers will be restricted to be
 * in the range `[low, high]`. This allows filtering out a set of `f32` values which are invalid for
 * const-evaluation but are needed to test the non-const implementation.
 *
 * @param source the input source for the test. If the `source` is `const` then the return will be filtered
 * @param low the lowest f32 value to permit when filtered
 * @param high the highest f32 value to permit when filtered
 */
export function sourceFilteredF32Range(source: String, low: number, high: number): Array<number> {
  return fullF32Range().filter(x => source !== 'const' || (x >= low && x <= high));
}

/**
 * @returns an ascending sorted array of numbers spread over the entire range of 16-bit floats
 *
 * Numbers are divided into 4 regions: negative normals, negative subnormals, positive subnormals & positive normals.
 * Zero is included.
 *
 * Numbers are generated via taking a linear spread of the bit field representations of the values in each region. This
 * means that number of precise f16 values between each returned value in a region should be about the same. This allows
 * for a wide range of magnitudes to be generated, instead of being extremely biased towards the edges of the f16 range.
 *
 * This function is intended to provide dense coverage of the f16 range, for a minimal list of values to use to cover
 * f16 behaviour, use sparseF16Range instead.
 *
 * @param counts structure param with 4 entries indicating the number of entries to be generated each region, entries
 *               must be 0 or greater.
 */
export function fullF16Range(
  counts: {
    neg_norm?: number;
    neg_sub?: number;
    pos_sub: number;
    pos_norm: number;
  } = { pos_sub: 10, pos_norm: 50 }
): Array<number> {
  counts.neg_norm = counts.neg_norm === undefined ? counts.pos_norm : counts.neg_norm;
  counts.neg_sub = counts.neg_sub === undefined ? counts.pos_sub : counts.neg_sub;

  // Generating bit fields first and then converting to f16, so that the spread across the possible f16 values is more
  // even. Generating against the bounds of f16 values directly results in the values being extremely biased towards the
  // extremes, since they are so much larger.
  const bit_fields = [
    ...linearRange(kBit.f16.negative.min, kBit.f16.negative.max, counts.neg_norm),
    ...linearRange(
      kBit.f16.subnormal.negative.min,
      kBit.f16.subnormal.negative.max,
      counts.neg_sub
    ),
    0,
    ...linearRange(
      kBit.f16.subnormal.positive.min,
      kBit.f16.subnormal.positive.max,
      counts.pos_sub
    ),
    ...linearRange(kBit.f16.positive.min, kBit.f16.positive.max, counts.pos_norm),
  ].map(Math.trunc);
  return bit_fields.map(reinterpretU16AsF16);
}

/**
 * @returns an ascending sorted array of numbers spread over the entire range of 64-bit floats
 *
 * Numbers are divided into 4 regions: negative normals, negative subnormals, positive subnormals & positive normals.
 * Zero is included.
 *
 * Numbers are generated via taking a linear spread of the bit field representations of the values in each region. This
 * means that number of precise f64 values between each returned value in a region should be about the same. This allows
 * for a wide range of magnitudes to be generated, instead of being extremely biased towards the edges of the f64 range.
 *
 * This function is intended to provide dense coverage of the f64 range, for a minimal list of values to use to cover
 * f64 behaviour, use sparseF64Range instead.
 *
 * @param counts structure param with 4 entries indicating the number of entries to be generated each region, entries
 *               must be 0 or greater.
 */
export function fullF64Range(
  counts: {
    neg_norm?: number;
    neg_sub?: number;
    pos_sub: number;
    pos_norm: number;
  } = { pos_sub: 10, pos_norm: 50 }
): Array<number> {
  counts.neg_norm = counts.neg_norm === undefined ? counts.pos_norm : counts.neg_norm;
  counts.neg_sub = counts.neg_sub === undefined ? counts.pos_sub : counts.neg_sub;

  // Generating bit fields first and then converting to f64, so that the spread across the possible f64 values is more
  // even. Generating against the bounds of f64 values directly results in the values being extremely biased towards the
  // extremes, since they are so much larger.
  const bit_fields = [
    ...linearRangeBigInt(kBit.f64.negative.min, kBit.f64.negative.max, counts.neg_norm),
    ...linearRangeBigInt(
      kBit.f64.subnormal.negative.min,
      kBit.f64.subnormal.negative.max,
      counts.neg_sub
    ),
    0n,
    ...linearRangeBigInt(
      kBit.f64.subnormal.positive.min,
      kBit.f64.subnormal.positive.max,
      counts.pos_sub
    ),
    ...linearRangeBigInt(kBit.f64.positive.min, kBit.f64.positive.max, counts.pos_norm),
  ];
  return bit_fields.map(reinterpretU64AsF64);
}

/**
 * @returns an ascending sorted array of f64 values spread over specific range of f64 normal floats
 *
 * Numbers are divided into 4 regions: negative 64-bit normals, negative 64-bit subnormals, positive 64-bit subnormals &
 * positive 64-bit normals.
 * Zero is included.
 *
 * Numbers are generated via taking a linear spread of the bit field representations of the values in each region. This
 * means that number of precise f64 values between each returned value in a region should be about the same. This allows
 * for a wide range of magnitudes to be generated, instead of being extremely biased towards the edges of the range.
 *
 * @param begin a negative f64 normal float value
 * @param end a positive f64 normal float value
 * @param counts structure param with 4 entries indicating the number of entries
 *               to be generated each region, entries must be 0 or greater.
 */
export function filteredF64Range(
  begin: number,
  end: number,
  counts: { neg_norm?: number; neg_sub?: number; pos_sub: number; pos_norm: number } = {
    pos_sub: 10,
    pos_norm: 50,
  }
): Array<number> {
  assert(
    begin <= kValue.f64.negative.max,
    `Beginning of range ${begin} must be negative f64 normal`
  );
  assert(end >= kValue.f64.positive.min, `Ending of range ${end} must be positive f64 normal`);

  counts.neg_norm = counts.neg_norm === undefined ? counts.pos_norm : counts.neg_norm;
  counts.neg_sub = counts.neg_sub === undefined ? counts.pos_sub : counts.neg_sub;

  const u64_begin = reinterpretF64AsU64(begin);
  const u64_end = reinterpretF64AsU64(end);
  // Generating bit fields first and then converting to f64, so that the spread across the possible f64 values is more
  // even. Generating against the bounds of f64 values directly results in the values being extremely biased towards the
  // extremes, since they are so much larger.
  const bit_fields = [
    ...linearRangeBigInt(u64_begin, kBit.f64.negative.max, counts.neg_norm),
    ...linearRangeBigInt(
      kBit.f64.subnormal.negative.min,
      kBit.f64.subnormal.negative.max,
      counts.neg_sub
    ),
    0n,
    ...linearRangeBigInt(
      kBit.f64.subnormal.positive.min,
      kBit.f64.subnormal.positive.max,
      counts.pos_sub
    ),
    ...linearRangeBigInt(kBit.f64.positive.min, u64_end, counts.pos_norm),
  ];
  return bit_fields.map(reinterpretU64AsF64);
}

/** Short list of i32 values of interest to test against */
const kInterestingI32Values: number[] = [
  kValue.i32.negative.max,
  Math.trunc(kValue.i32.negative.max / 2),
  -256,
  -10,
  -1,
  0,
  1,
  10,
  256,
  Math.trunc(kValue.i32.positive.max / 2),
  kValue.i32.positive.max,
];

/** @returns minimal i32 values that cover the entire range of i32 behaviours
 *
 * This is used instead of fullI32Range when the number of test cases being
 * generated is a super linear function of the length of i32 values which is
 * leading to time outs.
 */
export function sparseI32Range(): number[] {
  return kInterestingI32Values;
}

const kVectorI32Values = {
  2: kInterestingI32Values.flatMap(f => [
    [f, 1],
    [1, f],
    [f, -1],
    [-1, f],
  ]),
  3: kInterestingI32Values.flatMap(f => [
    [f, 1, 2],
    [1, f, 2],
    [1, 2, f],
    [f, -1, -2],
    [-1, f, -2],
    [-1, -2, f],
  ]),
  4: kInterestingI32Values.flatMap(f => [
    [f, 1, 2, 3],
    [1, f, 2, 3],
    [1, 2, f, 3],
    [1, 2, 3, f],
    [f, -1, -2, -3],
    [-1, f, -2, -3],
    [-1, -2, f, -3],
    [-1, -2, -3, f],
  ]),
};

/**
 * Returns set of vectors, indexed by dimension containing interesting i32
 * values.
 *
 * The tests do not do the simple option for coverage of computing the cartesian
 * product of all of the interesting i32 values N times for vecN tests,
 * because that creates a huge number of tests for vec3 and vec4, leading to
 * time outs.
 *
 * Instead they insert the interesting i32 values into each location of the
 * vector to get a spread of testing over the entire range. This reduces the
 * number of cases being run substantially, but maintains coverage.
 */
export function vectorI32Range(dim: number): number[][] {
  assert(dim === 2 || dim === 3 || dim === 4, 'vectorI32Range only accepts dimensions 2, 3, and 4');
  return kVectorI32Values[dim];
}

/**
 * @returns an ascending sorted array of numbers spread over the entire range of 32-bit signed ints
 *
 * Numbers are divided into 2 regions: negatives, and positives, with their spreads biased towards 0
 * Zero is included in range.
 *
 * @param counts structure param with 2 entries indicating the number of entries to be generated each region, values must be 0 or greater.
 */
export function fullI32Range(
  counts: {
    negative?: number;
    positive: number;
  } = { positive: 50 }
): Array<number> {
  counts.negative = counts.negative === undefined ? counts.positive : counts.negative;
  return [
    ...biasedRange(kValue.i32.negative.min, -1, counts.negative),
    0,
    ...biasedRange(1, kValue.i32.positive.max, counts.positive),
  ].map(Math.trunc);
}

/** Short list of u32 values of interest to test against */
const kInterestingU32Values: number[] = [
  0,
  1,
  10,
  256,
  Math.trunc(kValue.u32.max / 2),
  kValue.u32.max,
];

/** @returns minimal u32 values that cover the entire range of u32 behaviours
 *
 * This is used instead of fullU32Range when the number of test cases being
 * generated is a super linear function of the length of u32 values which is
 * leading to time outs.
 */
export function sparseU32Range(): number[] {
  return kInterestingU32Values;
}

const kVectorU32Values = {
  2: kInterestingU32Values.flatMap(f => [
    [f, 1],
    [1, f],
  ]),
  3: kInterestingU32Values.flatMap(f => [
    [f, 1, 2],
    [1, f, 2],
    [1, 2, f],
  ]),
  4: kInterestingU32Values.flatMap(f => [
    [f, 1, 2, 3],
    [1, f, 2, 3],
    [1, 2, f, 3],
    [1, 2, 3, f],
  ]),
};

/**
 * Returns set of vectors, indexed by dimension containing interesting u32
 * values.
 *
 * The tests do not do the simple option for coverage of computing the cartesian
 * product of all of the interesting u32 values N times for vecN tests,
 * because that creates a huge number of tests for vec3 and vec4, leading to
 * time outs.
 *
 * Instead they insert the interesting u32 values into each location of the
 * vector to get a spread of testing over the entire range. This reduces the
 * number of cases being run substantially, but maintains coverage.
 */
export function vectorU32Range(dim: number): number[][] {
  assert(dim === 2 || dim === 3 || dim === 4, 'vectorU32Range only accepts dimensions 2, 3, and 4');
  return kVectorU32Values[dim];
}

/**
 * @returns an ascending sorted array of numbers spread over the entire range of 32-bit unsigned ints
 *
 * Numbers are biased towards 0, and 0 is included in the range.
 *
 * @param count number of entries to include in the range, in addition to 0, must be greater than 0, defaults to 50
 */
export function fullU32Range(count: number = 50): Array<number> {
  return [0, ...biasedRange(1, kValue.u32.max, count)].map(Math.trunc);
}

/** Short list of f32 values of interest to test against */
const kInterestingF32Values: number[] = [
  kValue.f32.negative.min,
  -10.0,
  -1.0,
  -0.125,
  kValue.f32.negative.max,
  kValue.f32.subnormal.negative.min,
  kValue.f32.subnormal.negative.max,
  -0.0,
  0.0,
  kValue.f32.subnormal.positive.min,
  kValue.f32.subnormal.positive.max,
  kValue.f32.positive.min,
  0.125,
  1.0,
  10.0,
  kValue.f32.positive.max,
];

/** @returns minimal f32 values that cover the entire range of f32 behaviours
 *
 * Has specially selected values that cover edge cases, normals, and subnormals.
 * This is used instead of fullF32Range when the number of test cases being
 * generated is a super linear function of the length of f32 values which is
 * leading to time outs.
 *
 * These values have been chosen to attempt to test the widest range of f32
 * behaviours in the lowest number of entries, so may potentially miss function
 * specific values of interest. If there are known values of interest they
 * should be appended to this list in the test generation code.
 */
export function sparseF32Range(): number[] {
  return kInterestingF32Values;
}

const kVectorF32Values = {
  2: sparseF32Range().flatMap(f => [
    [f, 1.0],
    [1.0, f],
    [f, -1.0],
    [-1.0, f],
  ]),
  3: sparseF32Range().flatMap(f => [
    [f, 1.0, 2.0],
    [1.0, f, 2.0],
    [1.0, 2.0, f],
    [f, -1.0, -2.0],
    [-1.0, f, -2.0],
    [-1.0, -2.0, f],
  ]),
  4: sparseF32Range().flatMap(f => [
    [f, 1.0, 2.0, 3.0],
    [1.0, f, 2.0, 3.0],
    [1.0, 2.0, f, 3.0],
    [1.0, 2.0, 3.0, f],
    [f, -1.0, -2.0, -3.0],
    [-1.0, f, -2.0, -3.0],
    [-1.0, -2.0, f, -3.0],
    [-1.0, -2.0, -3.0, f],
  ]),
};

/**
 * Returns set of vectors, indexed by dimension containing interesting float
 * values.
 *
 * The tests do not do the simple option for coverage of computing the cartesian
 * product of all of the interesting float values N times for vecN tests,
 * because that creates a huge number of tests for vec3 and vec4, leading to
 * time outs.
 *
 * Instead they insert the interesting f32 values into each location of the
 * vector to get a spread of testing over the entire range. This reduces the
 * number of cases being run substantially, but maintains coverage.
 */
export function vectorF32Range(dim: number): number[][] {
  assert(dim === 2 || dim === 3 || dim === 4, 'vectorF32Range only accepts dimensions 2, 3, and 4');
  return kVectorF32Values[dim];
}

const kSparseVectorF32Values = {
  2: sparseF32Range().map((f, idx) => [idx % 2 === 0 ? f : idx, idx % 2 === 1 ? f : -idx]),
  3: sparseF32Range().map((f, idx) => [
    idx % 3 === 0 ? f : idx,
    idx % 3 === 1 ? f : -idx,
    idx % 3 === 2 ? f : idx,
  ]),
  4: sparseF32Range().map((f, idx) => [
    idx % 4 === 0 ? f : idx,
    idx % 4 === 1 ? f : -idx,
    idx % 4 === 2 ? f : idx,
    idx % 4 === 3 ? f : -idx,
  ]),
};

/**
 * Minimal set of vectors, indexed by dimension, that contain interesting float
 * values.
 *
 * This is an even more stripped down version of `vectorF32Range` for when
 * pairs of vectors are being tested.
 * All of the interesting floats from sparseF32 are guaranteed to be tested, but
 * not in every position.
 */
export function sparseVectorF32Range(dim: number): number[][] {
  assert(
    dim === 2 || dim === 3 || dim === 4,
    'sparseVectorF32Range only accepts dimensions 2, 3, and 4'
  );
  return kSparseVectorF32Values[dim];
}

const kSparseMatrixF32Values = {
  2: {
    2: kInterestingF32Values.map((f, idx) => [
      [idx % 4 === 0 ? f : idx, idx % 4 === 1 ? f : -idx],
      [idx % 4 === 2 ? f : -idx, idx % 4 === 3 ? f : idx],
    ]),
    3: kInterestingF32Values.map((f, idx) => [
      [idx % 6 === 0 ? f : idx, idx % 6 === 1 ? f : -idx, idx % 6 === 2 ? f : idx],
      [idx % 6 === 3 ? f : -idx, idx % 6 === 4 ? f : idx, idx % 6 === 5 ? f : -idx],
    ]),
    4: kInterestingF32Values.map((f, idx) => [
      [
        idx % 8 === 0 ? f : idx,
        idx % 8 === 1 ? f : -idx,
        idx % 8 === 2 ? f : idx,
        idx % 8 === 3 ? f : -idx,
      ],
      [
        idx % 8 === 4 ? f : -idx,
        idx % 8 === 5 ? f : idx,
        idx % 8 === 6 ? f : -idx,
        idx % 8 === 7 ? f : idx,
      ],
    ]),
  },
  3: {
    2: kInterestingF32Values.map((f, idx) => [
      [idx % 6 === 0 ? f : idx, idx % 6 === 1 ? f : -idx],
      [idx % 6 === 2 ? f : -idx, idx % 6 === 3 ? f : idx],
      [idx % 6 === 4 ? f : idx, idx % 6 === 5 ? f : -idx],
    ]),
    3: kInterestingF32Values.map((f, idx) => [
      [idx % 9 === 0 ? f : idx, idx % 9 === 1 ? f : -idx, idx % 9 === 2 ? f : idx],
      [idx % 9 === 3 ? f : -idx, idx % 9 === 4 ? f : idx, idx % 9 === 5 ? f : -idx],
      [idx % 9 === 6 ? f : idx, idx % 9 === 7 ? f : -idx, idx % 9 === 8 ? f : idx],
    ]),
    4: kInterestingF32Values.map((f, idx) => [
      [
        idx % 12 === 0 ? f : idx,
        idx % 12 === 1 ? f : -idx,
        idx % 12 === 2 ? f : idx,
        idx % 12 === 3 ? f : -idx,
      ],
      [
        idx % 12 === 4 ? f : -idx,
        idx % 12 === 5 ? f : idx,
        idx % 12 === 6 ? f : -idx,
        idx % 12 === 7 ? f : idx,
      ],
      [
        idx % 12 === 8 ? f : idx,
        idx % 12 === 9 ? f : -idx,
        idx % 12 === 10 ? f : idx,
        idx % 12 === 11 ? f : -idx,
      ],
    ]),
  },
  4: {
    2: kInterestingF32Values.map((f, idx) => [
      [idx % 8 === 0 ? f : idx, idx % 8 === 1 ? f : -idx],
      [idx % 8 === 2 ? f : -idx, idx % 8 === 3 ? f : idx],
      [idx % 8 === 4 ? f : idx, idx % 8 === 5 ? f : -idx],
      [idx % 8 === 6 ? f : -idx, idx % 8 === 7 ? f : idx],
    ]),
    3: kInterestingF32Values.map((f, idx) => [
      [idx % 12 === 0 ? f : idx, idx % 12 === 1 ? f : -idx, idx % 12 === 2 ? f : idx],
      [idx % 12 === 3 ? f : -idx, idx % 12 === 4 ? f : idx, idx % 12 === 5 ? f : -idx],
      [idx % 12 === 6 ? f : idx, idx % 12 === 7 ? f : -idx, idx % 12 === 8 ? f : idx],
      [idx % 12 === 9 ? f : -idx, idx % 12 === 10 ? f : idx, idx % 12 === 11 ? f : -idx],
    ]),
    4: kInterestingF32Values.map((f, idx) => [
      [
        idx % 16 === 0 ? f : idx,
        idx % 16 === 1 ? f : -idx,
        idx % 16 === 2 ? f : idx,
        idx % 16 === 3 ? f : -idx,
      ],
      [
        idx % 16 === 4 ? f : -idx,
        idx % 16 === 5 ? f : idx,
        idx % 16 === 6 ? f : -idx,
        idx % 16 === 7 ? f : idx,
      ],
      [
        idx % 16 === 8 ? f : idx,
        idx % 16 === 9 ? f : -idx,
        idx % 16 === 10 ? f : idx,
        idx % 16 === 11 ? f : -idx,
      ],
      [
        idx % 16 === 12 ? f : -idx,
        idx % 16 === 13 ? f : idx,
        idx % 16 === 14 ? f : -idx,
        idx % 16 === 15 ? f : idx,
      ],
    ]),
  },
};

/**
 * Returns a minimal set of matrices, indexed by dimension containing interesting
 * float values.
 *
 * This is the matrix analogue of `sparseVectorF32Range`, so it is producing a
 * minimal coverage set of matrices that test all of the interesting f32 values.
 * There is not a more expansive set of matrices, since matrices are even more
 * expensive than vectors for increasing runtime with coverage.
 *
 * All of the interesting floats from sparseF32 are guaranteed to be tested, but
 * not in every position.
 */
export function sparseMatrixF32Range(c: number, r: number): number[][][] {
  assert(
    c === 2 || c === 3 || c === 4,
    'sparseMatrixF32Range only accepts column counts of 2, 3, and 4'
  );
  assert(
    r === 2 || r === 3 || r === 4,
    'sparseMatrixF32Range only accepts row counts of 2, 3, and 4'
  );
  return kSparseMatrixF32Values[c][r];
}

/** Short list of f16 values of interest to test against */
const kInterestingF16Values: number[] = [
  kValue.f16.negative.min,
  -10.0,
  -1.0,
  -0.125,
  kValue.f16.negative.max,
  kValue.f16.subnormal.negative.min,
  kValue.f16.subnormal.negative.max,
  -0.0,
  0.0,
  kValue.f16.subnormal.positive.min,
  kValue.f16.subnormal.positive.max,
  kValue.f16.positive.min,
  0.125,
  1.0,
  10.0,
  kValue.f16.positive.max,
];

/** @returns minimal f16 values that cover the entire range of f16 behaviours
 *
 * Has specially selected values that cover edge cases, normals, and subnormals.
 * This is used instead of fullF16Range when the number of test cases being
 * generated is a super linear function of the length of f16 values which is
 * leading to time outs.
 *
 * These values have been chosen to attempt to test the widest range of f16
 * behaviours in the lowest number of entries, so may potentially miss function
 * specific values of interest. If there are known values of interest they
 * should be appended to this list in the test generation code.
 */
export function sparseF16Range(): number[] {
  return kInterestingF16Values;
}

const kVectorF16Values = {
  2: sparseF16Range().flatMap(f => [
    [f, 1.0],
    [1.0, f],
    [f, -1.0],
    [-1.0, f],
  ]),
  3: sparseF16Range().flatMap(f => [
    [f, 1.0, 2.0],
    [1.0, f, 2.0],
    [1.0, 2.0, f],
    [f, -1.0, -2.0],
    [-1.0, f, -2.0],
    [-1.0, -2.0, f],
  ]),
  4: sparseF16Range().flatMap(f => [
    [f, 1.0, 2.0, 3.0],
    [1.0, f, 2.0, 3.0],
    [1.0, 2.0, f, 3.0],
    [1.0, 2.0, 3.0, f],
    [f, -1.0, -2.0, -3.0],
    [-1.0, f, -2.0, -3.0],
    [-1.0, -2.0, f, -3.0],
    [-1.0, -2.0, -3.0, f],
  ]),
};

/**
 * Returns set of vectors, indexed by dimension containing interesting f16
 * values.
 *
 * The tests do not do the simple option for coverage of computing the cartesian
 * product of all of the interesting float values N times for vecN tests,
 * because that creates a huge number of tests for vec3 and vec4, leading to
 * time outs.
 *
 * Instead they insert the interesting f16 values into each location of the
 * vector to get a spread of testing over the entire range. This reduces the
 * number of cases being run substantially, but maintains coverage.
 */
export function vectorF16Range(dim: number): number[][] {
  assert(dim === 2 || dim === 3 || dim === 4, 'vectorF16Range only accepts dimensions 2, 3, and 4');
  return kVectorF16Values[dim];
}

const kSparseVectorF16Values = {
  2: sparseF16Range().map((f, idx) => [idx % 2 === 0 ? f : idx, idx % 2 === 1 ? f : -idx]),
  3: sparseF16Range().map((f, idx) => [
    idx % 3 === 0 ? f : idx,
    idx % 3 === 1 ? f : -idx,
    idx % 3 === 2 ? f : idx,
  ]),
  4: sparseF16Range().map((f, idx) => [
    idx % 4 === 0 ? f : idx,
    idx % 4 === 1 ? f : -idx,
    idx % 4 === 2 ? f : idx,
    idx % 4 === 3 ? f : -idx,
  ]),
};

/**
 * Minimal set of vectors, indexed by dimension, that contain interesting f16
 * values.
 *
 * This is an even more stripped down version of `vectorF16Range` for when
 * pairs of vectors are being tested.
 * All of the interesting floats from sparseF16 are guaranteed to be tested, but
 * not in every position.
 */
export function sparseVectorF16Range(dim: number): number[][] {
  assert(
    dim === 2 || dim === 3 || dim === 4,
    'sparseVectorF16Range only accepts dimensions 2, 3, and 4'
  );
  return kSparseVectorF16Values[dim];
}

const kSparseMatrixF16Values = {
  2: {
    2: kInterestingF16Values.map((f, idx) => [
      [idx % 4 === 0 ? f : idx, idx % 4 === 1 ? f : -idx],
      [idx % 4 === 2 ? f : -idx, idx % 4 === 3 ? f : idx],
    ]),
    3: kInterestingF16Values.map((f, idx) => [
      [idx % 6 === 0 ? f : idx, idx % 6 === 1 ? f : -idx, idx % 6 === 2 ? f : idx],
      [idx % 6 === 3 ? f : -idx, idx % 6 === 4 ? f : idx, idx % 6 === 5 ? f : -idx],
    ]),
    4: kInterestingF16Values.map((f, idx) => [
      [
        idx % 8 === 0 ? f : idx,
        idx % 8 === 1 ? f : -idx,
        idx % 8 === 2 ? f : idx,
        idx % 8 === 3 ? f : -idx,
      ],
      [
        idx % 8 === 4 ? f : -idx,
        idx % 8 === 5 ? f : idx,
        idx % 8 === 6 ? f : -idx,
        idx % 8 === 7 ? f : idx,
      ],
    ]),
  },
  3: {
    2: kInterestingF16Values.map((f, idx) => [
      [idx % 6 === 0 ? f : idx, idx % 6 === 1 ? f : -idx],
      [idx % 6 === 2 ? f : -idx, idx % 6 === 3 ? f : idx],
      [idx % 6 === 4 ? f : idx, idx % 6 === 5 ? f : -idx],
    ]),
    3: kInterestingF16Values.map((f, idx) => [
      [idx % 9 === 0 ? f : idx, idx % 9 === 1 ? f : -idx, idx % 9 === 2 ? f : idx],
      [idx % 9 === 3 ? f : -idx, idx % 9 === 4 ? f : idx, idx % 9 === 5 ? f : -idx],
      [idx % 9 === 6 ? f : idx, idx % 9 === 7 ? f : -idx, idx % 9 === 8 ? f : idx],
    ]),
    4: kInterestingF16Values.map((f, idx) => [
      [
        idx % 12 === 0 ? f : idx,
        idx % 12 === 1 ? f : -idx,
        idx % 12 === 2 ? f : idx,
        idx % 12 === 3 ? f : -idx,
      ],
      [
        idx % 12 === 4 ? f : -idx,
        idx % 12 === 5 ? f : idx,
        idx % 12 === 6 ? f : -idx,
        idx % 12 === 7 ? f : idx,
      ],
      [
        idx % 12 === 8 ? f : idx,
        idx % 12 === 9 ? f : -idx,
        idx % 12 === 10 ? f : idx,
        idx % 12 === 11 ? f : -idx,
      ],
    ]),
  },
  4: {
    2: kInterestingF16Values.map((f, idx) => [
      [idx % 8 === 0 ? f : idx, idx % 8 === 1 ? f : -idx],
      [idx % 8 === 2 ? f : -idx, idx % 8 === 3 ? f : idx],
      [idx % 8 === 4 ? f : idx, idx % 8 === 5 ? f : -idx],
      [idx % 8 === 6 ? f : -idx, idx % 8 === 7 ? f : idx],
    ]),
    3: kInterestingF16Values.map((f, idx) => [
      [idx % 12 === 0 ? f : idx, idx % 12 === 1 ? f : -idx, idx % 12 === 2 ? f : idx],
      [idx % 12 === 3 ? f : -idx, idx % 12 === 4 ? f : idx, idx % 12 === 5 ? f : -idx],
      [idx % 12 === 6 ? f : idx, idx % 12 === 7 ? f : -idx, idx % 12 === 8 ? f : idx],
      [idx % 12 === 9 ? f : -idx, idx % 12 === 10 ? f : idx, idx % 12 === 11 ? f : -idx],
    ]),
    4: kInterestingF16Values.map((f, idx) => [
      [
        idx % 16 === 0 ? f : idx,
        idx % 16 === 1 ? f : -idx,
        idx % 16 === 2 ? f : idx,
        idx % 16 === 3 ? f : -idx,
      ],
      [
        idx % 16 === 4 ? f : -idx,
        idx % 16 === 5 ? f : idx,
        idx % 16 === 6 ? f : -idx,
        idx % 16 === 7 ? f : idx,
      ],
      [
        idx % 16 === 8 ? f : idx,
        idx % 16 === 9 ? f : -idx,
        idx % 16 === 10 ? f : idx,
        idx % 16 === 11 ? f : -idx,
      ],
      [
        idx % 16 === 12 ? f : -idx,
        idx % 16 === 13 ? f : idx,
        idx % 16 === 14 ? f : -idx,
        idx % 16 === 15 ? f : idx,
      ],
    ]),
  },
};

/**
 * Returns a minimal set of matrices, indexed by dimension containing interesting
 * f16 values.
 *
 * This is the matrix analogue of `sparseVectorF16Range`, so it is producing a
 * minimal coverage set of matrices that test all of the interesting f16 values.
 * There is not a more expansive set of matrices, since matrices are even more
 * expensive than vectors for increasing runtime with coverage.
 *
 * All of the interesting floats from sparseF16 are guaranteed to be tested, but
 * not in every position.
 */
export function sparseMatrixF16Range(c: number, r: number): number[][][] {
  assert(
    c === 2 || c === 3 || c === 4,
    'sparseMatrixF16Range only accepts column counts of 2, 3, and 4'
  );
  assert(
    r === 2 || r === 3 || r === 4,
    'sparseMatrixF16Range only accepts row counts of 2, 3, and 4'
  );
  return kSparseMatrixF16Values[c][r];
}

/** Short list of f64 values of interest to test against */
const kInterestingF64Values: number[] = [
  kValue.f64.negative.min,
  -10.0,
  -1.0,
  -0.125,
  kValue.f64.negative.max,
  kValue.f64.subnormal.negative.min,
  kValue.f64.subnormal.negative.max,
  -0.0,
  0.0,
  kValue.f64.subnormal.positive.min,
  kValue.f64.subnormal.positive.max,
  kValue.f64.positive.min,
  0.125,
  1.0,
  10.0,
  kValue.f64.positive.max,
];

/** @returns minimal F64 values that cover the entire range of F64 behaviours
 *
 * Has specially selected values that cover edge cases, normals, and subnormals.
 * This is used instead of fullF64Range when the number of test cases being
 * generated is a super linear function of the length of F64 values which is
 * leading to time outs.
 *
 * These values have been chosen to attempt to test the widest range of F64
 * behaviours in the lowest number of entries, so may potentially miss function
 * specific values of interest. If there are known values of interest they
 * should be appended to this list in the test generation code.
 */
export function sparseF64Range(): number[] {
  return kInterestingF64Values;
}

const kVectorF64Values = {
  2: sparseF64Range().flatMap(f => [
    [f, 1.0],
    [1.0, f],
    [f, -1.0],
    [-1.0, f],
  ]),
  3: sparseF64Range().flatMap(f => [
    [f, 1.0, 2.0],
    [1.0, f, 2.0],
    [1.0, 2.0, f],
    [f, -1.0, -2.0],
    [-1.0, f, -2.0],
    [-1.0, -2.0, f],
  ]),
  4: sparseF64Range().flatMap(f => [
    [f, 1.0, 2.0, 3.0],
    [1.0, f, 2.0, 3.0],
    [1.0, 2.0, f, 3.0],
    [1.0, 2.0, 3.0, f],
    [f, -1.0, -2.0, -3.0],
    [-1.0, f, -2.0, -3.0],
    [-1.0, -2.0, f, -3.0],
    [-1.0, -2.0, -3.0, f],
  ]),
};

/**
 * Returns set of vectors, indexed by dimension containing interesting float
 * values.
 *
 * The tests do not do the simple option for coverage of computing the cartesian
 * product of all of the interesting float values N times for vecN tests,
 * because that creates a huge number of tests for vec3 and vec4, leading to
 * time outs.
 *
 * Instead they insert the interesting F64 values into each location of the
 * vector to get a spread of testing over the entire range. This reduces the
 * number of cases being run substantially, but maintains coverage.
 */
export function vectorF64Range(dim: number): number[][] {
  assert(dim === 2 || dim === 3 || dim === 4, 'vectorF64Range only accepts dimensions 2, 3, and 4');
  return kVectorF64Values[dim];
}

/**
 * @returns the result matrix in Array<Array<number>> type.
 *
 * Matrix multiplication. A is m x n and B is n x p. Returns
 * m x p result.
 */
// A is m x n. B is n x p. product is m x p.
export function multiplyMatrices(
  A: Array<Array<number>>,
  B: Array<Array<number>>
): Array<Array<number>> {
  assert(A.length > 0 && B.length > 0 && B[0].length > 0 && A[0].length === B.length);
  const product = new Array<Array<number>>(A.length);
  for (let i = 0; i < product.length; ++i) {
    product[i] = new Array<number>(B[0].length).fill(0);
  }

  for (let m = 0; m < A.length; ++m) {
    for (let p = 0; p < B[0].length; ++p) {
      for (let n = 0; n < B.length; ++n) {
        product[m][p] += A[m][n] * B[n][p];
      }
    }
  }

  return product;
}

/** Sign-extend the `bits`-bit number `n` to a 32-bit signed integer. */
export function signExtend(n: number, bits: number): number {
  const shift = 32 - bits;
  return (n << shift) >> shift;
}

export interface QuantizeFunc {
  (num: number): number;
}

/** @returns the closest 32-bit floating point value to the input */
export function quantizeToF32(num: number): number {
  return f32(num).value as number;
}

/** @returns the closest 16-bit floating point value to the input */
export function quantizeToF16(num: number): number {
  return f16(num).value as number;
}

/** @returns the closest 32-bit signed integer value to the input */
export function quantizeToI32(num: number): number {
  return i32(num).value as number;
}

/** @returns the closest 32-bit signed integer value to the input */
export function quantizeToU32(num: number): number {
  return u32(num).value as number;
}

/** @returns whether the number is an integer and a power of two */
export function isPowerOfTwo(n: number): boolean {
  if (!Number.isInteger(n)) {
    return false;
  }
  return n !== 0 && (n & (n - 1)) === 0;
}

/** @returns the Greatest Common Divisor (GCD) of the inputs */
export function gcd(a: number, b: number): number {
  assert(Number.isInteger(a) && a > 0);
  assert(Number.isInteger(b) && b > 0);

  while (b !== 0) {
    const bTemp = b;
    b = a % b;
    a = bTemp;
  }

  return a;
}

/** @returns the Least Common Multiplier (LCM) of the inputs */
export function lcm(a: number, b: number): number {
  return (a * b) / gcd(a, b);
}

/**
 * @returns the bit representation as a 64-integer, via interpreting the input
 * as a 64-bit float value
 */
export function reinterpretF64AsU64(input: number): bigint {
  return new BigUint64Array(new Float64Array([input]).buffer)[0];
}

/**
 * @returns a 64-bit float value via interpreting the input as the bit
 * representation as a 64-bit integer
 */
export function reinterpretU64AsF64(input: bigint): number {
  return new Float64Array(new BigUint64Array([input]).buffer)[0];
}

/**
 * @returns a 32-bit float value via interpreting the input as the bit
 * representation as a 32-bit integer
 */
export function reinterpretU32AsF32(input: number): number {
  return floatBitsToNumber(input, kFloat32Format);
}

/**
 * @returns a 16-bit float value via interpreting the input as the bit
 * representation as a 16-bit integer
 */
export function reinterpretU16AsF16(hex: number): number {
  return floatBitsToNumber(hex, kFloat16Format);
}

/** @returns the cross of an array with the intermediate result of cartesianProduct
 *
 * @param elements array of values to cross with the intermediate result of
 *                 cartesianProduct
 * @param intermediate arrays of values representing the partial result of
 *                     cartesianProduct
 */
function cartesianProductImpl<T>(elements: T[], intermediate: T[][]): T[][] {
  const result: T[][] = [];
  elements.forEach((e: T) => {
    if (intermediate.length > 0) {
      intermediate.forEach((i: T[]) => {
        result.push([...i, e]);
      });
    } else {
      result.push([e]);
    }
  });
  return result;
}

/** @returns the cartesian product (NxMx...) of a set of arrays
 *
 * This is implemented by calculating the cross of a single input against an
 * intermediate result for each input to build up the final array of arrays.
 *
 * There are examples of doing this more succinctly using map & reduce online,
 * but they are a bit more opaque to read.
 *
 * @param inputs arrays of numbers to calculate cartesian product over
 */
export function cartesianProduct<T>(...inputs: T[][]): T[][] {
  let result: T[][] = [];
  inputs.forEach((i: T[]) => {
    result = cartesianProductImpl<T>(i, result);
  });

  return result;
}

/** @returns all of the permutations of an array
 *
 * Recursively calculates all of the permutations, does not cull duplicate
 * entries.
 *
 * Only feasible for inputs of lengths 5 or so, since the number of permutations
 * is (input.length)!, so will cause the stack to explode for longer inputs.
 *
 * This code could be made iterative using something like
 * Steinhaus–Johnson–Trotter and additionally turned into a generator to reduce
 * the stack size, but there is still a fundamental combinatorial explosion
 * here that will affect runtime.
 *
 * @param input the array to get permutations of
 */
export function calculatePermutations<T>(input: T[]): T[][] {
  if (input.length === 0) {
    return [];
  }

  if (input.length === 1) {
    return [input];
  }

  if (input.length === 2) {
    return [input, [input[1], input[0]]];
  }

  const result: T[][] = [];
  input.forEach((head, idx) => {
    const tail = input.slice(0, idx).concat(input.slice(idx + 1));
    const permutations = calculatePermutations(tail);
    permutations.forEach(p => {
      result.push([head, ...p]);
    });
  });

  return result;
}

/**
 * Convert an Array of Arrays to linear array
 *
 * Caller is responsible to retaining the dimensions of the array for later
 * unflattening
 *
 * @param m Matrix to convert
 */
export function flatten2DArray<T>(m: T[][]): T[] {
  const c = m.length;
  const r = m[0].length;
  assert(
    m.every(c => c.length === r),
    `Unexpectedly received jagged array to flatten`
  );
  const result: T[] = Array<T>(c * r);
  for (let i = 0; i < c; i++) {
    for (let j = 0; j < r; j++) {
      result[j + i * r] = m[i][j];
    }
  }
  return result;
}

/**
 * Convert linear array to an Array of Arrays
 * @param n an array to convert
 * @param c number of elements in the array containing arrays
 * @param r number of elements in the arrays that are contained
 */
export function unflatten2DArray<T>(n: T[], c: number, r: number): T[][] {
  assert(
    c > 0 && Number.isInteger(c) && r > 0 && Number.isInteger(r),
    `columns (${c}) and rows (${r}) need to be positive integers`
  );
  assert(n.length === c * r, `m.length(${n.length}) should equal c * r (${c * r})`);
  const result: T[][] = [...Array(c)].map(_ => [...Array(r)]);
  for (let i = 0; i < c; i++) {
    for (let j = 0; j < r; j++) {
      result[i][j] = n[j + i * r];
    }
  }
  return result;
}

/**
 * Performs a .map over a matrix and return the result
 * The shape of the input and output matrices will be the same
 *
 * @param m input matrix of type T
 * @param op operation that converts an element of type T to one of type S
 * @returns a matrix with elements of type S that are calculated by applying op element by element
 */
export function map2DArray<T, S>(m: T[][], op: (input: T) => S): S[][] {
  const c = m.length;
  const r = m[0].length;
  assert(
    m.every(c => c.length === r),
    `Unexpectedly received jagged array to map`
  );
  const result: S[][] = [...Array(c)].map(_ => [...Array(r)]);
  for (let i = 0; i < c; i++) {
    for (let j = 0; j < r; j++) {
      result[i][j] = op(m[i][j]);
    }
  }
  return result;
}
