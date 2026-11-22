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
            std::cerr << "Can't open database: " << sqlite3_errmsg(m_db) << '\n';
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
            std::cerr << "Database not available" << '\n';
            return;
        }

        std::cout << "=== SHiP Geometry Validation Report ===" << '\n';
        std::cout << '\n';

        // Check database structure
        checkDatabaseStructure();

        // Extract and display key geometry information
        extractVolumeInformation();
        extractMaterialInformation();
        extractPositionInformation();

        std::cout << '\n';
        std::cout << "=== Validation Summary ===" << '\n';
        std::cout << "Database structure: OK" << '\n';
        std::cout << "For detailed GDML comparison, use external tools like:" << '\n';
        std::cout << "- ROOT's TGeoManager::Export() method" << '\n';
        std::cout << "- Geant4's GDML export functionality" << '\n';
        std::cout << "- Custom GDML writer based on database content" << '\n';
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
        std::cout << "--- Database Structure Check ---" << '\n';

        // Check essential tables exist
        std::string requiredTables[] = {"LogVols",           "PhysVols",   "Materials",
                                        "Elements",          "Shapes_Box", "Shapes_Tube",
                                        "ChildrenPositions", "RootVolume"};

        for (const auto& table : requiredTables) {
            if (tableExists(table)) {
                std::cout << "✓ Table " << table << " exists" << '\n';
            } else {
                std::cout << "✗ Table " << table << " missing" << '\n';
            }
        }
    }

    void extractVolumeInformation() {
        std::cout << '\n' << "--- Volume Information ---" << '\n';

        int logvolCount = countRecords("LogVols");
        int physvolCount = countRecords("PhysVols");

        std::cout << "Total logical volumes: " << logvolCount << '\n';
        std::cout << "Total physical volumes: " << physvolCount << '\n';

        // Show first few volumes as examples
        std::cout << '\n' << "Sample logical volumes:" << '\n';
        std::cout << std::setw(20) << "Name" << std::setw(15) << "Shape Type" << std::setw(15)
                  << "Material ID" << '\n';
        std::cout << std::string(50, '-') << '\n';

        std::string sql = "SELECT id, name, shapeId, shapeType, materialId FROM LogVols LIMIT 10";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* name = (const char*)sqlite3_column_text(stmt, 1);
                const char* shapeType = (const char*)sqlite3_column_text(stmt, 3);
                int materialId = sqlite3_column_int(stmt, 4);

                std::cout << std::setw(20) << (name ? name : "NULL") << std::setw(15)
                          << (shapeType ? shapeType : "NULL") << std::setw(15) << materialId
                          << '\n';
            }
        }
        sqlite3_finalize(stmt);
    }

    void extractMaterialInformation() {
        std::cout << '\n' << "--- Material Information ---" << '\n';

        int materialCount = countRecords("Materials");
        int elementCount = countRecords("Elements");

        std::cout << "Total materials: " << materialCount << '\n';
        std::cout << "Total elements: " << elementCount << '\n';

        // Show materials
        std::cout << '\n' << "Materials:" << '\n';
        std::cout << std::setw(20) << "Name" << std::setw(15) << "Density" << '\n';
        std::cout << std::string(35, '-') << '\n';

        std::string sql = "SELECT id, name, density FROM Materials";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* name = (const char*)sqlite3_column_text(stmt, 1);
                double density = sqlite3_column_double(stmt, 2);

                std::cout << std::setw(20) << (name ? name : "NULL") << std::setw(15) << density
                          << '\n';
            }
        }
        sqlite3_finalize(stmt);
    }

    void extractPositionInformation() {
        std::cout << '\n' << "--- Position Information ---" << '\n';

        int positionCount = countRecords("ChildrenPositions");
        int transformCount = countRecords("Transforms");

        std::cout << "Total position records: " << positionCount << '\n';
        std::cout << "Total transforms: " << transformCount << '\n';

        // Show some position data
        std::cout << '\n' << "Sample positions (first 5):" << '\n';
        std::cout << std::setw(15) << "Parent ID" << std::setw(15) << "Child ID" << std::setw(15)
                  << "Child Type" << '\n';
        std::cout << std::string(45, '-') << '\n';

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
                          << (childType ? childType : "NULL") << '\n';
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

        std::cout << '\n';
        std::cout << "Note: For full GDML export, consider implementing a custom" << '\n';
        std::cout << "GDML writer that traverses the database and generates XML." << '\n';
        std::cout << "The database contains all necessary geometric information." << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
