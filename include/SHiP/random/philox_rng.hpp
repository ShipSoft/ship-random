// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

/// @file
/// @brief Counter-based random number generator for reproducible, thread-safe
/// event processing.
///
/// Random123 Philox 4x32 is deterministic per seed with no shared state, so
/// each event seeds a fresh instance and the result is reproducible and
/// thread-safe by construction.
///
/// The counter advances sequentially (ctr[0]++ per 4-word block) and the
/// Philox output block is buffered; uniform() returns successive words of the
/// buffered output, which preserves Philox's guaranteed period.
///
/// The draws are part of the physics output of every package that uses this
/// class: changing how a value is computed changes simulated, digitised and
/// reconstructed events. tests/philox_rng_test.cpp pins the sequences.

#pragma once

#include <Random123/philox.h>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <stdexcept>

namespace SHiP::random {

// Resolution of the uniform draws feeding the derived distributions
// (gaussian, gamma_wh, beta_dist). Bits32 is the historical default and keeps
// existing digitisation output bit-identical; Bits53 uses uniform53() and
// extends the Box–Muller tail from ~6.7σ to ~8.6σ at the cost of twice as
// many Philox words per deviate.
enum class Precision : std::uint8_t { Bits32, Bits53 };

class PhiloxRng {
   public:
    /// key_hi selects an independent stream, so different generators seeded
    // with the same seed draw uncorrelated sequences. substream initializes
    // the counter, giving each (seed, key_hi, substream) triple a disjoint
    /// counter range. Use it for per-event sub-streams of one seed without
    /// perturbing the key (a key derived as seed ^ event would collide across
    /// seeds: XOR is not injective in (seed, event)).
    // The sub-stream index is 64 bits wide, split over counter words 1 (low)
    // and 2 (high), so event numbers beyond 2^32 don't wrap onto earlier
    // events. Indices below 2^32 leave word 2 at zero, as before.
    explicit PhiloxRng(std::uint32_t seed, std::uint32_t key_hi = 0xBEEFCAFE,
                       std::uint64_t substream = 0)
        : key_{{seed, key_hi}},
          ctr_{{0, static_cast<std::uint32_t>(substream),
                static_cast<std::uint32_t>(substream >> 32u), 0}} {}

    /// Uniform in [0, 1), with 32 bits of resolution.
    double uniform() {
        if (idx_ >= 4) {
            buf_ = rng_(ctr_, key_);
            ctr_[0]++;
            idx_ = 0;
        }
        return buf_[idx_++] * (1.0 / 4294967296.0);
    }

    /// Uniform in [lo, hi), with 32 bits of resolution.
    double uniform(double lo, double hi) { return lo + ((hi - lo) * uniform()); }

    /// Uniform in [0, 1) with full double resolution: combines two consecutive
    /// 32-bit Philox words into a 64-bit integer and keeps the top 53 bits.
    /// Use it when the sampled range spans many orders of magnitude (e.g.
    /// ps-scale jitter within a multi-second spill), where uniform() is too
    /// coarse. If only one word is left in the buffer, it is skipped.
    double uniform53() {
        if (idx_ > 2) {
            buf_ = rng_(ctr_, key_);
            ctr_[0]++;
            idx_ = 0;
        }
        std::uint64_t const hi = buf_[idx_++];
        std::uint64_t const lo = buf_[idx_++];
        std::uint64_t const bits = (hi << 32U) | lo;
        return static_cast<double>(bits >> 11U) * (1.0 / 9007199254740992.0);
    }

    /// Uniform in [lo, hi) with full double resolution.
    double uniform53(double lo, double hi) { return lo + ((hi - lo) * uniform53()); }

    /// Gaussian deviate by the Box–Muller transform, cosine branch only (two
    /// uniforms per deviate). The first draw is flipped from [0, 1) to (0, 1]
    /// to keep the log argument nonzero.
    double gaussian(double mean, double sigma, Precision precision = Precision::Bits32) {
        double const u1 = 1.0 - draw(precision);
        double const u2 = draw(precision);
        return mean +
               (sigma * std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::numbers::pi * u2));
    }

    // An approximation of a gamma function - Wilson-Hilferty (1931)
    // This rapidly approaches a Gaussian by alpha ~ 10
    double gamma_wh(double alpha, double scale = 1, Precision precision = Precision::Bits32) {
        if (alpha < 1.0) {
            throw std::invalid_argument(
                "Provided alpha for gamma function approximation is less than 1. Consider using an "
                "exact gamma function for this instance.");
        }
        const double a = 1.0 - (1.0 / (9.0 * alpha));
        const double b = 1.0 / (3.0 * std::sqrt(alpha));

        double x = 0.0;
        while (x <= 0.0) {
            x = a + (b * gaussian(0.0, 1.0, precision));
        }

        return scale * alpha * x * x * x;
    }

    // An approximate beta distribution. Uses approximations of the gamma function for maximum
    // speed. Not appropriate for very small alpha or zeta.
    double beta_dist_approx(double alpha, double zeta, Precision precision = Precision::Bits32) {
        const double x = gamma_wh(alpha, 1, precision);
        const double y = gamma_wh(zeta, 1, precision);

        if (x == 0.0 && y == 0.0) {
            return alpha >= zeta ? 1.0 : 0.0;  // both underflowed
        }
        if (x >= y) {
            return 1.0 / (1.0 + (y / x));
        }
        const double r = x / y;
        return r / (1.0 + r);
    }

   private:
    double draw(Precision precision) {
        return precision == Precision::Bits53 ? uniform53() : uniform();
    }

    r123::Philox4x32 rng_;
    r123::Philox4x32::key_type key_;
    r123::Philox4x32::ctr_type ctr_;
    r123::Philox4x32::ctr_type buf_{};
    int idx_ = 4;
};

}  // namespace SHiP::random
