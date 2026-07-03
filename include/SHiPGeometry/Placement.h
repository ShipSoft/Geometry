// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <string>

namespace SHiPGeometry {

/// Place @p child under @p mother using the repo's standard tag sequence: a
/// name tag, an identifier tag, a transform, then the child node. This is the
/// idiom repeated throughout the subsystem factories and the top-level builder.
inline void placeChild(GeoVPhysVol* mother, GeoVPhysVol* child, const std::string& name, int id,
                       const GeoTrf::Transform3D& transform) {
    mother->add(new GeoNameTag(name));
    mother->add(new GeoIdentifierTag(id));
    mother->add(new GeoTransform(transform));
    mother->add(child);
}

}  // namespace SHiPGeometry
