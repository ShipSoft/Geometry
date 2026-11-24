// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/GeoDefinitions.h>
#include <GeoModelKernel/GeoIdentifierTag.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoNameTag.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoTransform.h>
#include <GeoModelKernel/GeoVPhysVol.h>

#include <string>

namespace SHiP::geometry {

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

/// A physical volume around a new logical volume called @p name.
inline GeoPhysVol* makeVolume(const std::string& name, const GeoShape* shape,
                              const GeoMaterial* material) {
    return new GeoPhysVol(new GeoLogVol(name, shape, material));
}

/// Places children under one mother with placeChild(), giving them
/// consecutive identifiers in placement order.
class Placer {
   public:
    explicit Placer(GeoVPhysVol* mother, int firstId = 0) : m_mother{mother}, m_nextId{firstId} {}

    /// Place @p child as @p name.
    void place(GeoVPhysVol* child, const std::string& name, const GeoTrf::Transform3D& transform) {
        placeChild(m_mother, child, name, m_nextId++, transform);
    }

    /// Place @p child under the name of its logical volume.
    void place(GeoVPhysVol* child, const GeoTrf::Transform3D& transform) {
        place(child, child->getLogVol()->getName(), transform);
    }

   private:
    GeoVPhysVol* m_mother;
    int m_nextId;
};

}  // namespace SHiP::geometry
