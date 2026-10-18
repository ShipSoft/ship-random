// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// Pins every PhiloxRng draw method over long sequences, so any change to the
// numbers a given (seed, key_hi, substream) produces fails here. A deliberate
// stream change is a minor release under 0.x (the CMake version file accepts
// only the same minor); regenerate the tables with `stream_pin_test --print`
// and say in the changelog which streams moved.
//
// Two kinds of check:
//
// - Bit-exact methods (uniform, uniform(lo, hi), uniform53, uniform53(lo, hi))
//   are hashed over 10000 interleaved draws per stream, for a grid of seeds,
//   keys and substreams that includes the edges of the 32-bit range and
//   substreams above 2^32, which use counter word 2.
// - Methods that go through std::log/std::cos (gaussian, gamma_wh,
//   beta_dist_approx) can differ in the last bit between maths libraries, so
//   they are pinned to a relative tolerance: the first draws and the sum of a
//   longer run.
//
// The bit-exact part gives the same hashes on every platform and with any
// floating-point contraction flags, because the header writes its
// multiply-adds as std::fma. Before 0.4.0 the (lo, hi) overloads did not, and
// aarch64 or -mfma builds drew different bits from plain x86-64 ones.

#include <SHiP/random/philox_rng.hpp>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <print>
#include <stdexcept>
#include <string_view>

