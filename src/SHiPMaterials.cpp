// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"

#include <GeoModelKernel/GeoElement.h>
#include <GeoModelKernel/GeoMaterial.h>
#include <GeoModelKernel/Units.h>

#include <stdexcept>

namespace SHiPGeometry {

SHiPMaterials::SHiPMaterials() {
    createElements();
    createMaterials();
}

GeoElement* SHiPMaterials::getElement(const std::string& name) const {
    auto it = m_elements.find(name);
    return (it != m_elements.end()) ? it->second : nullptr;
}

GeoMaterial* SHiPMaterials::getMaterial(const std::string& name) const {
    auto it = m_materials.find(name);
    return (it != m_materials.end()) ? it->second : nullptr;
}

GeoMaterial* SHiPMaterials::requireMaterial(const std::string& name) const {
    auto* mat = getMaterial(name);
    if (!mat) {
        throw std::runtime_error("Material not found: " + name);
    }
    return mat;
}

void SHiPMaterials::createElements() {
    // Create all elements needed for SHiP
    m_elements["Hydrogen"] = new GeoElement(
        "Hydrogen", "H", 1, 1.008 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Helium"] = new GeoElement(
        "Helium", "He", 2, 4.003 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Carbon"] = new GeoElement(
        "Carbon", "C", 6, 12.011 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Nitrogen"] = new GeoElement(
        "Nitrogen", "N", 7, 14.007 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Oxygen"] = new GeoElement(
        "Oxygen", "O", 8, 15.999 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Silicon"] = new GeoElement(
        "Silicon", "Si", 14, 28.085 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Argon"] = new GeoElement(
        "Argon", "Ar", 18, 39.948 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Calcium"] = new GeoElement(
        "Calcium", "Ca", 20, 40.078 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Titanium"] = new GeoElement(
        "Titanium", "Ti", 22, 47.867 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Chromium"] = new GeoElement(
        "Chromium", "Cr", 24, 51.996 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Manganese"] = new GeoElement(
        "Manganese", "Mn", 25, 54.938 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Iron"] = new GeoElement(
        "Iron", "Fe", 26, 55.845 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Nickel"] = new GeoElement(
        "Nickel", "Ni", 28, 58.693 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Copper"] = new GeoElement(
        "Copper", "Cu", 29, 63.546 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Niobium"] = new GeoElement(
        "Niobium", "Nb", 41, 92.906 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Molybdenum"] = new GeoElement(
        "Molybdenum", "Mo", 42, 95.95 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Tantalum"] = new GeoElement(
        "Tantalum", "Ta", 73, 180.948 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Tungsten"] = new GeoElement(
        "Tungsten", "W", 74, 183.84 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);

    // Aluminium
    m_elements["Aluminium"] = new GeoElement(
        "Aluminium", "Al", 13.0, 26.982 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
}

void SHiPMaterials::createMaterials() {
    // Air (density 1.29e-3 g/cm³): N 75.5%, O 23.1%, Ar 1.4%
    GeoMaterial* air =
        new GeoMaterial("Air", 1.29e-3 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    air->add(m_elements["Nitrogen"], 0.755);
    air->add(m_elements["Oxygen"], 0.231);
    air->add(m_elements["Argon"], 0.014);
    air->lock();
    m_materials["Air"] = air;

    // Concrete (density 2.3 g/cm³): O 52%, Si 33%, Ca 15%
    GeoMaterial* concrete =
        new GeoMaterial("Concrete", 2.3 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    concrete->add(m_elements["Oxygen"], 0.52);
    concrete->add(m_elements["Silicon"], 0.33);
    concrete->add(m_elements["Calcium"], 0.15);
    concrete->lock();
    m_materials["Concrete"] = concrete;

    // Vacuum (density 1.205e-6 g/cm³): N 75.5%, O 23.1%, Ar 1.4%
    GeoMaterial* vacuum =
        new GeoMaterial("Vacuum", 1.205e-6 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    vacuum->add(m_elements["Nitrogen"], 0.755);
    vacuum->add(m_elements["Oxygen"], 0.231);
    vacuum->add(m_elements["Argon"], 0.014);
    vacuum->lock();
    m_materials["Vacuum"] = vacuum;

    // PressurisedHe90 (density 0.00212 g/cm³): pure He
    GeoMaterial* pressurisedHe90 = new GeoMaterial(
        "PressurisedHe90", 0.00212 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    pressurisedHe90->add(m_elements["Helium"], 1.0);
    pressurisedHe90->lock();
    m_materials["PressurisedHe90"] = pressurisedHe90;

    // Tungsten (density 19.3 g/cm³): pure W
    GeoMaterial* tungsten =
        new GeoMaterial("Tungsten", 19.3 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    tungsten->add(m_elements["Tungsten"], 1.0);
    tungsten->lock();
    m_materials["Tungsten"] = tungsten;

    // Tantalum (density 16.65 g/cm³): pure Ta
    GeoMaterial* tantalum =
        new GeoMaterial("Tantalum", 16.65 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    tantalum->add(m_elements["Tantalum"], 1.0);
    tantalum->lock();
    m_materials["Tantalum"] = tantalum;

    // Inconel718 (density 8.19 g/cm³): Ni 55%, Cr 20%, Fe 16%, Nb 5%, Mo 3%, Ti 1%
    GeoMaterial* inconel718 =
        new GeoMaterial("Inconel718", 8.19 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    inconel718->add(m_elements["Nickel"], 0.55);
    inconel718->add(m_elements["Chromium"], 0.20);
    inconel718->add(m_elements["Iron"], 0.16);
    inconel718->add(m_elements["Niobium"], 0.05);
    inconel718->add(m_elements["Molybdenum"], 0.03);
    inconel718->add(m_elements["Titanium"], 0.01);
    inconel718->lock();
    m_materials["Inconel718"] = inconel718;

    // Steel316L (density 7.99 g/cm³): Fe 65.3%, Cr 15.7%, Ni 12.5%, Mo 4.2%, Mn 1.9%, Si 0.4%
    GeoMaterial* steel316L =
        new GeoMaterial("Steel316L", 7.99 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    steel316L->add(m_elements["Iron"], 0.653);
    steel316L->add(m_elements["Chromium"], 0.157);
    steel316L->add(m_elements["Nickel"], 0.125);
    steel316L->add(m_elements["Molybdenum"], 0.042);
    steel316L->add(m_elements["Manganese"], 0.019);
    steel316L->add(m_elements["Silicon"], 0.004);
    steel316L->lock();
    m_materials["Steel316L"] = steel316L;

    // Copper (density 8.96 g/cm³): pure Cu
    GeoMaterial* copper =
        new GeoMaterial("Copper", 8.96 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    copper->add(m_elements["Copper"], 1.0);
    copper->lock();
    m_materials["Copper"] = copper;

    // Iron (density 7.87 g/cm³): pure Fe
    GeoMaterial* iron =
        new GeoMaterial("Iron", 7.87 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    iron->add(m_elements["Iron"], 1.0);
    iron->lock();
    m_materials["Iron"] = iron;

    // Aluminium (density 2.70 g/cm³): pure Al
    GeoMaterial* aluminium =
        new GeoMaterial("Aluminium", 2.70 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    aluminium->add(m_elements["Aluminium"], 1.0);
    aluminium->lock();
    m_materials["Aluminium"] = aluminium;

    // Scintillator / polyvinyltoluene (density 1.023 g/cm³): C 91.5%, H 8.5%
    GeoMaterial* scintillator =
        new GeoMaterial("Scintillator", 1.023 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    scintillator->add(m_elements["Carbon"], 0.915);
    scintillator->add(m_elements["Hydrogen"], 0.085);
    scintillator->lock();
    m_materials["Scintillator"] = scintillator;

    // Mylar / PET (density 1.39 g/cm³): C10H8O4 → C 62.50%, H 4.20%, O 33.30%
    GeoMaterial* mylar =
        new GeoMaterial("Mylar", 1.39 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    mylar->add(m_elements["Carbon"],   0.6250);
    mylar->add(m_elements["Hydrogen"], 0.0420);
    mylar->add(m_elements["Oxygen"],   0.3330);
    mylar->lock();
    m_materials["Mylar"] = mylar;

    // ArCO2 70/30 drift gas (density 1.842e-3 g/cm³): Ar 67.93%, C 8.75%, O 23.32%
    GeoMaterial* arco2 =
        new GeoMaterial("ArCO2", 1.842e-3 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    arco2->add(m_elements["Argon"],    0.6793);
    arco2->add(m_elements["Carbon"],   0.0875);
    arco2->add(m_elements["Oxygen"],   0.2332);
    arco2->lock();
    m_materials["ArCO2"] = arco2;

    // Polystyrene (density 1.06 g/cm³): C8H8 → C 92.26%, H 7.74%
    GeoMaterial* polystyrene =
        new GeoMaterial("Polystyrene", 1.06 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    polystyrene->add(m_elements["Carbon"],   0.9226);
    polystyrene->add(m_elements["Hydrogen"], 0.0774);
    polystyrene->lock();
    m_materials["Polystyrene"] = polystyrene;

}

}  // namespace SHiPGeometry
