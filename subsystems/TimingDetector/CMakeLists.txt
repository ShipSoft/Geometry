# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

add_library(TimingDetector src/TimingDetectorFactory.cpp)

target_include_directories(
    TimingDetector
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    TimingDetector
    PUBLIC GeoModelCore::GeoModelKernel
    PRIVATE GeoModelTools::GeoModelXml
)

# Copy GMX data file and DTD to build directory.
# The DTD must be co-located with the GMX file (Xerces resolves SYSTEM identifiers
# relative to the document location).
set(GMX_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/data")
file(MAKE_DIRECTORY "${GMX_OUT_DIR}")
configure_file(
    data/timing_detector.gmx
    "${GMX_OUT_DIR}/timing_detector.gmx"
    COPYONLY
)

# Locate geomodel.dtd from the GeoModelTools install tree
get_filename_component(_gt_prefix "${GeoModelTools_DIR}/../../.." ABSOLUTE)
set(_geomodel_dtd "${_gt_prefix}/share/GeoModelXml/geomodel.dtd")
if(EXISTS "${_geomodel_dtd}")
    configure_file("${_geomodel_dtd}" "${GMX_OUT_DIR}/geomodel.dtd" COPYONLY)
else()
    message(
        WARNING
        "geomodel.dtd not found at ${_geomodel_dtd}; TimingDetector GMX parsing may fail"
    )
endif()

# Bake the build-tree GMX path as a compile-time fallback so out-of-source
# builds always find timing_detector.gmx even when the CWD doesn't contain it.
target_compile_definitions(
    TimingDetector
    PRIVATE
        # Build-tree fallback (always valid during development and CI builds).
        TIMING_DETECTOR_GMX_DEFAULT_PATH="${GMX_OUT_DIR}/timing_detector.gmx"
        # Install-time fallback: set to the installed data directory so that
        # deployed builds (where the build tree is absent) can find the GMX.
        TIMING_DETECTOR_GMX_INSTALL_PATH="${CMAKE_INSTALL_FULL_DATADIR}/SHiPGeometry/TimingDetector/timing_detector.gmx"
)

# Install the GMX (and the co-located geomodel.dtd that Xerces will resolve
# against it) into the data directory so installed deployments find them.
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/timing_detector.gmx
    DESTINATION ${CMAKE_INSTALL_DATADIR}/SHiPGeometry/TimingDetector
)
if(EXISTS "${_geomodel_dtd}")
    install(
        FILES "${_geomodel_dtd}"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/SHiPGeometry/TimingDetector
    )
endif()

if(BUILD_TESTING)
    include(Catch)
    add_executable(test_timingdetector test_timingdetector.cpp)
    target_link_libraries(
        test_timingdetector
        PRIVATE TimingDetector SHiPGeometry Catch2::Catch2WithMain
    )
    catch_discover_tests(test_timingdetector)
endif()
