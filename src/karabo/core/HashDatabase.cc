/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 16, 2012, 4:43 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/io/Input.hh>
#include "HashDatabase.hh"
#include "karabo/io/FileTools.hh"

namespace karabo {
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;

        karabo::util::Hash HashDatabase::m_database;
        boost::mutex HashDatabase::m_databaseMutex;


        bool HashDatabase::readDatabase() {

            boost::mutex::scoped_lock lock(m_databaseMutex);

            if (boost::filesystem::exists(KARABO_DB_FILE)) {
                karabo::io::loadFromFile<Hash>(m_database, KARABO_DB_FILE);
                return true;
            }
            return false;
        }


        void HashDatabase::setupDatabase() {

            boost::mutex::scoped_lock lock(m_databaseMutex);

            Hash tables;

            // "Model" -> Hash (table)
            //     <modId> -> Hash (rows)
            //         "modelFile" -> string
            tables.set("Model", vector<Hash > ());

            // "Location" -> Hash (table)
            //     <locId> -> Hash  (rows)
            //         "xFrac" -> float, "yFrac" -> float, "zFrac" -> float, "modId" -> index (fields)
            tables.set("Location", vector<Hash > ());

            // "Node" -> Hash (table)
            //     <nodId> -> Hash (rows)
            //         "locId" -> index, "name" -> string (fields)
            tables.set("Node", vector<Hash > ());

            // "DeviceServerInstance" -> Hash (table)
            //     <devSerInsId> -> Hash (rows)
            //         "nodId" -> index, "instanceId" -> string, alias" -> string, "status" -> string (fields)
            tables.set("DeviceServerInstance", vector<Hash > ());

            // "DeviceClass" -> Hash (table)
            //      <devClaId> -> Hash (rows)
            //          "devSerInsId" -> index, "name" -> string, "schema" -> xsdString, version" -> string (fields)
            tables.set("DeviceClass", vector<Hash > ());

            // "DeviceInstance" -> Hash (table)
            //     <devInsId> -> Hash (rows)
            //         "devClaId" -> index, "instanceId" -> string, alias" - > string, "schema" -> xsdString, "configuration" -> Hash, devInsConId" -> index (fields)
            tables.set("DeviceInstance", vector<Hash > ());

            // "DeviceClassConfiguration" -> Hash (table)
            //     <devClaConId> -> Hash (rows)
            //         "devClaId" -> index, "configuration" -> xmlString, "useId" -> index, "version" -> string (fields)
            tables.set("DeviceClassConfiguration", vector<Hash > ());

            // "DeviceInstanceConfiguration" -> Hash (table)
            //     <devInsConId> -> Hash (rows)
            //         "configuration" -> xmlString
            tables.set("DeviceInstanceConfiguration", vector<Hash > ());

            // "Connection" -> Hash (table)
            //     <conId> -> Hash (rows)
            //         "devInsIdSrc" -> index, "devInsIdTgt" -> index, "useId" -> index
            tables.set("Connection", vector<Hash > ());

            // "User" -> Hash (table)
            //     <useId> -> Hash (rows)
            //         "firstName" -> string, "lastName" -> string, "email" -> string, "useRolId" -> index
            tables.set("User", vector<Hash > ());

            // "UserRole" -> Hash (table)
            //     <useRolId> -> Hash (rows)
            // 
            tables.set("UserRole", vector<Hash > ());

            m_database.set("Database", tables);

        }


        void HashDatabase::saveDatabase() {
            boost::mutex::scoped_lock lock(m_databaseMutex);
            karabo::io::saveToFile<Hash>(m_database, KARABO_DB_FILE);
        }


        unsigned int HashDatabase::insert(const std::string& tableName, karabo::util::Hash keyValuePairs) {
            boost::mutex::scoped_lock lock(m_databaseMutex);
            Hash& database = m_database.get<Hash > (KARABO_DB_NAME);
            vector<Hash>& table = database.get<vector<Hash> > (tableName);

            unsigned int id = 0;
            if (!table.empty()) {
                id = table.back().get<unsigned int>("id");
                ++id;
            }
            keyValuePairs.set("id", id);
            table.push_back(keyValuePairs);
            return id;
        }
    }
}




