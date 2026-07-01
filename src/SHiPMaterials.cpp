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
    m_elements["Hydrogen_E"] = new GeoElement(
        "Hydrogen_E", "H", 1, 1.008 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Helium_E"] = new GeoElement(
        "Helium_E", "He", 2, 4.003 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Carbon_E"] = new GeoElement(
        "Carbon_E", "C", 6, 12.011 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Nitrogen_E"] = new GeoElement(
        "Nitrogen_E", "N", 7, 14.007 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Oxygen_E"] = new GeoElement(
        "Oxygen_E", "O", 8, 15.999 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Silicon_E"] = new GeoElement(
        "Silicon_E", "Si", 14, 28.085 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Argon_E"] = new GeoElement(
        "Argon_E", "Ar", 18, 39.948 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Calcium_E"] = new GeoElement(
        "Calcium_E", "Ca", 20, 40.078 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Titanium_E"] = new GeoElement(
        "Titanium_E", "Ti", 22, 47.867 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Chromium_E"] = new GeoElement(
        "Chromium_E", "Cr", 24, 51.996 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Manganese_E"] = new GeoElement(
        "Manganese_E", "Mn", 25, 54.938 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Iron_E"] = new GeoElement(
        "Iron_E", "Fe", 26, 55.845 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Nickel_E"] = new GeoElement(
        "Nickel_E", "Ni", 28, 58.693 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Copper_E"] = new GeoElement(
        "Copper_E", "Cu", 29, 63.546 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Niobium_E"] = new GeoElement(
        "Niobium_E", "Nb", 41, 92.906 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Molybdenum_E"] = new GeoElement(
        "Molybdenum_E", "Mo", 42, 95.95 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Tantalum_E"] = new GeoElement(
        "Tantalum_E", "Ta", 73, 180.948 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
    m_elements["Tungsten_E"] = new GeoElement(
        "Tungsten_E", "W", 74, 183.84 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);

    // Aluminium
    m_elements["Aluminium_E"] = new GeoElement(
        "Aluminium_E", "Al", 13.0, 26.982 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);

    // Lead
    m_elements["Lead_E"] = new GeoElement("Lead_E", "Pb", 82.0,
                                        207.2 * GeoModelKernelUnits::g / GeoModelKernelUnits::mole);
}

