<!--
SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration

SPDX-License-Identifier: LGPL-3.0-or-later
-->

# ship-random

Random number helpers shared by the SHiP software: the simulation
([aegir](https://github.com/ShipSoft/aegir) and
[aegir-genie](https://github.com/ShipSoft/aegir-genie)), digitisation
([Shannon](https://github.com/ShipSoft/Shannon)) and reconstruction
([Trout](https://github.com/ShipSoft/Trout)). Each of these used to carry its
own copy of the same generator class. They now take it from here.

It is header-only and depends only on
[Random123](https://github.com/DEShawResearch/random123).

## `ship::random::PhiloxRng`

A counter-based generator built on Random123's Philox 4x32. It has no shared
state: a job seeds a fresh instance per event (or per spill, hit, …) from
the seed, a stream key and a counter offset, so results are reproducible and
the same whatever the number of threads.

```cpp
#include <ship/random/philox_rng.hpp>

// seed, stream key, sub-stream (e.g. the event number)
ship::random::PhiloxRng rng{seed, 0xBEEFCAFE, event_number};
double const u = rng.uniform();            // [0, 1), 32-bit resolution
double const x = rng.uniform(-1.0, 1.0);   // [lo, hi)
double const t = rng.uniform53(0.0, 5e9);  // [lo, hi), full double resolution
double const g = rng.gaussian(0.0, 1.0);   // mean, sigma
```

Use a different stream key for each independent purpose (each generator,
each detector) so that their sequences are uncorrelated even with the same
seed.

The numbers drawn are part of the physics output of every package that uses
this class. `tests/philox_rng_test.cpp` pins the sequences: they match, bit
for bit, the copies the packages used before. A change that alters them
changes simulated, digitised and reconstructed events, and needs a new minor
version.

## Using it

With pixi or conda, add `ship-random` from
[`prefix.dev/ship`](https://prefix.dev/channels/ship). In CMake:

```cmake
find_package(SHiPRandom 0.1 REQUIRED)
target_link_libraries(my_target PRIVATE SHiP::Random)
```

## Building

```bash
pixi run test      # configure, build and run the tests
pixi run install   # install into the pixi environment
```

## License

LGPL-3.0-or-later, see [`LICENSE.md`](LICENSE.md).
