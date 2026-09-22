// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include "SHiPGeometry/SHiPMaterials.h"

#include "SHiPGeometry/Units.h"

#include <GeoModelKernel/GeoElement.h>
#include <GeoModelKernel/GeoMaterial.h>

#include <stdexcept>

namespace SHiPGeometry {

using units::gm;

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
    m_elements["Hydrogen"] = new GeoElement("Hydrogen", "H", 1, gm(1.008 * units::g / units::mol));
    m_elements["Helium"] = new GeoElement("Helium", "He", 2, gm(4.003 * units::g / units::mol));
    m_elements["Carbon"] = new GeoElement("Carbon", "C", 6, gm(12.011 * units::g / units::mol));
    m_elements["Nitrogen"] = new GeoElement("Nitrogen", "N", 7, gm(14.007 * units::g / units::mol));
    m_elements["Oxygen"] = new GeoElement("Oxygen", "O", 8, gm(15.999 * units::g / units::mol));
    m_elements["Silicon"] = new GeoElement("Silicon", "Si", 14, gm(28.085 * units::g / units::mol));
    m_elements["Argon"] = new GeoElement("Argon", "Ar", 18, gm(39.948 * units::g / units::mol));
    m_elements["Calcium"] = new GeoElement("Calcium", "Ca", 20, gm(40.078 * units::g / units::mol));
    m_elements["Titanium"] =
        new GeoElement("Titanium", "Ti", 22, gm(47.867 * units::g / units::mol));
    m_elements["Chromium"] =
        new GeoElement("Chromium", "Cr", 24, gm(51.996 * units::g / units::mol));
    m_elements["Manganese"] =
        new GeoElement("Manganese", "Mn", 25, gm(54.938 * units::g / units::mol));
    m_elements["Iron"] = new GeoElement("Iron", "Fe", 26, gm(55.845 * units::g / units::mol));
    m_elements["Nickel"] = new GeoElement("Nickel", "Ni", 28, gm(58.693 * units::g / units::mol));
    m_elements["Copper"] = new GeoElement("Copper", "Cu", 29, gm(63.546 * units::g / units::mol));
    m_elements["Niobium"] = new GeoElement("Niobium", "Nb", 41, gm(92.906 * units::g / units::mol));
    m_elements["Molybdenum"] =
        new GeoElement("Molybdenum", "Mo", 42, gm(95.95 * units::g / units::mol));
    m_elements["Tantalum"] =
        new GeoElement("Tantalum", "Ta", 73, gm(180.948 * units::g / units::mol));
    m_elements["Tungsten"] =
        new GeoElement("Tungsten", "W", 74, gm(183.84 * units::g / units::mol));

    // Aluminium
    m_elements["Aluminium"] =
        new GeoElement("Aluminium", "Al", 13.0, gm(26.982 * units::g / units::mol));

    // Lead
    m_elements["Lead"] = new GeoElement("Lead", "Pb", 82.0, gm(207.2 * units::g / units::mol));
}

