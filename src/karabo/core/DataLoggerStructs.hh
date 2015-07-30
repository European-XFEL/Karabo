/* 
 * File:   DataLoggerStructs.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on July 17, 2015, 2:08 PM
 */

#ifndef DATALOGGERSTRUCTS_HH
#define	DATALOGGERSTRUCTS_HH

#include <karabo/util/Epochstamp.hh>

namespace karabo {
    namespace core {

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
            bool marker;     // flag that tells should be current record to be marked
            
            MetaData() : idxFile(), idxStream(), record(), marker(true) {}
        };
        
        struct MetaSearchResult {
            size_t fromFileNumber;
            size_t toFileNumber;
            size_t fromRecord;
            size_t toRecord;
            std::vector<size_t> nrecList;
        };
    }
}



#endif	/* DATALOGGERSTRUCTS_HH */

