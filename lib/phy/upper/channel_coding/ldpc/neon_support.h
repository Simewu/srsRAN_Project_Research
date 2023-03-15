/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

/// \file
/// \brief NEON support for LDPC.
///
/// Builds upon \ref simd_support.h and specializes functions and templates for NEON registers.

#pragma once

#include "simd_support.h"

namespace srsran {

namespace neon {

using neon_span = detail::simd_span<detail::int8x16_wrapper>;

/// \brief Scales packed 8-bit integers in \c a by the scaling factor \c sf.
///
/// Values of \c a larger than \c max or smaller than \c -max are forwarded unaltered.
/// \param[in] a   Vector of packed 8-bit integers.
/// \param[in] sf  Scaling factor (0, 1].
/// \param[in] max Maximum input value (in absolute value) to which the scaling is applied [0, 127).
/// \return    Vector of packed 8-bit integers with the scaling result.
inline int8x16_t scale_s8(int8x16_t a, float sf, uint8_t max)
{
  srsran_assert((sf > 0) && (sf <= 1), "Scaling factor out of range.");
  srsran_assert(max < 127, "Parameter max out of range.");

  if (sf >= .9999) {
    return a;
  }
  // FLOAT2INT = 2^8 = 256
  static constexpr unsigned FLOAT2INT       = 1U << 8U;
  const int8x16_t           MAX_s8          = vdupq_n_s8(max);
  const int8x16_t           MIN_s8          = vdupq_n_s8(-max);
  static const uint16x8_t   mask_even_bytes = vdupq_n_u16(0x00ff);
  static const uint16x8_t   mask_odd_bytes  = vdupq_n_u16(0xff00);

  // Scaling factor times FLOAT2INT. Note that the upper bound on sf implies
  // that (sf * FLOAT2INT) can be represented over 8 bits.
  uint8x8_t    sf_u8               = vdup_n_u8(static_cast<uint8_t>(sf * FLOAT2INT));
  uint8x16x2_t even_odd_deinterlvd = vuzpq_u8(vreinterpretq_u8_s8(a), vreinterpretq_u8_s8(a));
  // The above instruction results in:
  // even bytes of input vector placed in interleaved.val[0]
  // odd bytes of input vector  placed in interleaved.val[1]
  uint16x8_t p_even_u16 = vmull_u8(vget_low_u8(even_odd_deinterlvd.val[0]), sf_u8);
  uint16x8_t p_odd_u16  = vmull_u8(vget_low_u8(even_odd_deinterlvd.val[1]), sf_u8);
  // Shift right MSB byte in every element of p_even_u16, mask it and xor with odd bytes stored in p_odd_u16.
  p_odd_u16        = vandq_u16(p_odd_u16, mask_odd_bytes);
  p_even_u16       = vandq_u16(vshrq_n_u16(p_even_u16, 8), mask_even_bytes);
  int8x16_t result = vreinterpretq_s8_u16(vorrq_u16(p_even_u16, p_odd_u16));

  // Replace values corresponding to "large" inputs with their original values.
  uint8x16_t mask_u8 = vcgtq_s8(a, MAX_s8);
  result             = vbslq_s8(mask_u8, a, result);
  mask_u8            = vcgtq_s8(MIN_s8, a);
  result             = vbslq_s8(mask_u8, a, result);
  return result;
}

} // namespace neon
} // namespace srsran
