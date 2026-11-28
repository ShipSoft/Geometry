// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiP/geometry/Publishing.h"

#include <GeoModelKernel/GeoVFullPhysVol.h>

#include <memory>
#include <string>
#include <vector>

namespace SHiP::geometry {

namespace {

// The innermost active scope's target on this thread, or null.
Publishers*& currentPublishers() noexcept {
    thread_local Publishers* current = nullptr;
    return current;
}

}  // namespace

GeoPublisher& Publishers::system(const std::string& name) {
    auto& slot = m_publishers[name];
    if (!slot) {
        slot = std::make_unique<GeoPublisher>();
        slot->setName(name);
    }
    return *slot;
}

std::vector<GeoPublisher*> Publishers::all() const {
    std::vector<GeoPublisher*> result;
    result.reserve(m_publishers.size());
    for (const auto& [name, publisher] : m_publishers) {
        result.push_back(publisher.get());
    }
    return result;
}

PublishingScope::PublishingScope(Publishers& publishers) noexcept
    : m_previous{currentPublishers()} {
    currentPublishers() = &publishers;
}

PublishingScope::~PublishingScope() {
    currentPublishers() = m_previous;
}

void publish(const std::string& system, const std::string& key, GeoVFullPhysVol* volume) {
    if (auto* const current = currentPublishers()) {
        current->system(system).publishNode(volume, key);
    }
}

}  // namespace SHiP::geometry
