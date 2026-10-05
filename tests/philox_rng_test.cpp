// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// Pins the PhiloxRng sequences. The expected values were produced by the
// copies of this class that aegir and Shannon carried before ship-random
// existed, so a pass means the shared class draws exactly the same numbers
// they did. uniform, uniform(lo, hi) and uniform53 must match bit for bit;
// gaussian goes through std::log and std::cos and may differ in the last bit
// between maths libraries.

#include <SHiP/random/philox_rng.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <print>

namespace {

template <std::size_t N, typename Draw>
int check(char const* name, std::array<double, N> const& expected, Draw draw,
          double rel_tol = 0.0) {
    int failures = 0;
    std::size_t i = 0;
    for (double const want : expected) {
        double const got = draw();
        if (std::abs(got - want) > rel_tol * std::abs(want)) {
            std::println("{}[{}]: got {:a}, expected {:a}", name, i, got, want);
            ++failures;
        }
        ++i;
    }
    return failures;
}

}  // namespace

int main() {
    using SHiP::random::PhiloxRng;
    int failures = 0;

    PhiloxRng a{12345};
    failures +=
        check<9>("uniform",
                 {0x1.b06708p-1, 0x1.b0f2fep-2, 0x1.54e21b1p-3, 0x1.e21a636cp-1, 0x1.d8841fb8p-3,
                  0x1.35d41p-4, 0x1.df5607a8p-1, 0x1.0e1989f4p-1, 0x1.009ea97p-1},
                 [&] { return a.uniform(); });

    PhiloxRng b{20260703, 0x47345EED, 42};
    failures += check<5>(
        "uniform(lo, hi)",
        {0x1.279fad7f6p+2, 0x1.35c950c6p+2, 0x1.02eb6f84p-1, 0x1.f8d30426p+1, 0x1.e438764ap-2},
        [&] { return b.uniform(-2.5, 7.0); });

    PhiloxRng c{7, 0xBEEFCAFE, 3};
    failures += check<5>("uniform53",
                         {0x1.ec09a84a2215p-2, 0x1.75b3cbce8cbe7p-1, 0x1.5752b565bc7f7p-1,
                          0x1.a888b4e9e8bf9p-1, 0x1.3dea405f25c14p-3},
                         [&] { return c.uniform53(); });

    // One uniform() first, so the next uniform53() has to skip a lone word.
    PhiloxRng d{7, 0xBEEFCAFE, 3};
    auto const mixed = std::array{0x1.ec09a848p-2, 0x1.88854222bad9ep+0, 0x1.aba95ab2de3fcp+0,
                                  0x1.d4445a74f45fcp+0};
    failures += check<1>("mixed uniform", {mixed[0]}, [&] { return d.uniform(); });
    failures += check<3>("mixed uniform53(lo, hi)", {mixed[1], mixed[2], mixed[3]},
                         [&] { return d.uniform53(1.0, 2.0); });

    PhiloxRng e{99, 0x12345678, 0};
    failures += check<5>(
        "gaussian",
        {0x1.32818172155cap+3, 0x1.39a9d1fb0dfabp+3, 0x1.4921dfa48ac1dp+3, 0x1.4add79e0a32a7p+3,
         0x1.379f710495434p+3},
        [&] { return e.gaussian(10.0, 0.5); }, 1e-13);

    if (failures == 0) {
        std::println("all PhiloxRng sequences match");
    }
    return failures == 0 ? 0 : 1;
}
