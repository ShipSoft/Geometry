// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/DecayVolume/DecayVolumeFactory.h"

#include "SHiP/geometry/DecayVolume/SBTConstants.h"
#include "SHiP/geometry/DecayVolume/SBTEnvelope.h"
#include "SHiP/geometry/DecayVolume/SBTSensorBuilder.h"
#include "SHiP/geometry/DecayVolume/SBTStructureBuilder.h"
#include "SHiP/geometry/Materials.h"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/GeoPhysVol.h>
#include <GeoModelKernel/Units.h>

namespace SHiP::geometry {

using namespace GeoModelKernelUnits;

DecayVolumeFactory::DecayVolumeFactory(Materials& materials) : m_materials(materials) {}

GeoPhysVol* DecayVolumeFactory::build() {
    const GeoMaterial* air = m_materials.requireMaterial(sbt::kAirMaterial);
    const GeoMaterial* steel = m_materials.requireMaterial(sbt::kSteelMaterial);
    const GeoMaterial* alMat = m_materials.requireMaterial(sbt::kWallMaterial);
    const GeoMaterial* labMat = m_materials.requireMaterial(sbt::kScintillatorMaterial);
    const GeoMaterial* helium = m_materials.requireMaterial(sbt::kHeliumMaterial);

    // ── Air container ────────────────────────────────────────────────────
    // The container is the experiment's fixed envelope allocation for the
    // decay region. Static_asserts in SBTConstants.h guarantee the SBT
    // structure cannot outgrow it.
    auto* containerBox =
        new GeoBox(sbt::kEnvelopeHalfX * mm, sbt::kEnvelopeHalfY * mm, sbt::kEnvelopeHalfZ * mm);
    auto* containerLog = new GeoLogVol("/SHiP/decay_volume", containerBox, air);
    auto* container = new GeoPhysVol(containerLog);

    // ── SBT steel structure + LAB sensors ────────────────────────────────
    SBTStructureBuilder::build(container, steel);
    SBTSensorBuilder::build(container, alMat, labMat);

    // ── Central helium decay region ──────────────────────────────────────
    // The helium is not sized independently — it is *derived* from where the
    // SBT material actually is (SBTEnvelope, which reads the same placement
    // primitives in SBTConstants.h that the two builders above place with).
    // It fills the interior exactly: no protrusion into steel or scintillator,
    // and no arbitrary margin left behind beyond heliumClearance (1 um, just
    // enough to avoid coincident surfaces).
    //
    // It is a stack of GeoTraps rather than one, because the inner surface is
    // not linear in Z: the side containers present a flat outer face over the
    // first zSplitOffset() of every sub-frustum so as to clear the columns, so
    // the free region is a sawtooth. One GeoTrap per envelope segment tracks
    // it exactly; a single frustum could not.
    sbt::buildHelium(container, helium);

    return container;
}

}  // namespace SHiP::geometry
