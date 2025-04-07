/*
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on July 17, 2015, 2:08 PM
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

#ifndef DATALOGGERSTRUCTS_HH
#define DATALOGGERSTRUCTS_HH

#include <boost/optional.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"


namespace karabo {
    namespace util {

        char const* const INFLUX_DURATION_UNIT = "u";         // Duration unit used for Influx (microsecs)
        unsigned int const INFLUX_PRECISION_FACTOR = 1000000; // Precision factor for time used in Influx

        char const* const DATALOGMANAGER_ID = "Karabo_DataLoggerManager_0";
        char const* const DATALOGGER_PREFIX = "DataLogger-";
        char const* const DATALOGREADER_PREFIX = "DataLogReader-";
        //    ts=timestamp
        //    tsAsIso8601 : numbers, dot and uppercase letters (timezone)
        //    tsAsDouble  : numbers and a dot (positive double)
        //    trainId     : unsigned long long
        //    path        : one or more characters, "." in case of LOGOUT events
        //    type        : 1 or more characters, starts with a capital letter, or empty in case of LOGOUT events
        //    value       : 0 or more characters, timestamp in case of LOGOUT events
        //    user        : 0 or more lower case letters, numbers and underscores
        //    flag        : one or more uppercase letters
        //
        //        tsAsIso8601  | tsAsDouble  | trainId  | path |    type           | value|   user       |flag
        char const* const DATALOG_LINE_REGEX =
              "^([TZ0-9\\.]+)\\|([0-9\\.]+)\\|([0-9]+)\\|(.+)\\|([A-Z][0-9A-Z_]+)\\|(.*)\\|([a-z0-9_]*)\\|([A-Z]+)$";

        // this will match logout lines. Needed because the single regex expression used in the python migration script
        // does not reliably work with boost
        //        tsAsIso8601  | tsAsDouble  | trainId  |path*|    type**   | value|   user       |flag
        char const* const DATALOG_LOGOUT_REGEX =
              "^([TZ0-9\\.]+)\\|([0-9\\.]+)\\|([0-9]+)\\|\\.\\|(![\\s\\S])\\|(.*)\\|([a-z0-9_]*)\\|([A-Z]+)$";
        // *  path: always ".""
        // ** type: always empty

        //    event       : indexing event type (+LOG, -LOG, =NEW)
        //    ts=timestamp
        //    tsAsIso8601 : numbers, dot and uppercase letters (timezone)
        //    tsAsDouble  : numbers and a dot (positive double)
        //    tail        : whatever comes afterwards
        //                                                 event      tsAsIso8601    tsAsDouble tail
        char const* const DATALOG_INDEX_LINE_REGEX = "^([A-Z=\\+\\-]+)[\\s]+([TZ0-9\\.]+)[\\s]+([0-9\\.]+)[\\s]+(.+)$";

        //    trainId     : numbers (non-negative integer)
        //    position    : numbers (positive integer)
        //    user        : lowercase letters, numbers and underscores (can also be a point)
        //    fileIndex   : numbers (positive integer)
        //                                             trainId       position user            fileIndex
        char const* const DATALOG_INDEX_TAIL_REGEX = "^([0-9]+)[\\s]+([0-9]+)[\\s]+([a-z0-9_\\.]*)[\\s]+([0-9]+)$";

        // replacement for '\n' in data logger files
        char const* const DATALOG_NEWLINE_MANGLE = ".KRB_NEWLINE.";


        // Since Telegraf states a limit of 1 Mb as the maximum length acceptable for a string
        // metric, a value known to be below that limit is used for saving string metrics.
        // Schemas whose sizes are above the value of MAX_INFLUX_VALUE_LENGTH bytes are split
        // into chunks of MAX_INFLUX_VALUE_LENGTH bytes.
        const unsigned int MAX_INFLUX_VALUE_LENGTH = 921600u; // 900 *1024 bytes

        /**
         * A structure defining meta data as used by the data loggers
         */
        struct MetaData {
            typedef std::shared_ptr<MetaData> Pointer;

            struct Record {
                double epochstamp;
                unsigned long long trainId;
                unsigned long long positionInRaw;
                unsigned int extent1;
                unsigned int extent2;

                Record() : epochstamp(0.0), trainId(0), positionInRaw(0), extent1(0), extent2(0) {}
            };

            std::string idxFile;
            std::ofstream idxStream;
            Record record;
            bool marker; // flag that tells should be current record to be marked

            MetaData() : idxFile(), idxStream(), record(), marker(true) {}
        };

        /**
         * A structure defining meta data as used by the data logger's search results
         */
        struct MetaSearchResult {
            size_t fromFileNumber;
            size_t toFileNumber;
            size_t fromRecord;
            size_t toRecord;
            std::vector<size_t> nrecList;

            MetaSearchResult() : fromFileNumber(0), toFileNumber(0), fromRecord(0), toRecord(0) {}
        };

        /**
         * Convert an std::string that represents a double of the seconds since Unix epoch
         * to an Epochstamp
         */
        data::Epochstamp stringDoubleToEpochstamp(const std::string& timestampAsDouble);

        void getLeaves(const karabo::data::Hash& configuration, const karabo::data::Schema& schema,
                       std::vector<std::string>& result, const char separator = karabo::data::Hash::k_defaultSep);

        void getLeaves_r(const karabo::data::Hash& hash, const karabo::data::Schema& schema,
                         std::vector<std::string>& result, std::string prefix, const char separator,
                         const bool fullPaths);

        std::string toInfluxDurationUnit(const karabo::data::TIME_UNITS& karaboDurationUnit);

        std::string epochAsMicrosecString(const karabo::data::Epochstamp& ep);

        /**
         * Utility function to convert a json object.
         *
         * @param value
         * @return a boost::optional<std::string> a null boost::optional<std::string> matches a null JSON value
         */
        boost::optional<std::string> jsonValueAsString(nlohmann::json value);

        // InfluxResultSet is a pair with a vector of column names in its first position and
        // a vector of rows of values represented as optional strings in its second position.
        // The optional strings have no value when they correspond to nulls returned by Influx.
        using InfluxResultSet = std::pair<
              /* first  */ std::vector<std::string>,
              /* second */ std::vector<std::vector<boost::optional<std::string>>>>;

        /**
         * Utility function to convert a string into an InfluxResultSet
         *
         * One or multiple concatenated JSON objects containing the results of an InfluxDB query
         * are decoded and filled into a InfluxResultSet object.
         *
         * @param jsonResult : the string containing the JSON object(s)
         * @param influxResult : the result
         * @param columnPrefixToRemove : remove this prefix from the column names.
         *                               InfluxQL selector functions (e.g. SAMPLE) are
         *                               prepended to the column name. Use this argument to
         *                               remove said prefixes.
         * @throw  karabo::util::NotSupportedException in case the column mismatch
         *         nlohmann::json exceptions in case of malformatted JSON objects.
         */
        void jsonResultsToInfluxResultSet(const std::string& jsonResult, InfluxResultSet& influxResult,
                                          const std::string& columnPrefixToRemove);

    } // namespace util
} // namespace karabo


#endif /* DATALOGGERSTRUCTS_HH */
