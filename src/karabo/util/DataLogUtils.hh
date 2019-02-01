/*
 * File:   DataLoggerStructs.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on July 17, 2015, 2:08 PM
 */

#ifndef DATALOGGERSTRUCTS_HH
#define	DATALOGGERSTRUCTS_HH

#include <vector>
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"

namespace karabo {
    namespace util {

        char const * const DATALOGMANAGER_ID = "Karabo_DataLoggerManager_0";
        char const * const DATALOGGER_PREFIX = "DataLogger-";
        char const * const DATALOGREADER_PREFIX = "DataLogReader";
        //    ts=timestamp
        //    tsAsIso8601 : numbers, dot and uppercase letters (timezone)
        //    tsAsDouble  : numbers and a dot (positive double)
        //    trainId     : unsigned long long
        //    path        : one or more characters
        //    type        : 0 or more characters
        //    user        : 0 or more lower case letters, numbers and underscores
        //    flag        : one or more uppercase letters
        //
        //                                          tsAsIso8601   | tsAsDouble  | trainId  | path |    type      | value|   user       |flag
        char const * const DATALOG_LINE_REGEX = "^([A-Z0-9\\.]+)\\|([0-9\\.]+)\\|([0-9]+)\\|(.+)\\|([0-9A-Z_]*)\\|(.*)\\|([a-z0-9_]*)\\|([A-Z]+)$";

        //    event       : indexing event type (+LOG, -LOG, =NEW)
        //    ts=timestamp
        //    tsAsIso8601 : numbers, dot and uppercase letters (timezone)
        //    tsAsDouble  : numbers and a dot (positive double)
        //    tail        : whatever comes afterwards
        //                                                 event      tsAsIso8601    tsAsDouble tail
        char const * const DATALOG_INDEX_LINE_REGEX = "^([A-Z=\\+\\-]+)[\\s]+([A-Z0-9\\.]+)[\\s]+([0-9\\.]+)[\\s]+(.+)$";

        //    trainId     : numbers (integer; can be negative)
        //    position    : numbers (positive integer)
        //    user        : lowercase letters, numbers and underscores (can also be a point)
        //    fileIndex   : numbers (positive integer)
        //                                             trainId       position user            fileIndex
        char const * const DATALOG_INDEX_TAIL_REGEX = "^([0-9\\-]+)[\\s]+([0-9]+)[\\s]+([a-z0-9_\\.]*)[\\s]+([0-9]+)$";

        unsigned int const DATALOGREADERS_PER_SERVER = 2;

        /**
         * A structure defining meta data as used by the data loggers
         */
        struct MetaData {

            typedef boost::shared_ptr<MetaData> Pointer;

            struct Record {

                double epochstamp;
                unsigned long long trainId;
                unsigned long long positionInRaw;
                unsigned int extent1;
                unsigned int extent2;

                Record() : epochstamp(0.0), trainId(0), positionInRaw(0), extent1(0), extent2(0) {
                }
            };

            std::string idxFile;
            std::ofstream idxStream;
            Record record;
            bool marker; // flag that tells should be current record to be marked

            MetaData() : idxFile(), idxStream(), record(), marker(true) {
            }
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

            MetaSearchResult() : fromFileNumber(0), toFileNumber(0), fromRecord(0), toRecord(0) {
            }
        };

        /**
         * Convert an std::string that represents a double of the seconds since Unix epoch
         * to an Epochstamp
         */
        util::Epochstamp stringDoubleToEpochstamp(const std::string& timestampAsDouble);
        
        void getLeaves(const karabo::util::Hash& configuration, const karabo::util::Schema& schema,
                       std::vector<std::string>& result, const char separator='.');

        void getLeaves_r(const karabo::util::Hash& hash, const karabo::util::Schema& schema, std::vector<std::string>& result,
                         std::string prefix, const char separator, const bool fullPaths);
    }
}



#endif	/* DATALOGGERSTRUCTS_HH */

