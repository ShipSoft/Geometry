// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPGeometry.h"

#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelWrite/WriteGeoModel.h>

#include <filesystem>
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
        std::cerr << "Error: Geometry not yet implemented" << std::endl;
        return 1;
    }

    // Remove existing output file
    if (std::filesystem::exists(outputFile)) {
        std::filesystem::remove(outputFile);
    }

    std::cout << "Writing geometry to " << outputFile << std::endl;

    GMDBManager db(outputFile);
    GeoModelIO::WriteGeoModel writer(db);

    // Traverse the geometry tree
    world->exec(&writer);

    // Save to database
    writer.saveToDB();

    std::cout << "Done." << std::endl;
    return 0;
}
