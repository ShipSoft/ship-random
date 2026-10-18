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
// The streams for substreams below 2^32 were checked against ship-random 0.2.0
// when this test was written: both releases produce the same bits.

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
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x0ULL, .hash = 0xECE66468A7BD6F13ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x1ULL, .hash = 0x1EC2FA5C031301E2ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x7FFFFFFFULL, .hash = 0x3D7B11D607556A28ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x80000000ULL, .hash = 0xE1D7BB52A475E647ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFULL, .hash = 0xF77FF4EE741924E6ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0x100000000ULL, .hash = 0xFC7EE9C2EFE50DE3ULL},
    Stream{.seed = 0x00000000U, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xF11AAF7ED4C85AEDULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x0ULL, .hash = 0x858E665519CBBAB5ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x1ULL, .hash = 0x9E2D1C66BABCC4EEULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x7FFFFFFFULL, .hash = 0x3D8BCDC1EAB3B6E4ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x80000000ULL, .hash = 0x6093FA17B9B42010ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0xFFFFFFFFULL, .hash = 0xB4A9F67B003E8B31ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0x100000000ULL, .hash = 0x3D28461FE1BA8368ULL},
    Stream{.seed = 0x00000000U, .key = 0x47345EEDU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x0839A4ABA4EE5705ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x0ULL, .hash = 0xA01160795EF63419ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x1ULL, .hash = 0xA9A601E659356B33ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x7FFFFFFFULL, .hash = 0xC438ADC9E2848A2EULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x80000000ULL, .hash = 0xC496FEC67B84C57BULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0xFFFFFFFFULL, .hash = 0xE5C5C64061A0582DULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0x100000000ULL, .hash = 0x9D2683DE2239B3F5ULL},
    Stream{.seed = 0x00000000U, .key = 0x50594E49U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xAE4E58444DBA4E1EULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x0ULL, .hash = 0xB874AFF1A54A7AD4ULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x1ULL, .hash = 0x23AB2486FA76B5EDULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x7FFFFFFFULL, .hash = 0x09A926F3DB47922DULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x80000000ULL, .hash = 0x8EDF4D9352CAE649ULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0xFFFFFFFFULL, .hash = 0x603651785C1F6756ULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0x100000000ULL, .hash = 0xBAFEC3477E413AE6ULL},
    Stream{.seed = 0x00000000U, .key = 0x00000000U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xC139AD81E6A21702ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x0ULL, .hash = 0x4ED8F7060EF3655DULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x1ULL, .hash = 0xEF462FD7D1B4C1F2ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x7FFFFFFFULL, .hash = 0xFCF4BD59652D4084ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x80000000ULL, .hash = 0xB0C19F2C9ECAE567ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFULL, .hash = 0xBC124FC144EE3B3BULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0x100000000ULL, .hash = 0xB1244D5801B32371ULL},
    Stream{.seed = 0x00000001U, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x9EB822F447F9D8EDULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x0ULL, .hash = 0xA820A065915CED7BULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x1ULL, .hash = 0xFE27CAEEE159C5C3ULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x7FFFFFFFULL, .hash = 0xD201AA7DCCB43DD3ULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x80000000ULL, .hash = 0xBAF4F2389ED78C0FULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0xFFFFFFFFULL, .hash = 0xB9734548CFE6B246ULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0x100000000ULL, .hash = 0x93D64D90BF4668DCULL},
    Stream{.seed = 0x00000001U, .key = 0x47345EEDU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x1AD2580EBB0369BDULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x0ULL, .hash = 0xAAB67F56769D097CULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x1ULL, .hash = 0x4437649B68B8FBF4ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x7FFFFFFFULL, .hash = 0x4BB71134BDB63F3CULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x80000000ULL, .hash = 0x3BE72CBA79C5FF86ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0xFFFFFFFFULL, .hash = 0x662CA4A6F9051A04ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0x100000000ULL, .hash = 0x6F8E80BF27B96D77ULL},
    Stream{.seed = 0x00000001U, .key = 0x50594E49U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x41A3E55139AC1217ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x0ULL, .hash = 0x36B4B4C34F1F3B65ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x1ULL, .hash = 0xD571EF4E9CCDE5DEULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x7FFFFFFFULL, .hash = 0x895948CF4E478F02ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x80000000ULL, .hash = 0x2733F5ADC5645179ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0xFFFFFFFFULL, .hash = 0x2FF5693BB92B7DFAULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0x100000000ULL, .hash = 0xCB508B7EB46C6993ULL},
    Stream{.seed = 0x00000001U, .key = 0x00000000U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x48E0D8DF05E69561ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x0ULL, .hash = 0x2790BEF0363C2836ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x1ULL, .hash = 0x72168AB1C888F302ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x7FFFFFFFULL, .hash = 0x6E604E7C18769698ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x80000000ULL, .hash = 0xD503416CDCC374FEULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFULL, .hash = 0x75F42C237DBFF4C0ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0x100000000ULL, .hash = 0x24ACAF44F32B710CULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0xBEEFCAFEU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xC1C9C7FF9C1151E5ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x0ULL, .hash = 0xA3F22FFF9655CC90ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x1ULL, .hash = 0x86E6E6E3B76638C4ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x7FFFFFFFULL, .hash = 0x616D2F4C01A812E5ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x80000000ULL, .hash = 0x5414A6BB26CF2029ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0xFFFFFFFFULL, .hash = 0xA2D375524E837772ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0x100000000ULL, .hash = 0x65485596B0E9DD2DULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x47345EEDU, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0x0C7A679D53DD40ABULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x0ULL, .hash = 0xF5A2C77916E41334ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x1ULL, .hash = 0x6DE7D5C60D6F2D59ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x7FFFFFFFULL, .hash = 0xB7A6F42E58FBB115ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x80000000ULL, .hash = 0x11A48E67BC10A848ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0xFFFFFFFFULL, .hash = 0x1571E03F53D77B14ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0x100000000ULL, .hash = 0x1085736A1DFD9199ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x50594E49U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xBE916740BDC4DFB4ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x0ULL, .hash = 0xF2169DFBBDBC1032ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x1ULL, .hash = 0x5AFF017285074BD2ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x7FFFFFFFULL, .hash = 0x1EB4CF37754C836FULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x80000000ULL, .hash = 0xDB67A8BA33DBDE8BULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0xFFFFFFFFULL, .hash = 0x87C4CB1B6DC16989ULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0x100000000ULL, .hash = 0xB40AA006F0839F3DULL},
    Stream{.seed = 0xFFFFFFFFU, .key = 0x00000000U, .substream = 0xFFFFFFFFFFFFFFFFULL, .hash = 0xA4878091FF7E6EF4ULL},
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
    Distribution{.name = "gamma_wh alpha=1", .first = {0x1.913ab8a3ecdd8p-2, 0x1.262015272a554p+1, 0x1.4e4874da692cdp-3, 0x1.08948a55613ap+1}, .run_sum = 0x1.0024bdf715761p+11},
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
