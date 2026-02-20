// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPGeometry.h"
#include <GeoModelIO/GeoModelWrite.h>
#include <iostream>

int main(int argc, char* argv[]) {
    std::string outputFile = "ship_geometry.db";
    if (argc > 1) {
        outputFile = argv[1];
    }

    std::cout << "Building SHiP geometry..." << std::endl;

    SHiPGeometry::SHiPGeometryBuilder builder;
    GeoPhysVol* world = builder.build();

    if (!world) {
        std::cerr << "Error: Failed to build geometry" << std::endl;
        return 1;
    }

    std::cout << "Writing geometry to " << outputFile << std::endl;
    // TODO: Write geometry to file using GeoModelIO

    std::cout << "Done." << std::endl;
    return 0;
}