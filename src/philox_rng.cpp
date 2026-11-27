// SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Random123/philox.h>
#include <SHiP/random/philox_rng.hpp>
#include <array>
#include <cstdint>

namespace SHiP::random {

// The drawn sequences depend on the round count, so a Random123 that changed
// its default would change every simulated, digitised and reconstructed event.
static_assert(r123::Philox4x32::rounds == 10);

std::array<std::uint32_t, 4> philox_block(std::array<std::uint32_t, 4> const& ctr,
                                          std::array<std::uint32_t, 2> const& key) {
    r123::Philox4x32::ctr_type const c{{ctr[0], ctr[1], ctr[2], ctr[3]}};
    r123::Philox4x32::key_type const k{{key[0], key[1]}};
    auto const out = r123::Philox4x32{}(c, k);
    return {out[0], out[1], out[2], out[3]};
}

}  // namespace SHiP::random
