# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# Shared recipe: library (globs src/*.cpp, incl. SNDEnvelope.cpp), include dirs,
# kernel link, Catch test.
ship_add_subsystem(NeutrinoDetector)

# NeutrinoDetector parses SD.toml (the SND reservation envelope) via SNDEnvelope
# (toml++): link toml++, stage and install SD.toml, and define
# SD_TOML_DEFAULT_PATH / SD_TOML_INSTALL_PATH for resolveConfigPath().
ship_add_toml_config(NeutrinoDetector SD.toml SD)
