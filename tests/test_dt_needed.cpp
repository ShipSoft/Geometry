// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// Check that loading libship_geometry on its own pulls in every subsystem
// library, i.e. that the subsystems are in libship_geometry's own DT_NEEDED.
//
// Usage: test_dt_needed <path/to/libship_geometry.so> <subsystem soname> ...
//
// This executable deliberately does not link ship_geometry: a CMake-linked
// consumer also links every subsystem directly, through ship_geometry's link
// interface, so the registry tests cannot catch a library whose own link line
// dropped them. Here the library is dlopen()ed, as
// a plugin or a hand-written -lship_geometry would see it, and each subsystem
// must then already be resident (RTLD_NOLOAD loads nothing itself).

#include <dlfcn.h>
#include <iostream>

// The paths come from ctest, not from an untrusted user, and the test is
// single-threaded.
// NOLINTBEGIN(clang-analyzer-optin.taint.GenericTaint,concurrency-mt-unsafe)
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <libship_geometry> <subsystem soname> ...\n";
        return 2;
    }

    if (dlopen(argv[1], RTLD_NOW | RTLD_LOCAL) == nullptr) {
        std::cerr << "Could not load " << argv[1] << ": " << dlerror() << "\n";
        return 1;
    }

    int missing = 0;
    for (int i = 2; i < argc; ++i) {
        if (dlopen(argv[i], RTLD_NOW | RTLD_NOLOAD) == nullptr) {
            std::cerr << argv[i] << " was not loaded with " << argv[1]
                      << ": it is missing from its DT_NEEDED\n";
            ++missing;
        }
    }
    return missing == 0 ? 0 : 1;
}
// NOLINTEND(clang-analyzer-optin.taint.GenericTaint,concurrency-mt-unsafe)
