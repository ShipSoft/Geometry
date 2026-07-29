# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Shared recipe: library (globs src/*.cpp), include dirs, kernel link, Catch test.
ship_add_subsystem(MuonShield)

# MuonShield parses MS.toml via MuonShieldConfig (toml++): link toml++, stage
# and install MS.toml, and define MS_TOML_DEFAULT_PATH / MS_TOML_INSTALL_PATH.
ship_add_toml_config(MuonShield MS.toml MS)
