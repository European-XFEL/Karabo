/*
 * Author: <gero.flucke@xfel.eu>
 *
 * Created on March 23, 2016, 12:10 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "TableElement.hh"

#include <set>
#include <sstream>
#include <utility>

#include "karabo/data/types/ToLiteral.hh"
#include "karabo/data/types/Types.hh"

const karabo::data::Validator::ValidationRules karabo::data::tableValidationRules(
      /* injectDefaults */ true,
      /* allowUnrootedConfiguration */ true,
      /* allowAdditionalKeys */ false,
      /* allowMissingKeys */ false,
      /* injectTimestamps */ false);
namespace karabo {
    namespace data {

        // Types supported for table element columns.
        const std::set<Types::ReferenceType> SUPPORTED_TBL_COL_TYPES = {
              Types::BOOL,          Types::INT8,         Types::UINT8,         Types::INT16,
              Types::UINT16,        Types::INT32,        Types::UINT32,        Types::INT64,
              Types::UINT64,        Types::FLOAT,        Types::DOUBLE,        Types::STRING,
              Types::VECTOR_BOOL,   Types::VECTOR_INT8,  Types::VECTOR_UINT8,  Types::VECTOR_INT16,
              Types::VECTOR_UINT16, Types::VECTOR_INT32, Types::VECTOR_UINT32, Types::VECTOR_INT64,
              Types::VECTOR_UINT64, Types::VECTOR_FLOAT, Types::VECTOR_DOUBLE, Types::VECTOR_STRING};


        void TableElement::beforeAddition() {
            this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
            this->m_node->setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::data::Schema::PROPERTY);
            this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Table");
            this->m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, "VECTOR_HASH");
            if (m_nodeSchema.empty()) {
                std::stringstream s;
                s << "Table element '" << this->m_node->getKey() << "' has an empty row schema, "
                  << "likely a call to setColumns(..) is missing.";
                throw KARABO_LOGIC_EXCEPTION(s.str());
            }
            this->m_node->setAttribute(KARABO_SCHEMA_ROW_SCHEMA, m_nodeSchema);

            // m_nodeSchema can be used below because it has just been assigned
            // to the table row schema and the findUnsupportedColumnType doesn't
            // change it.
            const auto& unsupCol = findUnsupportedColumnType(m_nodeSchema);
            if (!unsupCol.first.empty()) {
                std::ostringstream oss;
                oss << "Table element '" << m_node->getKey() << "' has a column, '" << unsupCol.first
                    << "', of unsupported type '" << Types::to<ToLiteral>(unsupCol.second) << "'.";
                throw KARABO_PARAMETER_EXCEPTION(oss.str());
            }

            if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->init(); // This is the default