namespace {

using SHiP::random::PhiloxRng;
using SHiP::random::Precision;

constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr int kDrawsPerStream = 10000;
constexpr int kDistributionRun = 2000;
constexpr double kDistributionTol = 1e-12;

std::uint64_t mix(std::uint64_t hash, double value) {
    auto const bits = std::bit_cast<std::uint64_t>(value);
    for (unsigned byte = 0U; byte < 8U; ++byte) {
        hash ^= (bits >> (8U * byte)) & 0xFFU;
        hash *= kFnvPrime;
    }
    return hash;
}

struct Stream {
    std::uint32_t seed;
    std::uint32_t key;
    std::uint64_t substream;
    std::uint64_t hash;
};

// Mixes 32-bit draws (one Philox word) and 53-bit draws (two words) in an
// order that reaches every position in the four-word block for both, including
// a uniform53 call with one word left, which skips it. Alternate passes use the
// (lo, hi) overloads. A fixed rhythm such as u, u, u53, u53 never hits that
// case.
constexpr std::array<bool, 13> kIs53{false, true,  true,  false, false, true, false,
                                     true,  false, false, false, true,  true};

std::uint64_t hash_stream(std::uint32_t seed, std::uint32_t key, std::uint64_t substream) {
    PhiloxRng rng{seed, key, substream};
    std::uint64_t hash = kFnvOffset;
    for (int i = 0; i < kDrawsPerStream; ++i) {
        bool const ranged = ((i / static_cast<int>(kIs53.size())) % 2) == 1;
        if (kIs53.at(static_cast<std::size_t>(i) % kIs53.size())) {
            hash = mix(hash, ranged ? rng.uniform53(1e-3, 2e3) : rng.uniform53());
        } else {
            hash = mix(hash, ranged ? rng.uniform(-3.0, 7.5) : rng.uniform());
        }
    }
    return hash;
}

// clang-format off
constexpr std::array kStreams{
    // STREAM-TABLE
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x0ULL, .hash = 0x4F57D384B62AAF40ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x1ULL, .hash = 0x71FB3D3EDCE822E8ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x7FFFFFFFULL, .hash = 0xB7995C6832E0520AULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x80000000ULL, .hash = 0xC2BBE0F9FECA1AACULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFULL, .hash = 0x25A04E8CCA35C9D4ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x100000000ULL, .hash = 0x840C0D60EAE8BE3BULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x57A54069F8A7950DULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x0ULL, .hash = 0x6CEBB1D86A627B60ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x1ULL, .hash = 0x8EE47BE1B4F06850ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x7FFFFFFFULL, .hash = 0x8B5489D6ED12E7B2ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x80000000ULL, .hash = 0x7612A8800F231D1EULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0xFFFFFFFFULL, .hash = 0x85470FF9F922EF24ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x100000000ULL, .hash = 0x38683BCB6ADECB79ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x98BDAF0EC1C9CBABULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x0ULL, .hash = 0x3A3EBE842B1C2447ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x1ULL, .hash = 0x3BFC43E4417B65E4ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x7FFFFFFFULL, .hash = 0x9C4D584A2C8E1DEAULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x80000000ULL, .hash = 0x0F7DC7C382FFE488ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0xFFFFFFFFULL, .hash = 0xC129473425219D43ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x100000000ULL, .hash = 0x764A64A8E0F4A592ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xDD586F5165DD7960ULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x0ULL, .hash = 0x5123765F32F327ABULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x1ULL, .hash = 0x09F5FEB6CDD82CDFULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x7FFFFFFFULL, .hash = 0xCD5C084AD702113DULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x80000000ULL, .hash = 0xFE76B3F3984BFA55ULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0xFFFFFFFFULL, .hash = 0x9DB7C3C03CABF690ULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x100000000ULL, .hash = 0x3C04737BC55AFE0CULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xA73F6E807FF811C1ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x0ULL, .hash = 0xD819383E28B9FF82ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x1ULL, .hash = 0xB21584E85CFBBC76ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x7FFFFFFFULL, .hash = 0x15A81ECA203E8AE0ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x80000000ULL, .hash = 0xE4AFEC847FE813C1ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFULL, .hash = 0xB4DAC71E0B782AABULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x100000000ULL, .hash = 0xB883DE3A16C24433ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xB5CAA13D07536037ULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x0ULL, .hash = 0x0AF2B31D647397D5ULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x1ULL, .hash = 0xE1A3EB20F20CD4EAULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x7FFFFFFFULL, .hash = 0x2F349EC267E6707BULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x80000000ULL, .hash = 0xDB80DE30C192A9CDULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0xFFFFFFFFULL, .hash = 0x8F00BD228E6182BFULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x100000000ULL, .hash = 0x146CA2C474D85696ULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x8444961299759DC4ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x0ULL, .hash = 0xD3CDBC04D73B6F63ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x1ULL, .hash = 0xD75F74F8D63F909EULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x7FFFFFFFULL, .hash = 0x05D255EA18336654ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x80000000ULL, .hash = 0xB4236C9B8589D775ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0xFFFFFFFFULL, .hash = 0x219C89BA41473E0AULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x100000000ULL, .hash = 0x9F4111B5F7E2FF87ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x0D9FA827875A3D13ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x0ULL, .hash = 0x667CB1E9EDA9D767ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x1ULL, .hash = 0x3A80CE6529E21C76ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x7FFFFFFFULL, .hash = 0x5789580EB5BEB355ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x80000000ULL, .hash = 0x46C56E8380D13D0EULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0xFFFFFFFFULL, .hash = 0x9D61CCD3E7DC9CEEULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x100000000ULL, .hash = 0xBACEF6E412E31558ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x978484FC30FC0CD3ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x0ULL, .hash = 0x929CA4F7D3114C30ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x1ULL, .hash = 0x6C63643EE357FCB9ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x7FFFFFFFULL, .hash = 0xFF07F7675DFC377BULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x80000000ULL, .hash = 0xAFF3FD76FE161E03ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFULL, .hash = 0x842F00DBB99B4AF1ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x100000000ULL, .hash = 0xAE436F26678212A2ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x6EB78E1E41326ED5ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x0ULL, .hash = 0x5EB5E4D07D0FA458ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x1ULL, .hash = 0xD61AD6718230E58FULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x7FFFFFFFULL, .hash = 0x1DB2A721CF2A32ABULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x80000000ULL, .hash = 0x436650857D6AD394ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0xFFFFFFFFULL, .hash = 0x694E2ABFB4074680ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x100000000ULL, .hash = 0x177F6096D260E62FULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x9F2581592E3091B2ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x0ULL, .hash = 0xC54CE54B8004A2AEULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x1ULL, .hash = 0x0FA9272EA48C363EULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x7FFFFFFFULL, .hash = 0x5795C13122A81E25ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x80000000ULL, .hash = 0xFC3ED05FDB3E917DULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0xFFFFFFFFULL, .hash = 0x7634E1F7C4013621ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x100000000ULL, .hash = 0xE07A1B3857FF5834ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x3D5CF6A1655ACA06ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x0ULL, .hash = 0x33EDD1880BABAFCBULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x1ULL, .hash = 0x02C0BBD3C15A211EULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x7FFFFFFFULL, .hash = 0xA0BFA66C24D7AF83ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x80000000ULL, .hash = 0x3321EF4CDEAF2CA3ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0xFFFFFFFFULL, .hash = 0xDF2A42DDDB8E2519ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x100000000ULL, .hash = 0xD56F610E4ECD3B78ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xCEFC4AD83CC5C682ULL},
};
// clang-format on

