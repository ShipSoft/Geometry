// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// Build the SHiP GeoModel geometry and serialise it to a GeoModel SQLite (.db).
//
// Usage:
//   build_geometry                     # full detector          -> ship_geometry.db
//   build_geometry out.db              # full detector          -> out.db
//   build_geometry Calorimeter         # just that subsystem    -> Calorimeter.db
//   build_geometry Calorimeter c.db    # just that subsystem    -> c.db
//   build_geometry --list              # list available subsystems
//
// A token ending in ".db" is the output file; any other token names a
// subsystem. With no subsystem named, the complete detector is built.

#include "SHiPGeometry/SHiPGeometry.h"
#include "SHiPGeometry/SubsystemRegistry.h"

#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelWrite/WriteGeoModel.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}
}  // namespace

int main(int argc, char* argv[]) {
    std::string outputFile;
    std::vector<std::string> names;  // requested subsystem(s)

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--list") {
            for (const auto& n : SHiPGeometry::subsystemNames())
                std::cout << n << "\n";
            return 0;
        }
        if (endsWith(arg, ".db")) {
            outputFile = arg;
        } else {
            names.push_back(arg);
        }
    }

    GeoVPhysVol* geometry = nullptr;
    std::string label;

    if (names.empty()) {
        // Default: the complete detector (unchanged behaviour).
        SHiPGeometry::SHiPGeometryBuilder builder;
        geometry = builder.build();
        label = "full SHiP geometry";
        if (outputFile.empty())
            outputFile = "ship_geometry.db";
    } else if (names.size() == 1) {
        // A single subsystem, on its own (local frame).
        try {
            geometry = SHiPGeometry::buildSubsystem(names[0]);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\nAvailable subsystems:\n";
            for (const auto& n : SHiPGeometry::subsystemNames())
                std::cerr << "  " << n << "\n";
            return 1;
        }
        label = names[0];
        if (outputFile.empty())
            outputFile = names[0] + ".db";
    } else {
        std::cerr << "Error: build at most one subsystem at a time (got " << names.size() << ").\n";
        return 1;
    }

    if (!geometry) {
        std::cerr << "Error: geometry is null (not yet implemented?)." << std::endl;
        return 1;
    }

    if (std::filesystem::exists(outputFile)) {
        std::filesystem::remove(outputFile);
    }

    std::cout << "Writing " << label << " to " << outputFile << std::endl;
    GMDBManager db(outputFile);
    GeoModelIO::WriteGeoModel writer(db);
    geometry->exec(&writer);
    writer.saveToDB();
    std::cout << "Done." << std::endl;
    return 0;
}
