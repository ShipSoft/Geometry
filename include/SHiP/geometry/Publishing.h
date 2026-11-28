// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#pragma once

#include <GeoModelKernel/GeoPublisher.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

class GeoVFullPhysVol;

namespace SHiP::geometry {

/**
 * @brief Full physical volumes that subsystems publish, one GeoPublisher per
 * system.
 *
 * Publishing gives a volume a stable key that readers can look up without
 * walking the tree. build_geometry writes the publishers into the .db next to
 * the tree, as GeoModel's PublishedFullPhysVols_<system> tables, where ACTS's
 * GeoModel plugin finds them (GeoModelTree::VolumePublisher).
 */
class Publishers {
   public:
    /// The publisher for @p system, created on first use.
    GeoPublisher& system(const std::string& name);

    /// Every publisher, in system-name order, for WriteGeoModel::saveToDB.
    [[nodiscard]] std::vector<GeoPublisher*> all() const;

   private:
    std::map<std::string, std::unique_ptr<GeoPublisher>> m_publishers;
};

/**
 * @brief Collects what subsystem factories publish on this thread while it is
 * alive.
 *
 * Scopes nest: the innermost one receives the volumes, and the outer one is
 * restored on destruction.
 */
class PublishingScope {
   public:
    explicit PublishingScope(Publishers& publishers) noexcept;
    ~PublishingScope();

    PublishingScope(const PublishingScope&) = delete;
    PublishingScope& operator=(const PublishingScope&) = delete;
    PublishingScope(PublishingScope&&) = delete;
    PublishingScope& operator=(PublishingScope&&) = delete;

   private:
    Publishers* m_previous;
};

/**
 * @brief Publish @p volume under @p key in @p system.
 *
 * Goes to the innermost PublishingScope on this thread. Without one, nothing
 * is recorded, so building the geometry for simulation alone needs no setup.
 * Throws (from GeoPublisher) if @p key is already taken by another volume.
 */
void publish(const std::string& system, const std::string& key, GeoVFullPhysVol* volume);

}  // namespace SHiP::geometry
