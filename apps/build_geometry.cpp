// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration
//
// Build the SHiP GeoModel geometry and serialise it to a GeoModel SQLite (.db).
//
// Usage:
//   build_geometry [-o <file>] [--standalone] [Name ...]
//   build_geometry --list
//
//   build_geometry                          # full detector           -> ship_geometry.db
//   build_geometry -o out.db                # full detector           -> out.db
//   build_geometry Target Magnet            # those two, world-placed -> ship_selection.db
//   build_geometry --standalone Calorimeter # on its own, local frame -> Calorimeter.db
//
// Every non-flag token names a subsystem, spelled as --list spells it; the
// output file is set only with -o. Named subsystems are placed in the world at
// the positions their descriptors declare, unless --standalone is given, which
// builds exactly one subsystem on its own at the origin.

#include "SHiPGeometry/SHiPGeometry.h"
#include "SHiPGeometry/SubsystemRegistry.h"

#include <GeoModelDBManager/GMDBManager.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelWrite/WriteGeoModel.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <vector>

namespace {

/**
 * @brief Send every log line to stderr.
 *
 * This tool's stdout carries program output (--list), so logs belong on stderr:
 * redirecting the geometry listing must not swallow an error message. spdlog's
 * default logger writes to *stdout*, so it has to be replaced rather than used.
 * The pattern is spdlog's default minus the logger name, which is noise here.
 */
void logToStderr() {
    spdlog::set_default_logger(spdlog::stderr_color_mt("build_geometry"));
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
}

/// @brief Print how to call the tool. @param out Where to write it.
void printUsage(std::ostream& out) {
    out << "Usage:\n"
           "  build_geometry [-o <file>] [--standalone] [Name ...]\n"
           "  build_geometry --list\n"
           "\n"
           "  -o, --output <file>  Write the geometry here. Defaults to\n"
           "                       ship_geometry.db for the full detector,\n"
           "                       <Name>.db with --standalone, and\n"
           "                       ship_selection.db for a selection.\n"
           "      --standalone     Build exactly one named subsystem on its own,\n"
           "                       in its local frame at the origin, with no\n"
           "                       world around it.\n"
           "      --list           Print the available subsystem names.\n"
           "  -h, --help           Print this message.\n"
           "\n"
           "Every non-flag token names a subsystem. With none named, the complete\n"
           "detector is built; named subsystems are placed in the world at their\n"
           "declared positions unless --standalone is given.\n";
}

}  // namespace

/**
 * @brief Entry point: parse the arguments, build, and serialise to SQLite.
 *
 * @param argc Argument count.
 * @param argv Flags (`--list`, `-o <file>`, `--standalone`, `--help`) and any
 *             number of subsystem names as spelled by `--list`.
 * @return 0 on success, 1 on a bad argument, an unknown subsystem, a null
 *         geometry, or a filesystem error preparing the output file.
 */
int main(int argc, char* argv[]) {
    logToStderr();

    std::string outputFile;
    std::vector<std::string> names;  // requested subsystem(s)
    bool standalone = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--list") {
            // Program output, not a log line: plain stdout so it stays pipeable.
            for (const auto& n : SHiPGeometry::subsystemNames())
                std::cout << n << "\n";
            return 0;
        }
        if (arg == "-h" || arg == "--help") {
            printUsage(std::cout);
            return 0;
        }
        if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                spdlog::error("{} needs a file name.", arg);
                printUsage(std::cerr);
                return 1;
            }
            outputFile = argv[++i];
        } else if (arg == "--standalone") {
            standalone = true;
        } else if (arg.starts_with("-")) {
            spdlog::error("Unknown option: '{}'", arg);
            printUsage(std::cerr);
            return 1;
        } else {
            names.push_back(arg);
        }
    }

    if (standalone && names.size() != 1) {
        spdlog::error("--standalone builds one subsystem, but {} were named.", names.size());
        printUsage(std::cerr);
        return 1;
    }

    // Check the names here rather than leaving it to the builders, so that the
    // list of available subsystems is offered for a mis-spelled name and not
    // for, say, a configuration file that failed to parse.
    const auto known = SHiPGeometry::subsystemNames();
    bool anyUnknown = false;
    for (const auto& name : names) {
        if (std::find(known.begin(), known.end(), name) == known.end()) {
            spdlog::error("Unknown subsystem: '{}'", name);
            anyUnknown = true;
        }
    }
    if (anyUnknown) {
        spdlog::info("Available subsystems:");
        for (const auto& n : known)
            spdlog::info("  {}", n);
        return 1;
    }

    GeoVPhysVol* geometry = nullptr;
    std::string label;

    try {
        if (standalone) {
            // One subsystem by itself: no world, so it sits at the origin
            // rather than at its position in the detector.
            geometry = SHiPGeometry::buildSubsystem(names[0]);
            label = names[0] + " (standalone)";
            if (outputFile.empty())
                outputFile = names[0] + ".db";
        } else if (names.empty()) {
            // Default: the complete detector.
            SHiPGeometry::SHiPGeometryBuilder builder;
            geometry = builder.build();
            label = "full SHiP geometry";
            if (outputFile.empty())
                outputFile = "ship_geometry.db";
        } else {
            // A selection, assembled into the world at the placements the
            // subsystems declare (the world/cavern is always included).
            geometry = SHiPGeometry::assembleGeometry(names);
            label = names.size() == 1
                        ? names[0] + " (in the world)"
                        : "selection (" + std::to_string(names.size()) + " subsystems)";
            if (outputFile.empty())
                outputFile = "ship_selection.db";
        }
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
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
