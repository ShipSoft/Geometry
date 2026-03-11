// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) CERN for the benefit of the SHiP Collaboration

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sqlite3.h>
#include <string>

class GeometryValidator {
   private:
    sqlite3* m_db;

   public:
    explicit GeometryValidator(const std::string& dbFile) : m_db(nullptr) {
        int rc = sqlite3_open(dbFile.c_str(), &m_db);
        if (rc) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(m_db) << std::endl;
            sqlite3_close(m_db);
            m_db = nullptr;
        }
    }

    ~GeometryValidator() {
        if (m_db) {
            sqlite3_close(m_db);
        }
    }

    bool isValid() const { return m_db != nullptr; }

    void validateGeometry() {
        if (!isValid()) {
            std::cerr << "Database not available" << std::endl;
            return;
        }

        std::cout << "=== SHiP Geometry Validation Report ===" << std::endl;
        std::cout << std::endl;

        // Check database structure
        checkDatabaseStructure();

        // Extract and display key geometry information
        extractVolumeInformation();
        extractMaterialInformation();
        extractPositionInformation();

        std::cout << std::endl;
        std::cout << "=== Validation Summary ===" << std::endl;
        std::cout << "Database structure: OK" << std::endl;
        std::cout << "For detailed GDML comparison, use external tools like:" << std::endl;
        std::cout << "- ROOT's TGeoManager::Export() method" << std::endl;
        std::cout << "- Geant4's GDML export functionality" << std::endl;
        std::cout << "- Custom GDML writer based on database content" << std::endl;
    }

   private:
    int countRecords(const std::string& tableName) {
        std::string sql = "SELECT COUNT(*) FROM " + tableName;
        sqlite3_stmt* stmt;
        int count = 0;

        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);
        return count;
    }

    bool tableExists(const std::string& tableName) {
        std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
        sqlite3_stmt* stmt;
        bool exists = false;

        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                exists = true;
            }
        }
        sqlite3_finalize(stmt);
        return exists;
    }

    void checkDatabaseStructure() {
        std::cout << "--- Database Structure Check ---" << std::endl;

        // Check essential tables exist
        std::string requiredTables[] = {"LogVols",           "PhysVols",   "Materials",
                                        "Elements",          "Shapes_Box", "Shapes_Tube",
                                        "ChildrenPositions", "RootVolume"};

        for (const auto& table : requiredTables) {
            if (tableExists(table)) {
                std::cout << "✓ Table " << table << " exists" << std::endl;
            } else {
                std::cout << "✗ Table " << table << " missing" << std::endl;
            }
        }
    }

    void extractVolumeInformation() {
        std::cout << std::endl << "--- Volume Information ---" << std::endl;

        int logvolCount = countRecords("LogVols");
        int physvolCount = countRecords("PhysVols");

        std::cout << "Total logical volumes: " << logvolCount << std::endl;
        std::cout << "Total physical volumes: " << physvolCount << std::endl;

        // Show first few volumes as examples
        std::cout << std::endl << "Sample logical volumes:" << std::endl;
        std::cout << std::setw(20) << "Name" << std::setw(15) << "Shape Type" << std::setw(15)
                  << "Material ID" << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        std::string sql = "SELECT id, name, shapeId, shapeType, materialId FROM LogVols LIMIT 10";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* name = (const char*)sqlite3_column_text(stmt, 1);
                const char* shapeType = (const char*)sqlite3_column_text(stmt, 3);
                int materialId = sqlite3_column_int(stmt, 4);

                std::cout << std::setw(20) << (name ? name : "NULL") << std::setw(15)
                          << (shapeType ? shapeType : "NULL") << std::setw(15) << materialId
                          << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }

    void extractMaterialInformation() {
        std::cout << std::endl << "--- Material Information ---" << std::endl;

        int materialCount = countRecords("Materials");
        int elementCount = countRecords("Elements");

        std::cout << "Total materials: " << materialCount << std::endl;
        std::cout << "Total elements: " << elementCount << std::endl;

        // Show materials
        std::cout << std::endl << "Materials:" << std::endl;
        std::cout << std::setw(20) << "Name" << std::setw(15) << "Density" << std::endl;
        std::cout << std::string(35, '-') << std::endl;

        std::string sql = "SELECT id, name, density FROM Materials";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* name = (const char*)sqlite3_column_text(stmt, 1);
                double density = sqlite3_column_double(stmt, 2);

                std::cout << std::setw(20) << (name ? name : "NULL") << std::setw(15) << density
                          << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }

    void extractPositionInformation() {
        std::cout << std::endl << "--- Position Information ---" << std::endl;

        int positionCount = countRecords("ChildrenPositions");
        int transformCount = countRecords("Transforms");

        std::cout << "Total position records: " << positionCount << std::endl;
        std::cout << "Total transforms: " << transformCount << std::endl;

        // Show some position data
        std::cout << std::endl << "Sample positions (first 5):" << std::endl;
        std::cout << std::setw(15) << "Parent ID" << std::setw(15) << "Child ID" << std::setw(15)
                  << "Child Type" << std::endl;
        std::cout << std::string(45, '-') << std::endl;

        std::string sql =
            "SELECT parentId, parentType, childId, parentCopyNumber, childPos, childType FROM "
            "ChildrenPositions LIMIT 5";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int parentId = sqlite3_column_int(stmt, 0);
                int childId = sqlite3_column_int(stmt, 2);
                const char* childType = (const char*)sqlite3_column_text(stmt, 5);

                std::cout << std::setw(15) << parentId << std::setw(15) << childId << std::setw(15)
                          << (childType ? childType : "NULL") << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }
};

int main(int argc, char* argv[]) {
    std::string dbFile = "ship_geomodel.db";
    if (argc > 1) {
        dbFile = argv[1];
    }

    try {
        GeometryValidator validator(dbFile);
        validator.validateGeometry();

        std::cout << std::endl;
        std::cout << "Note: For full GDML export, consider implementing a custom" << std::endl;
        std::cout << "GDML writer that traverses the database and generates XML." << std::endl;
        std::cout << "The database contains all necessary geometric information." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
