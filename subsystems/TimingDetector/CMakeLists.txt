# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_library(TimingDetector
    src/TimingDetectorFactory.cpp
)

target_include_directories(TimingDetector
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(TimingDetector
    PUBLIC
        GeoModelCore::GeoModelKernel
    PRIVATE
        GeoModelTools::GeoModelXml
)

# Copy GMX data file and DTD to build directory.
# The DTD must be co-located with the GMX file (Xerces resolves SYSTEM identifiers
# relative to the document location).
set(GMX_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/data")
file(MAKE_DIRECTORY "${GMX_OUT_DIR}")
configure_file(data/timing_detector.gmx "${GMX_OUT_DIR}/timing_detector.gmx" COPYONLY)

# Locate geomodel.dtd from the GeoModelTools install tree
get_filename_component(_gt_prefix "${GeoModelTools_DIR}/../../.." ABSOLUTE)
set(_geomodel_dtd "${_gt_prefix}/share/GeoModelXml/geomodel.dtd")
if(EXISTS "${_geomodel_dtd}")
    configure_file("${_geomodel_dtd}" "${GMX_OUT_DIR}/geomodel.dtd" COPYONLY)
else()
    message(WARNING "geomodel.dtd not found at ${_geomodel_dtd}; TimingDetector GMX parsing may fail")
endif()

# Embed the build-time GMX path as a compile-time string constant
target_compile_definitions(TimingDetector PRIVATE
    TIMING_DETECTOR_GMX="${GMX_OUT_DIR}/timing_detector.gmx"
)

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_timingdetector test_timingdetector.cpp)
    target_link_libraries(test_timingdetector PRIVATE TimingDetector SHiPGeometry Catch2::Catch2WithMain)
    catch_discover_tests(test_timingdetector)
endif()
