/*
 * File:   DataLoggerStructs.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on July 17, 2015, 2:08 PM
 */

#ifndef DATALOGGERSTRUCTS_HH
#define	DATALOGGERSTRUCTS_HH

#include "karabo/util/Epochstamp.hh"

namespace karabo {
    namespace util {

        char const * const DATALOGMANAGER_ID = "Karabo_DataLoggerManager_0";
        char const * const DATALOGGER_PREFIX = "DataLogger-";
        char const * const DATALOGREADER_PREFIX = "DataLogReader";
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

    }
}



#endif	/* DATALOGGERSTRUCTS_HH */