struct Distribution {
    std::string_view name;
    std::array<double, 4> first;
    double run_sum;
};

template <typename Draw>
Distribution measure(std::string_view name, Draw draw) {
    Distribution result{.name = name, .first = {}, .run_sum = 0.0};
    for (double& value : result.first) {
        value = draw();
    }
    for (int i = 0; i < kDistributionRun; ++i) {
        result.run_sum += draw();
    }
    return result;
}

// One fresh generator per distribution, so a change in one does not shift the
// others.
std::array<Distribution, 7> measure_all() {
    PhiloxRng g32{99, 0x12345678, 7};
    PhiloxRng g53{99, 0x12345678, 7};
    PhiloxRng ga{2026, 0x6A6D6D61, 1};
    PhiloxRng gb{2026, 0x6A6D6D61, 2};
    PhiloxRng gc{2026, 0x6A6D6D61, 0x100000003ULL};
    PhiloxRng ba{31, 0xBE7AD157, 5};
    PhiloxRng bb{31, 0xBE7AD157, 6};
    return {
        measure("gaussian Bits32", [&] { return g32.gaussian(10.0, 0.5); }),
        measure("gaussian Bits53", [&] { return g53.gaussian(10.0, 0.5, Precision::Bits53); }),
        measure("gamma_wh alpha=1", [&] { return ga.gamma_wh(1.0); }),
        measure("gamma_wh alpha=2.5 scale=3", [&] { return gb.gamma_wh(2.5, 3.0); }),
        measure("gamma_wh alpha=10 Bits53",
                [&] { return gc.gamma_wh(10.0, 1.0, Precision::Bits53); }),
        measure("beta_dist_approx 2,5", [&] { return ba.beta_dist_approx(2.0, 5.0); }),
        measure("beta_dist_approx 8,3 Bits53",
                [&] { return bb.beta_dist_approx(8.0, 3.0, Precision::Bits53); }),
    };
}

// clang-format off
constexpr std::array<Distribution, 7> kDistributions{{
    // DISTRIBUTION-TABLE
    Distribution{.name = "gaussian Bits32", .first = {0x1.498c18a9503c6p+3, 0x1.2a8bae9365247p+3, 0x1.3ed5a690e7547p+3, 0x1.46bd508876126p+3}, .run_sum = 0x1.38b0beb530788p+14},
    Distribution{.name = "gaussian Bits53", .first = {0x1.302906131c82fp+3, 0x1.4b0ae8f50b2efp+3, 0x1.481f045da1d13p+3, 0x1.6366dfb94cd6bp+3}, .run_sum = 0x1.37fe2656a6531p+14},
    Distribution{.name = "gamma_wh alpha=1", .first = {0x1.913ab8a3ecdd8p-2, 0x1.262015272a557p+1, 0x1.4e4874da692cdp-3, 0x1.08948a55613ap+1}, .run_sum = 0x1.0024bdf715761p+11},
    Distribution{.name = "gamma_wh alpha=2.5 scale=3", .first = {0x1.d8b495716e3a3p+0, 0x1.6daafa5b301dep-1, 0x1.26c03b152dfe1p+3, 0x1.f08ba56c77acdp+3}, .run_sum = 0x1.d0c0eac79bb47p+13},
    Distribution{.name = "gamma_wh alpha=10 Bits53", .first = {0x1.1e1079016e85dp+3, 0x1.542a3430a6a8fp+3, 0x1.fcd805615daf5p+3, 0x1.07b403d3180e5p+2}, .run_sum = 0x1.37a560424a97fp+14},
    Distribution{.name = "beta_dist_approx 2,5", .first = {0x1.6f14db0df6bd4p-4, 0x1.f1fcf55ef8b8dp-2, 0x1.7672adc5cae39p-2, 0x1.ad6d5e55c06f1p-4}, .run_sum = 0x1.211c5ebbcdf8ep+9},
    Distribution{.name = "beta_dist_approx 8,3 Bits53", .first = {0x1.26ec5cf6b0b6bp-1, 0x1.769fa8906e423p-1, 0x1.5150c144d5c4bp-1, 0x1.464e0ed9b89d6p-1}, .run_sum = 0x1.6b0883f7ab9e8p+10},
}};
// clang-format on

