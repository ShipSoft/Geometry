# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration

# fullSimLight already needs Qt, so the figures add no dependency to the environment.
find_package(Qt6 REQUIRED COMPONENTS Gui)

add_executable(make_figure_pdf make_figure_pdf.cpp)
target_link_libraries(make_figure_pdf PRIVATE Qt6::Gui)
