/*
 * $Id$
 *
 * Author: <gero.flucke@xfel.eu>
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

#include "DataLogUtils.hh"

#include <boost/iostreams/stream.hpp>

#include "karabo/data/time/DateTimeString.hh"
#include "karabo/data/types/StringTools.hh"

using karabo::data::TIME_UNITS;
using karabo::data::toString;
namespace karabo {
    namespace util {
        namespace nl = nlohmann;


        data::Epochstamp stringDoubleToEpochstamp(const std::string& timestampAsDouble) {
            std::vector<std::string> tparts;
            boost::split(tparts, timestampAsDouble, boost::is_any_of("."));
            const unsigned long long seconds = data::fromString<unsigned long long>(tparts[0]);
            unsigned long long fractions = 0ULL;
            // If by chance we hit a full second without fractions, we have no ".":
            if (tparts.size() >= 2) {
                std::string& fracString = tparts[1];
                // We read in all after the dot. If coming from the raw data logger files, we should have exactly
                // 6 digits (e.g. ms), even with trailing zeros, but one never knows.
                unsigned long long factorToAtto = 1000000000000ULL;
                const size_t nDigitMicrosec = 6;
                if (nDigitMicrosec > fracString.size()) {
                    for (size_t i = 0; i < nDigitMicrosec - fracString.size(); ++i) {
                        factorToAtto *= 10ULL;
                    }
                } else if (nDigitMicrosec < fracString.size()) {
                    for (size_t i = 0; i < fracString.size() - nDigitMicrosec; ++i) {
                        factorToAtto /= 10ULL;
                    }
                }
                const size_t firstNonZero = fracString.find_first_not_of('0');
                if (firstNonZero != 0 && firstNonZero != std::string::npos) {
                    // Get rid of leading 0 which triggers interpretation as octal number:
                    fracString.replace(0, firstNonZero, firstNonZero, ' '); // replace by space avoids any re-allocation
                }
                // Finally, multiply to convert to ATTOSEC:
                fractions = data::fromString<unsigned long long>(fracString) * factorToAtto;
            }

            return data::Epochstamp(seconds, fractions);
        }


        void getLeaves(const data::Hash& configuration, const data::Schema& schema, std::vector<std::string>& result,
                       const char separator) {
            if (configuration.empty() || schema.empty()) return;
            getLeaves_r(configuration, schema, result, "", separator, false);
        }


        void getLeaves_r(const data::Hash& hash, const data::Schema& schema, std::vector<std::string>& result,
                         std::string prefix, const char separator, const bool fullPaths) {
            if (hash.empty()) {
                return;
            }

            for (data::Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                std::string currentKey = it->getKey();

                if (!prefix.empty()) {
                    char separators[] = {separator, 0};
                    currentKey = prefix + separators + currentKey;
                }
                if (it->is<data::Hash>() &&
                    (fullPaths || !it->hasAttribute(KARABO_HASH_CLASS_ID))) { // Recursion, but no hash sub classes
                    getLeaves_r(it->getValue<data::Hash>(), schema, result, currentKey, separator, fullPaths);
                } else if (it->is<std::vector<data::Hash>>()) { // Recursion for vector
                    // if this is a LEAF then don't go to recurse further ... leaf!
                    if (schema.has(currentKey) && schema.isLeaf(currentKey)) {
                        result.push_back(currentKey);
                    } else {
                        for (size_t i = 0; i < it->getValue<std::vector<data::Hash>>().size(); ++i) {
                            std::ostringstream os;
                            os << currentKey << "[" << i << "]";
                            getLeaves_r(it->getValue<std::vector<data::Hash>>().at(i), schema, result, os.str(),
                                        separator, fullPaths);
                        }
                    }
                } else {
                    result.push_back(currentKey);
                }
            }
        }


        // helper function for `jsonResultsToInfluxResultSet`
        void parseSingleJsonResult(const nl::json& respObj, InfluxResultSet& influxResult,
                                   const std::string& columnPrefixToRemove) {
            const auto& result0 = respObj["results"][0];
            if (result0.find("series") == result0.end()) {
                // No data in requested period - can happen with jsonResultsToInfluxResultSet
                // in InfluxLogReader::onGetBadData
                influxResult.first.clear();
                return;
            }
            const auto& columns = result0["series"][0]["columns"];
            std::vector<std::string> columnTitles;
            for (const auto& column : columns) {
                const std::string columnStr = column.get<std::string>();
                const size_t prefixPos = columnStr.find(columnPrefixToRemove);
                if (columnPrefixToRemove.empty() || prefixPos != 0u) {
                    columnTitles.push_back(columnStr);
                } else {
                    columnTitles.push_back(columnStr.substr(columnPrefixToRemove.size()));
                }
            }

            if (influxResult.first.empty()) {
                influxResult.first = std::move(columnTitles);
            } else {
                if (!std::equal(influxResult.first.begin(), influxResult.first.end(), columnTitles.begin())) {
                    throw KARABO_NOT_SUPPORTED_EXCEPTION("Mixed column parsing not supported");
                }
            }

            const auto& rows = result0["series"][0]["values"];
            for (const auto& row : rows) {
                std::vector<boost::optional<std::string>> rowValues;
                rowValues.reserve(row.size());
                for (const auto& value : row) {
                    rowValues.push_back(jsonValueAsString(value));
                }
                influxResult.second.push_back(std::move(rowValues));
            }
        }

        void jsonResultsToInfluxResultSet(const std::string& jsonResult, InfluxResultSet& influxResult,
                                          const std::string& columnPrefixToRemove) {
            nl::json respObj;
            // use boost::iostreams::stream for copy-less stream access of the jsonResult string
            boost::iostreams::stream<boost::iostreams::array_source> inputStream(jsonResult.c_str(), jsonResult.size());
            while (true) {
                inputStream >> respObj;
                parseSingleJsonResult(respObj, influxResult, columnPrefixToRemove);
                // InfluxDB might return multiple JSON concatenated (sic) objects when the
                // number of points exceeds a given limit
                // https://docs.influxdata.com/influxdb/v1.8/tools/api#query-string-parameters
                const auto& result0 = respObj["results"][0];
                auto it = result0.find("partial");
                if (it != result0.end()) {
                    const auto& partial = *it;
                    // continue only if partial == true, break otherwise
                    if (partial.is_boolean() && partial == true) continue;
                }
                break;
            }
        }


        boost::optional<std::string> jsonValueAsString(nl::json value) {
            if (value.is_number_unsigned()) {
                return toString(value.get<unsigned long long>());
            } else if (value.is_number_integer()) {
                return toString(value.get<long long>());
            } else if (value.is_number_float()) {
                return toString(value.get<double>());
            } else if (value.is_string()) {
                return value.get<std::string>();
            } else if (value.is_boolean()) {
                return toString(value.get<bool>());
            } else if (value.is_null()) {
                return boost::none;
            } else {
                // The remaining types recognized by the JSON Parser won't be
                // handled in here. They are: 'is_primitive', 'is_structured',
                // 'is_number' (already handled by the three 'is_number_*' above),
                // 'is_object', 'is_array' and 'is_discarded' (can only be true
                // during JSON parsing).
                return std::string("");
            }
        }

        std::string toInfluxDurationUnit(const TIME_UNITS& karaboDurationUnit) {
            std::string influxDU;

            switch (karaboDurationUnit) {
                case TIME_UNITS::DAY:
                    influxDU = "d";
                    break;
                case TIME_UNITS::HOUR:
                    influxDU = "h";
                    break;
                case TIME_UNITS::MINUTE:
                    influxDU = "m";
                    break;
                case TIME_UNITS::SECOND:
                    influxDU = "s";
                    break;
                case TIME_UNITS::MILLISEC:
                    influxDU = "ms";
                    break;
                case TIME_UNITS::MICROSEC:
                    influxDU = "u";
                    break;
                case TIME_UNITS::NANOSEC:
                    influxDU = "ns";
                    break;
                default:
                    std::ostringstream errMsg;
                    errMsg << "There's no InfluxDb duration corresponding to Karabo's TIME_UNITS '"
                           << karaboDurationUnit << "'.";
                    throw KARABO_PARAMETER_EXCEPTION(errMsg.str());
            }

            return influxDU;
        }


        std::string epochAsMicrosecString(const data::Epochstamp& ep) {
            std::ostringstream epStr;
            const std::string fract(data::DateTimeString::fractionalSecondToString(TIME_UNITS::MICROSEC,
                                                                                   ep.getFractionalSeconds(), true));
            // It is safe to use fract because fractionalSecondToString fills the leading positions with zeros.
            epStr << ep.getSeconds() << fract;
            return epStr.str();
        }


    } // namespace util
} // namespace karabo