bool close(double got, double want) {
    return std::abs(got - want) <= kDistributionTol * std::abs(want);
}

void print_tables() {
    std::println("    // STREAM-TABLE");
    for (auto const& stream : kStreams) {
        std::println(
            "    Stream{{.seed = 0x{:08X}U, .key = 0x{:08X}U, .substream = 0x{:X}ULL, .hash = "
            "0x{:016X}ULL}},",
            stream.seed, stream.key, stream.substream,
            hash_stream(stream.seed, stream.key, stream.substream));
    }
    std::println("    // DISTRIBUTION-TABLE");
    for (auto const& dist : measure_all()) {
        std::println(
            "    Distribution{{.name = \"{}\", .first = {{0x{:a}, 0x{:a}, 0x{:a}, 0x{:a}}}, "
            ".run_sum = 0x{:a}}},",
            dist.name, dist.first[0], dist.first[1], dist.first[2], dist.first[3], dist.run_sum);
    }
}

}  // namespace

// argv keeps the type the standard gives main.
int main(int argc, char** argv) {  // NOLINT(misc-const-correctness)
    if (argc > 1 && std::string_view{argv[1]} == "--print") {
        print_tables();
        return 0;
    }

    int failures = 0;
    for (auto const& stream : kStreams) {
        auto const got = hash_stream(stream.seed, stream.key, stream.substream);
        if (got != stream.hash) {
            std::println(
                "stream seed=0x{:08X} key=0x{:08X} substream=0x{:X}: hash 0x{:016X}, "
                "expected 0x{:016X}",
                stream.seed, stream.key, stream.substream, got, stream.hash);
            ++failures;
        }
    }

    auto const measured = measure_all();
    for (std::size_t d = 0; d < measured.size(); ++d) {
        auto const& got = measured.at(d);
        auto const& want = kDistributions.at(d);
        for (std::size_t i = 0; i < got.first.size(); ++i) {
            if (!close(got.first.at(i), want.first.at(i))) {
                std::println("{}[{}]: got {:a}, expected {:a}", want.name, i, got.first.at(i),
                             want.first.at(i));
                ++failures;
            }
        }
        if (!close(got.run_sum, want.run_sum)) {
            std::println("{} sum of {} draws: got {:a}, expected {:a}", want.name, kDistributionRun,
                         got.run_sum, want.run_sum);
            ++failures;
        }
    }

    // gamma_wh's documented domain: the Wilson-Hilferty approximation is not
    // used below alpha = 1.
    bool threw = false;
    try {
        PhiloxRng rng{1};
        static_cast<void>(rng.gamma_wh(0.5));
    } catch (std::invalid_argument const&) {
        threw = true;
    }
    if (!threw) {
        std::println("gamma_wh(0.5) did not throw std::invalid_argument");
        ++failures;
    }

    if (failures == 0) {
        std::println("all {} streams and {} distributions match", kStreams.size(),
                     kDistributions.size());
    }
    return failures == 0 ? 0 : 1;
}
