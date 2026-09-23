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
//   build_geometry Target Magnet       # assemble a selection    -> ship_selection.db
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
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <vector>

int main(int argc, char* argv[]) {
    std::string outputFile;
    std::vector<std::string> names;  // requested subsystem(s)

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--list") {
            // Program output, not a log line: plain stdout so it stays pipeable.
            for (const auto& n : SHiPGeometry::subsystemNames())
                std::cout << n << "\n";
            return 0;
        }
        if (arg.ends_with(".db")) {
            outputFile = arg;
        } else {
            names.push_back(arg);
        }
    }

    GeoVPhysVol* geometry = nullptr;
    std::string label;

    try {
        if (names.empty()) {
            // Default: the complete detector (unchanged behaviour).
            SHiPGeometry::SHiPGeometryBuilder builder;
            geometry = builder.build();
            label = "full SHiP geometry";
            if (outputFile.empty())
                outputFile = "ship_geometry.db";
        } else if (names.size() == 1) {
            // A single subsystem, on its own (local frame).
            geometry = SHiPGeometry::buildSubsystem(names[0]);
            label = names[0];
            if (outputFile.empty())
                outputFile = names[0] + ".db";
        } else {
            // Several subsystems: assemble them into the world at their declared
            // placements (the world/cavern is always included).
            geometry = SHiPGeometry::assembleGeometry(names);
            label = "selection (" + std::to_string(names.size()) + " subsystems)";
            if (outputFile.empty())
                outputFile = "ship_selection.db";
        }
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
        spdlog::info("Available subsystems:");
        for (const auto& n : SHiPGeometry::subsystemNames())
            spdlog::info("  {}", n);
        return 1;
    }

    if (!geometry) {
        spdlog::error("Geometry is null (not yet implemented?).");
        return 1;
    }

    // The non-throwing overload: remove() is a harmless no-op when the path does
    // not exist, and a locked or permission-denied file is reported rather than
    // terminating the tool.
    std::error_code ec;
    std::filesystem::remove(outputFile, ec);
    if (ec) {
        spdlog::error("Could not remove existing {}: {}", outputFile, ec.message());
        return 1;
    }

    spdlog::info("Writing {} to {}", label, outputFile);
    GMDBManager db(outputFile);
    GeoModelIO::WriteGeoModel writer(db);
    geometry->exec(&writer);
    writer.saveToDB();
    spdlog::info("Done.");
    return 0;
}