void SHiPMaterials::createMaterials() {
    // Air (density 1.29e-3 g/cm³): N 75.5%, O 23.1%, Ar 1.4%
    GeoMaterial* air =
        new GeoMaterial("Air", 1.29e-3 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    air->add(m_elements["Nitrogen_E"], 0.755);
    air->add(m_elements["Oxygen_E"], 0.231);
    air->add(m_elements["Argon_E"], 0.014);
    air->lock();
    m_materials["Air"] = air;

    // Concrete (density 2.3 g/cm³): O 52%, Si 33%, Ca 15%
    GeoMaterial* concrete =
        new GeoMaterial("Concrete", 2.3 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    concrete->add(m_elements["Oxygen_E"], 0.52);
    concrete->add(m_elements["Silicon_E"], 0.33);
    concrete->add(m_elements["Calcium_E"], 0.15);
    concrete->lock();
    m_materials["Concrete"] = concrete;

    // Vacuum (density 1.205e-6 g/cm³): N 75.5%, O 23.1%, Ar 1.4%
    GeoMaterial* vacuum =
        new GeoMaterial("Vacuum", 1.205e-6 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    vacuum->add(m_elements["Nitrogen_E"], 0.755);
    vacuum->add(m_elements["Oxygen_E"], 0.231);
    vacuum->add(m_elements["Argon_E"], 0.014);
    vacuum->lock();
    m_materials["Vacuum"] = vacuum;

    // PressurisedHe90 (density 0.00212 g/cm³): pure He
    GeoMaterial* pressurisedHe90 = new GeoMaterial(
        "PressurisedHe90", 0.00212 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    pressurisedHe90->add(m_elements["Helium_E"], 1.0);
    pressurisedHe90->lock();
    m_materials["PressurisedHe90"] = pressurisedHe90;

    // Tungsten (density 19.3 g/cm³): pure W
    GeoMaterial* tungsten =
        new GeoMaterial("Tungsten", 19.3 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    tungsten->add(m_elements["Tungsten_E"], 1.0);
    tungsten->lock();
    m_materials["Tungsten"] = tungsten;

    // Silicon (density 2.33 g/cm³): pure Si (neutrino-target tracking planes)
    GeoMaterial* silicon =
        new GeoMaterial("Silicon", 2.33 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    silicon->add(m_elements["Silicon_E"], 1.0);
    silicon->lock();
    m_materials["Silicon"] = silicon;

    // Tantalum (density 16.65 g/cm³): pure Ta
    GeoMaterial* tantalum =
        new GeoMaterial("Tantalum", 16.65 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    tantalum->add(m_elements["Tantalum_E"], 1.0);
    tantalum->lock();
    m_materials["Tantalum"] = tantalum;

    // Inconel718 (density 8.19 g/cm³): Ni 55%, Cr 20%, Fe 16%, Nb 5%, Mo 3%, Ti 1%
    GeoMaterial* inconel718 =
        new GeoMaterial("Inconel718", 8.19 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    inconel718->add(m_elements["Nickel_E"], 0.55);
    inconel718->add(m_elements["Chromium_E"], 0.20);
    inconel718->add(m_elements["Iron_E"], 0.16);
    inconel718->add(m_elements["Niobium_E"], 0.05);
    inconel718->add(m_elements["Molybdenum_E"], 0.03);
    inconel718->add(m_elements["Titanium_E"], 0.01);
    inconel718->lock();
    m_materials["Inconel718"] = inconel718;

    // Steel316L (density 7.99 g/cm³): Fe 65.3%, Cr 15.7%, Ni 12.5%, Mo 4.2%, Mn 1.9%, Si 0.4%
    GeoMaterial* steel316L =
        new GeoMaterial("Steel316L", 7.99 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    steel316L->add(m_elements["Iron_E"], 0.653);
    steel316L->add(m_elements["Chromium_E"], 0.157);
    steel316L->add(m_elements["Nickel_E"], 0.125);
    steel316L->add(m_elements["Molybdenum_E"], 0.042);
    steel316L->add(m_elements["Manganese_E"], 0.019);
    steel316L->add(m_elements["Silicon_E"], 0.004);
    steel316L->lock();
    m_materials["Steel316L"] = steel316L;

    // Copper (density 8.96 g/cm³): pure Cu
    GeoMaterial* copper =
        new GeoMaterial("Copper", 8.96 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    copper->add(m_elements["Copper_E"], 1.0);
    copper->lock();
    m_materials["Copper"] = copper;

    // Iron (density 7.87 g/cm³): pure Fe
    GeoMaterial* iron =
        new GeoMaterial("Iron", 7.87 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    iron->add(m_elements["Iron_E"], 1.0);
    iron->lock();
    m_materials["Iron"] = iron;

    // Aluminium (density 2.70 g/cm³): pure Al
    GeoMaterial* aluminium =
        new GeoMaterial("Aluminium", 2.70 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    aluminium->add(m_elements["Aluminium_E"], 1.0);
    aluminium->lock();
    m_materials["Aluminium"] = aluminium;

    // LAB — linear alkylbenzene liquid scintillator (density 0.867 g/cm³):
    // C 87.41%, H 12.59% by mass. Used in the Surround Background Tagger cells.
    GeoMaterial* lab =
        new GeoMaterial("LAB", 0.867 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    lab->add(m_elements["Carbon_E"], 0.8741);
    lab->add(m_elements["Hydrogen_E"], 0.1259);
    lab->lock();
    m_materials["LAB"] = lab;

    // TimDetScint — polyvinyltoluene scintillator for the Timing Detector bars
    // (C8H8, density 1.023 g/cm³; mass fractions per timing_detector.gmx).
    GeoMaterial* timDetScint =
        new GeoMaterial("TimDetScint", 1.023 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    timDetScint->add(m_elements["Carbon_E"], 0.9226);
    timDetScint->add(m_elements["Hydrogen_E"], 0.0774);
    timDetScint->lock();
    m_materials["TimDetScint"] = timDetScint;

    // Lead (density 11.34 g/cm³): pure Pb
    GeoMaterial* lead =
        new GeoMaterial("Lead", 11.34 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
    lead->add(m_elements["Lead_E"], 1.0);
    lead->lock();
    m_materials["Lead"] = lead;

    // PVT / polyvinyltoluene (density 1.032 g/cm³): C9H10
    // MW = 9*12.011 + 10*1.008 = 108.099 + 10.080 = 118.179 g/mol
    {
        const double awC = 12.011;
        const double awH = 1.008;
        const double mw = 9.0 * awC + 10.0 * awH;
        GeoMaterial* pvt =
            new GeoMaterial("PVT", 1.032 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
        pvt->add(m_elements["Carbon_E"], 9.0 * awC / mw);
        pvt->add(m_elements["Hydrogen_E"], 10.0 * awH / mw);
        pvt->lock();
        m_materials["PVT"] = pvt;
    }

    // Polystyrene (density 1.05 g/cm³): C8H8
    // MW = 8*12.011 + 8*1.008 = 96.088 + 8.064 = 104.152 g/mol
    {
        const double awC = 12.011;
        const double awH = 1.008;
        const double mw = 8.0 * awC + 8.0 * awH;
        GeoMaterial* polystyrene = new GeoMaterial(
            "Polystyrene", 1.05 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
        polystyrene->add(m_elements["Carbon_E"], 8.0 * awC / mw);
        polystyrene->add(m_elements["Hydrogen_E"], 8.0 * awH / mw);
        polystyrene->lock();
        m_materials["Polystyrene"] = polystyrene;
    }

    // Mylar / PET (density 1.39 g/cm³): C10H8O4 — straw tube walls
    // MW = 10*12.011 + 8*1.008 + 4*15.999 = 192.166 g/mol
    {
        const double awC = 12.011;
        const double awH = 1.008;
        const double awO = 15.999;
        const double mw = 10.0 * awC + 8.0 * awH + 4.0 * awO;
        GeoMaterial* mylar =
            new GeoMaterial("Mylar", 1.39 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
        mylar->add(m_elements["Carbon_E"], 10.0 * awC / mw);
        mylar->add(m_elements["Hydrogen_E"], 8.0 * awH / mw);
        mylar->add(m_elements["Oxygen_E"], 4.0 * awO / mw);
        mylar->lock();
        m_materials["Mylar"] = mylar;
    }

    // ArCO2_70_30 (density 1.56e-3 g/cm³): 70% Ar + 30% CO2 by mass —
    // straw tube gas fill. The CO2 mass fraction is split into its C and O
    // constituents (C: 12.011/44.009, O: 2*15.999/44.009 of the CO2 mass).
    {
        const double mwCO2 = 44.009;
        const double fracAr = 0.70;
        const double fracCO2 = 0.30;
        GeoMaterial* arco2 = new GeoMaterial(
            "ArCO2_70_30", 1.56e-3 * GeoModelKernelUnits::g / GeoModelKernelUnits::cm3);
        arco2->add(m_elements["Argon_E"], fracAr);
        arco2->add(m_elements["Carbon_E"], fracCO2 * 12.011 / mwCO2);
        arco2->add(m_elements["Oxygen_E"], fracCO2 * 2.0 * 15.999 / mwCO2);
        arco2->lock();
        m_materials["ArCO2_70_30"] = arco2;
    }
}

}  // namespace SHiPGeometry