            if (!this->m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {
                // for init, reconfigurable elements - set default value of requiredAccessLevel to USER
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) ||              // init element
                    this->m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT ||  // init element
                    this->m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE) { // reconfigurable element

                    this->userAccess();

                } else { // else set default value of requiredAccessLevel to OBSERVER
                    this->observerAccess();
                }
            }

            // protect setting options etc to table element via overwrite
            OverwriteElement::Restrictions restrictions;
            restrictions.options = true;
            restrictions.minInc = true;
            restrictions.minExc = true;
            restrictions.maxInc = true;
            restrictions.maxExc = true;
            m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());

            sanitizeRowSchema(m_node->getAttribute<Schema>(KARABO_SCHEMA_ROW_SCHEMA));
        }


        std::pair<std::string, Types::ReferenceType> TableElement::findUnsupportedColumnType(const Schema& rowSchema) {
            auto res = std::make_pair<std::string, Types::ReferenceType>("", Types::UNKNOWN);
            const std::vector<std::string>& columns = rowSchema.getPaths();
            for (const std::string& col : columns) {
                const Types::ReferenceType colType = rowSchema.getValueType(col);
                if (SUPPORTED_TBL_COL_TYPES.count(colType) == 0) {
                    // Column type not supported.
                    res.first = col;
                    res.second = colType;
                    break;
                }
            }
            return res;
        }


        void TableElement::sanitizeRowSchema(Schema& rowSchema) {
            sanitizeColumnsAccessModes(rowSchema);
            sanitizeNoDefaultColumns(rowSchema);
        }


        void TableElement::sanitizeColumnsAccessModes(Schema& rowSchema) {
            const int tblAccessMode = m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE);
            const std::vector<std::string>& columns = rowSchema.getPaths();
            if (tblAccessMode == READ) {
                // For read-only tables, all columns should be read-only.
                for (const std::string& col : columns) {
                    const int colAccessMode = rowSchema.getAccessMode(col);
                    if (colAccessMode != READ) {
                        rowSchema.setAccessMode(col, READ);
                        std::cerr << "\nTABLE SANITIZE (" << m_node->getKey() << "): Non read-only column '" << col
                                  << "' of read-only table had its access mode " << "adjusted to read-only."
                                  << std::endl;
                    }
                }
            } else {
                // For non read-only tables, all init-only columns should
                // become writable (read-only and writable columns should keep
                // their access modes).
                for (const std::string& col : columns) {
                    const int colAccessMode = rowSchema.getAccessMode(col);
                    if (colAccessMode == INIT) {
                        rowSchema.setAccessMode(col, WRITE);
                        std::cerr << "\nTABLE SANITIZE (" << m_node->getKey() << "): init-only column '" << col
                                  << "' of non read-only table had its access mode " << "adjusted to reconfigurable."
                                  << std::endl;
                    }
                }
            }
        }


        void TableElement::sanitizeNoDefaultColumns(Schema& rowSchema) {
            const std::vector<std::string>& columns = rowSchema.getPaths();
            for (const std::string& col : columns) {
                if (!rowSchema.hasDefaultValue(col)) {
                    const Types::ReferenceType colType = rowSchema.getValueType(col);
                    setDefaultValueForColumn(col, colType, rowSchema);
                    std::cerr << "\nTABLE SANITIZE (" << m_node->getKey() << "):" << "column '" << col
                              << "' lacked a default value. " << "A zero or empty default value was added."
                              << std::endl;
                }
            }
        }


        void TableElement::setDefaultValueForColumn(const std::string& colName, const Types::ReferenceType& colType,
                                                    Schema& rowSchema) {
            if (Types::isVector(colType) && rowSchema.hasMinSize(colName)) {
                // Checks if the default value for vectors, which is the empty
                // vector, doesn't violate any existing minSize attr in the row
                // schema.
                const int minVecSize = rowSchema.getMinSize(colName);
                if (minVecSize > 0) {
                    std::ostringstream oss;
                    oss << "Cannot generate default value for column '" << colName << "': the minimum vector size, '"
                        << minVecSize << "', is greater than '0', the size of " << "the default vector.";
                    throw KARABO_PARAMETER_EXCEPTION(oss.str());
                }
            } else if (Types::isSimple(colType)) {
                checkSimpleDefaultInOptions(colName, colType, rowSchema);
                if (Types::isNumericPod(colType)) {
                    checkNumericDefaultInRange(colName, rowSchema);
                }
            }

            switch (colType) {
#define SET_DEFAULT_VALUE(K_TYPE, CPP_TYPE, DEFAULT_VALUE)           \
    case Types::ReferenceType::K_TYPE:                               \
        rowSchema.setDefaultValue<CPP_TYPE>(colName, DEFAULT_VALUE); \
        break;

                SET_DEFAULT_VALUE(BOOL, bool, false);
                SET_DEFAULT_VALUE(INT8, signed char, 0);
                SET_DEFAULT_VALUE(INT16, signed short, 0);
                SET_DEFAULT_VALUE(INT32, int, 0);
                SET_DEFAULT_VALUE(INT64, long long, 0ll);
                SET_DEFAULT_VALUE(UINT8, unsigned char, 0);
                SET_DEFAULT_VALUE(UINT16, unsigned short, 0);
                SET_DEFAULT_VALUE(UINT32, unsigned int, 0u);
                SET_DEFAULT_VALUE(UINT64, unsigned long long, 0ull);
                SET_DEFAULT_VALUE(FLOAT, float, 0.0f);
                SET_DEFAULT_VALUE(DOUBLE, double, 0.0);
                SET_DEFAULT_VALUE(STRING, std::string, "");
                SET_DEFAULT_VALUE(VECTOR_BOOL, std::vector<bool>, std::vector<bool>());
                SET_DEFAULT_VALUE(VECTOR_INT8, std::vector<signed char>, std::vector<signed char>());
                SET_DEFAULT_VALUE(VECTOR_INT16, std::vector<signed short>, std::vector<signed short>());
                SET_DEFAULT_VALUE(VECTOR_INT32, std::vector<int>, std::vector<int>());
                SET_DEFAULT_VALUE(VECTOR_INT64, std::vector<long long>, std::vector<long long>());
                SET_DEFAULT_VALUE(VECTOR_UINT8, std::vector<unsigned char>, std::vector<unsigned char>());
                SET_DEFAULT_VALUE(VECTOR_UINT16, std::vector<unsigned short>, std::vector<unsigned short>());
                SET_DEFAULT_VALUE(VECTOR_UINT32, std::vector<unsigned int>, std::vector<unsigned int>());
                SET_DEFAULT_VALUE(VECTOR_UINT64, std::vector<unsigned long long>, std::vector<unsigned long long>());
                SET_DEFAULT_VALUE(VECTOR_FLOAT, std::vector<float>, std::vector<float>());
                SET_DEFAULT_VALUE(VECTOR_DOUBLE, std::vector<double>, std::vector<double>());
                SET_DEFAULT_VALUE(VECTOR_STRING, std::vector<std::string>, std::vector<std::string>());

#undef SET_DEFAULT_VALUE

                default:
                    std::ostringstream oss;
                    oss << "Column '" << colName << "' lacks a default value and is of an unsupported type, '"
                        << Types::to<ToLiteral>(colType) << "'.";
                    throw KARABO_PARAMETER_EXCEPTION(oss.str());
            }
        }


        void TableElement::checkSimpleDefaultInOptions(const std::string& colName, const Types::ReferenceType& colType,
                                                       const Schema& rowSchema) {
            if (!rowSchema.hasOptions(colName)) {
                return;
            }
            const Hash& rowSchemaHash = rowSchema.getParameterHash();
            const Hash::Node& colNode = rowSchemaHash.getNode(colName);
            bool isDefaultInOptions = true;

            switch (colType) {
#define CHECK_DEFAULT_IN_OPTIONS(K_TYPE, C_TYPE, DEFAULT_VALUE)                                      \
    case Types::ReferenceType::K_TYPE: {                                                             \
        const auto& opVals = colNode.getAttribute<std::vector<C_TYPE>>(KARABO_SCHEMA_OPTIONS);       \
        isDefaultInOptions = std::find(opVals.begin(), opVals.end(), DEFAULT_VALUE) != opVals.end(); \
        break;                                                                                       \
    }

                // Unlikely, but possible, for a BOOL_ELEMENT to specify
                // options.
                CHECK_DEFAULT_IN_OPTIONS(BOOL, bool, false);
                CHECK_DEFAULT_IN_OPTIONS(INT8, signed char, 0);
                CHECK_DEFAULT_IN_OPTIONS(INT16, signed short, 0);
                CHECK_DEFAULT_IN_OPTIONS(INT32, int, 0);
                CHECK_DEFAULT_IN_OPTIONS(INT64, long long, 0ll);
                CHECK_DEFAULT_IN_OPTIONS(UINT8, unsigned char, 0);
                CHECK_DEFAULT_IN_OPTIONS(UINT16, unsigned short, 0);
                CHECK_DEFAULT_IN_OPTIONS(UINT32, unsigned int, 0u);
                CHECK_DEFAULT_IN_OPTIONS(UINT64, unsigned long long, 0ull);
                CHECK_DEFAULT_IN_OPTIONS(FLOAT, float, 0.0f);
                CHECK_DEFAULT_IN_OPTIONS(DOUBLE, double, 0.0);
                CHECK_DEFAULT_IN_OPTIONS(STRING, std::string, "");

#undef CHECK_DEFAULT_IN_OPTIONS

                default: {
                    std::ostringstream oss;
                    oss << "Column '" << colName << "' lacks a default value and is of an unsupported type, '"
                        << Types::to<ToLiteral>(colType) << "'.";
                    throw KARABO_PARAMETER_EXCEPTION(oss.str());
                }
            }

            if (!isDefaultInOptions) {
                std::ostringstream oss;
                oss << "Default value to be generated for column '" << colName << "' is not among the valid options.";
                throw KARABO_PARAMETER_EXCEPTION(oss.str());
            }
        }


        void TableElement::checkNumericDefaultInRange(const std::string& colName, const Schema& rowSchema) {
            // Checks if the default value is not outside any range
            // specified by at least one of minInc, mixExc, maxInc, and
            // maxExc.
            const Hash& rowSchemaHash = rowSchema.getParameterHash();
            const Hash::Node& colNode = rowSchemaHash.getNode(colName);
            if (rowSchema.hasMinExc(colName)) {
                double minExc = colNode.getAttributeAs<double>(KARABO_SCHEMA_MIN_EXC);
                if (0.0 <= minExc) {
                    std::ostringstream oss;
                    oss << "Default value to be generated for column '" << colName
                        << "' would be outside of lower bound '" << minExc << "'.";
                    throw KARABO_PARAMETER_EXCEPTION(oss.str());
                }
            }
            if (rowSchema.hasMinInc(colName)) {
                double minInc = colNode.getAttributeAs<double>(KARABO_SCHEMA_MIN_INC);
                if (0.0 < minInc) {
                    std::ostringstream oss;
                    oss << "Default value to be generated for column '" << colName
                        << "' would be outside of lower bound '" << minInc << "'.";
                    throw KARABO_PARAMETER_EXCEPTION(oss.str());
                }
            }
            if (rowSchema.hasMaxExc(colName)) {
                double maxExc = colNode.getAttributeAs<double>(KARABO_SCHEMA_MAX_EXC);
                if (0.0 >= maxExc) {
                    std::ostringstream oss;
                    oss << "Default value to be generated for column '" << colName
                        << "' would be outside of upper bound '" << maxExc << "'.";
                    throw KARABO_PARAMETER_EXCEPTION(oss.str());
                }
            }
            if (rowSchema.hasMaxInc(colName)) {
                double maxInc = colNode.getAttributeAs<double>(KARABO_SCHEMA_MAX_INC);
                if (0.0 > maxInc) {
                    std::ostringstream oss;
                    oss << "Default value to be generated for column '" << colName
                        << "' would be outside of upper bound '" << maxInc << "'.";
                    throw KARABO_PARAMETER_EXCEPTION(oss.str());
                }
            }
        }


    } // namespace data
} // namespace karabo
