# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Library + test (all src/*.cpp); the test runs from the build dir so it finds
# the staged calo.toml.
ship_add_subsystem(Calorimeter TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Calorimeter parses calo.toml via CalorimeterConfig (toml++): link toml++,
# stage and install calo.toml, and define CALO_TOML_DEFAULT_PATH /
# CALO_TOML_INSTALL_PATH.
ship_add_toml_config(Calorimeter calo.toml CALO)
