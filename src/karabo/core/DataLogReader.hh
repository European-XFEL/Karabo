/* 
 * File:   DataLogReader.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on January 29, 2015, 11:04 AM
 */

#ifndef DATALOGREADER_HH
#define	DATALOGREADER_HH

#include <boost/filesystem.hpp>
#include "Device.hh"
#include "OkErrorFsm.hh"


/**
 * The main karabo namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        struct DataLoggerIndex {
            std::string m_event;
            karabo::util::Epochstamp m_epoch;
            unsigned long long m_train;
            long m_position;
            std::string m_user;
            int m_fileindex;

            DataLoggerIndex()
            : m_event()
            , m_epoch(0, 0)
            , m_train(0)
            , m_position(-1)
            , m_user(".")
            , m_fileindex(-1) {
            }
        };

        class DataLogReader : public karabo::core::Device<karabo::core::OkErrorFsm> {
        public:

            KARABO_CLASSINFO(DataLogReader, "DataLogReader", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            DataLogReader(const karabo::util::Hash& input);

            virtual ~DataLogReader();

        private: // Functions

            void okStateOnEntry();

            void slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const karabo::util::Hash& params);

            void slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint);

            DataLoggerIndex findLoggerIndexTimepoint(const std::string& deviceId, const std::string& timepoint);

            DataLoggerIndex findNearestLoggerIndex(const std::string& deviceId, const karabo::util::Epochstamp& timepoint);

            int getFileIndex(const std::string& deviceId);

        };
    }
}

#endif	/* DATALOGREADER_HH */