void SHiPMaterials::createMaterials() {
    // Air (density 1.29e-3 g/cm³): N 75.5%, O 23.1%, Ar 1.4%
    GeoMaterial* air = new GeoMaterial("Air", gm(1.29e-3 * units::g / units::cm3));
    air->add(m_elements["Nitrogen"], 0.755);
    air->add(m_elements["Oxygen"], 0.231);
    air->add(m_elements["Argon"], 0.014);
    air->lock();
    m_materials["Air"] = air;

    // Concrete (density 2.3 g/cm³): O 52%, Si 33%, Ca 15%
    GeoMaterial* concrete = new GeoMaterial("Concrete", gm(2.3 * units::g / units::cm3));
    concrete->add(m_elements["Oxygen"], 0.52);
    concrete->add(m_elements["Silicon"], 0.33);
    concrete->add(m_elements["Calcium"], 0.15);
    concrete->lock();
    m_materials["Concrete"] = concrete;

    // Vacuum (density 1.205e-6 g/cm³): N 75.5%, O 23.1%, Ar 1.4%
    GeoMaterial* vacuum = new GeoMaterial("Vacuum", gm(1.205e-6 * units::g / units::cm3));
    vacuum->add(m_elements["Nitrogen"], 0.755);
    vacuum->add(m_elements["Oxygen"], 0.231);
    vacuum->add(m_elements["Argon"], 0.014);
    vacuum->lock();
    m_materials["Vacuum"] = vacuum;

    // PressurisedHe90 (density 0.00212 g/cm³): pure He
    GeoMaterial* pressurisedHe90 =
        new GeoMaterial("PressurisedHe90", gm(0.00212 * units::g / units::cm3));
    pressurisedHe90->add(m_elements["Helium"], 1.0);
    pressurisedHe90->lock();
    m_materials["PressurisedHe90"] = pressurisedHe90;

    // Tungsten (density 19.3 g/cm³): pure W
    GeoMaterial* tungsten = new GeoMaterial("Tungsten", gm(19.3 * units::g / units::cm3));
    tungsten->add(m_elements["Tungsten"], 1.0);
    tungsten->lock();
    m_materials["Tungsten"] = tungsten;

    // Silicon (density 2.33 g/cm³): pure Si (neutrino-target tracking planes)
    GeoMaterial* silicon = new GeoMaterial("Silicon", gm(2.33 * units::g / units::cm3));
    silicon->add(m_elements["Silicon"], 1.0);
    silicon->lock();
    m_materials["Silicon"] = silicon;

    // Tantalum (density 16.65 g/cm³): pure Ta
    GeoMaterial* tantalum = new GeoMaterial("Tantalum", gm(16.65 * units::g / units::cm3));
    tantalum->add(m_elements["Tantalum"], 1.0);
    tantalum->lock();
    m_materials["Tantalum"] = tantalum;

    // Inconel718 (density 8.19 g/cm³): Ni 55%, Cr 20%, Fe 16%, Nb 5%, Mo 3%, Ti 1%
    GeoMaterial* inconel718 = new GeoMaterial("Inconel718", gm(8.19 * units::g / units::cm3));
    inconel718->add(m_elements["Nickel"], 0.55);
    inconel718->add(m_elements["Chromium"], 0.20);
    inconel718->add(m_elements["Iron"], 0.16);
    inconel718->add(m_elements["Niobium"], 0.05);
    inconel718->add(m_elements["Molybdenum"], 0.03);
    inconel718->add(m_elements["Titanium"], 0.01);
    inconel718->lock();
    m_materials["Inconel718"] = inconel718;

    // Steel316L (density 7.99 g/cm³): Fe 65.3%, Cr 15.7%, Ni 12.5%, Mo 4.2%, Mn 1.9%, Si 0.4%
    GeoMaterial* steel316L = new GeoMaterial("Steel316L", gm(7.99 * units::g / units::cm3));
    steel316L->add(m_elements["Iron"], 0.653);
    steel316L->add(m_elements["Chromium"], 0.157);
    steel316L->add(m_elements["Nickel"], 0.125);
    steel316L->add(m_elements["Molybdenum"], 0.042);
    steel316L->add(m_elements["Manganese"], 0.019);
    steel316L->add(m_elements["Silicon"], 0.004);
    steel316L->lock();
    m_materials["Steel316L"] = steel316L;

    // Copper (density 8.96 g/cm³): pure Cu
    GeoMaterial* copper = new GeoMaterial("Copper", gm(8.96 * units::g / units::cm3));
    copper->add(m_elements["Copper"], 1.0);
    copper->lock();
    m_materials["Copper"] = copper;

    // Iron (density 7.87 g/cm³): pure Fe
    GeoMaterial* iron = new GeoMaterial("Iron", gm(7.87 * units::g / units::cm3));
    iron->add(m_elements["Iron"], 1.0);
    iron->lock();
    m_materials["Iron"] = iron;

    // Aluminium (density 2.70 g/cm³): pure Al
    GeoMaterial* aluminium = new GeoMaterial("Aluminium", gm(2.70 * units::g / units::cm3));
    aluminium->add(m_elements["Aluminium"], 1.0);
    aluminium->lock();
    m_materials["Aluminium"] = aluminium;

    // LAB — linear alkylbenzene liquid scintillator (density 0.867 g/cm³):
    // C 87.41%, H 12.59% by mass. Used in the Surround Background Tagger cells.
    GeoMaterial* lab = new GeoMaterial("LAB", gm(0.867 * units::g / units::cm3));
    lab->add(m_elements["Carbon"], 0.8741);
    lab->add(m_elements["Hydrogen"], 0.1259);
    lab->lock();
    m_materials["LAB"] = lab;

    // TimDetScint — polyvinyltoluene scintillator for the Timing Detector bars
    // (C8H8, density 1.023 g/cm³; mass fractions per timing_detector.gmx).
    GeoMaterial* timDetScint = new GeoMaterial("TimDetScint", gm(1.023 * units::g / units::cm3));
    timDetScint->add(m_elements["Carbon"], 0.9226);
    timDetScint->add(m_elements["Hydrogen"], 0.0774);
    timDetScint->lock();
    m_materials["TimDetScint"] = timDetScint;

    // Lead (density 11.34 g/cm³): pure Pb
    GeoMaterial* lead = new GeoMaterial("Lead", gm(11.34 * units::g / units::cm3));
    lead->add(m_elements["Lead"], 1.0);
    lead->lock();
    m_materials["Lead"] = lead;

    // PVT / polyvinyltoluene (density 1.032 g/cm³): C9H10
    // MW = 9*12.011 + 10*1.008 = 108.099 + 10.080 = 118.179 g/mol
    {
        const double awC = 12.011;
        const double awH = 1.008;
        const double mw = 9.0 * awC + 10.0 * awH;
        GeoMaterial* pvt = new GeoMaterial("PVT", gm(1.032 * units::g / units::cm3));
        pvt->add(m_elements["Carbon"], 9.0 * awC / mw);
        pvt->add(m_elements["Hydrogen"], 10.0 * awH / mw);
        pvt->lock();
        m_materials["PVT"] = pvt;
    }

    // Polystyrene (density 1.05 g/cm³): C8H8
    // MW = 8*12.011 + 8*1.008 = 96.088 + 8.064 = 104.152 g/mol
    {
        const double awC = 12.011;
        const double awH = 1.008;
        const double mw = 8.0 * awC + 8.0 * awH;
        GeoMaterial* polystyrene = new GeoMaterial("Polystyrene", gm(1.05 * units::g / units::cm3));
        polystyrene->add(m_elements["Carbon"], 8.0 * awC / mw);
        polystyrene->add(m_elements["Hydrogen"], 8.0 * awH / mw);
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
        GeoMaterial* mylar = new GeoMaterial("Mylar", gm(1.39 * units::g / units::cm3));
        mylar->add(m_elements["Carbon"], 10.0 * awC / mw);
        mylar->add(m_elements["Hydrogen"], 8.0 * awH / mw);
        mylar->add(m_elements["Oxygen"], 4.0 * awO / mw);
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
        GeoMaterial* arco2 = new GeoMaterial("ArCO2_70_30", gm(1.56e-3 * units::g / units::cm3));
        arco2->add(m_elements["Argon"], fracAr);
        arco2->add(m_elements["Carbon"], fracCO2 * 12.011 / mwCO2);
        arco2->add(m_elements["Oxygen"], fracCO2 * 2.0 * 15.999 / mwCO2);
        arco2->lock();
        m_materials["ArCO2_70_30"] = arco2;
    }
}

}  // namespace SHiPGeometry
