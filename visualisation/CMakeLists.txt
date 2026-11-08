# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# A separate project, so that the geometry library and its release package do
# not link Qt because of this tool. fullSimLight already needs Qt, so the figures
# add no dependency to the environment.
cmake_minimum_required(VERSION 3.16)
project(ShipFigurePdf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Gui)

add_executable(make_figure_pdf make_figure_pdf.cpp)
target_link_libraries(make_figure_pdf PRIVATE Qt6::Gui)
