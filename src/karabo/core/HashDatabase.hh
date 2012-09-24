/*
 * $Id$
 *
 * File:   HashDatabase.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 16, 2012, 4:43 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_CORE_HASHDATABASE_HH
#define	EXFEL_CORE_HASHDATABASE_HH

#include "Device.hh"

// TODO This whole class could be moved to IO-Package from a dependency point of view


namespace exfel {
    namespace core {

#define EXFEL_DB_NAME "Database"
#define EXFEL_DB_FILE "database.xml"

        
        
        class HashDatabase {
            
        public: // functions
            
            typedef std::vector<exfel::util::Hash> ResultType;

            static bool readDatabase();
            
            static void setupDatabase();

            static void saveDatabase();
            
            static unsigned int insert(const std::string& tableName, exfel::util::Hash keyValuePairs);
            
        public: // members
            
            static exfel::util::Hash m_database;
            
            static boost::mutex m_databaseMutex;
            
        };
    }
}

#define EXFEL_DB_READ HashDatabase::readDatabase();

#define EXFEL_DB_SETUP HashDatabase::setupDatabase();

#define EXFEL_DB_SAVE HashDatabase::saveDatabase();

#define EXFEL_DB_INSERT(tableName, keyValuePairs) HashDatabase::insert(tableName, keyValuePairs);

#define EXFEL_DB_SELECT(result, what, tableName, condition) { \
    boost::mutex::scoped_lock lock(HashDatabase::m_databaseMutex); \
    std::vector<std::string> _fields; \
    boost::split(_fields, what, boost::is_any_of(",")); \
    Hash& database = HashDatabase::m_database.get<Hash > (EXFEL_DB_NAME); \
    vector<Hash>& _table = database.get<vector<Hash> >(tableName); \
    Hash resultSet; \
    for (size_t i = 0; i < _table.size(); ++i) { \
        const Hash& row = _table[i]; \
        if (condition) { \
            Hash rowResult; \
            for (size_t i = 0; i < _fields.size(); ++i) { \
                Hash::const_iterator fieldIt = row.find(_fields[i]); \
                if (fieldIt == row.end()) throw PARAMETER_EXCEPTION("Selection key \"" + _fields[i] + "\" is not a valid field name in table \"" + tableName + "\""); \
                const string& field = _fields[i]; \
                rowResult[field] = row.find(field)->second; \
            } \
            result.push_back(rowResult); \
        } \
    } \
}

#define EXFEL_DB_UPDATE(tableName, keyValuePairs, condition) { \
    boost::mutex::scoped_lock lock(HashDatabase::m_databaseMutex); \
    Hash& database = HashDatabase::m_database.get<Hash > (EXFEL_DB_NAME); \
    vector<Hash>& table = database.get<vector<Hash> >(tableName); \
    for (size_t i = 0; i < table.size(); ++i) { \
        Hash& row = table[i]; \
        if (condition) { \
            row.update(keyValuePairs); \
        } \
    } \
}

#define EXFEL_DB_DELETE(tableName, condition) { \
    boost::mutex::scoped_lock lock(HashDatabase::m_databaseMutex); \
    Hash& database = HashDatabase::m_database.get<Hash > (EXFEL_DB_NAME); \
    vector<Hash>& table = database.get<vector<Hash> >(tableName); \
    if (table.size() > 0) { \
        vector<vector<Hash>::iterator > toBeDeleted; \
        for (vector<Hash>::iterator __i = table.begin(); __i != table.end(); ++__i) { \
            const Hash& row = *__i; \
            if (condition) { \
                toBeDeleted.push_back(__i); \
            } \
        } \
        for (size_t __i = 0; __i < toBeDeleted.size(); ++__i) { \
            table.erase(toBeDeleted[__i]); \
        } \
    } \
}

#endif
