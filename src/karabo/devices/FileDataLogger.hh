/* 
 * File:   FileDataLogger.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on October 25, 2019, 4:05 PM
 */

#ifndef FILEDATALOGGER_HH
#define	FILEDATALOGGER_HH

#include "DataLogger.hh"

namespace karabo {
    
    namespace devices {
        
        struct FileDeviceData : public DeviceData {

            KARABO_CLASSINFO(FileDeviceData, "FileDataLoggerDeviceData", "2.6")

            FileDeviceData(const karabo::util::Hash& input);

            virtual ~FileDeviceData();

            karabo::util::Schema m_currentSchema;

            std::fstream m_configStream;

            unsigned int m_lastIndex;
            std::string m_user;
            boost::mutex m_lastTimestampMutex;
            karabo::util::Timestamp m_lastDataTimestamp;
            bool m_updatedLastTimestamp;
            bool m_pendingLogin;

            std::map<std::string, karabo::util::MetaData::Pointer> m_idxMap;
            std::vector<std::string> m_idxprops;
            size_t m_propsize;
            time_t m_lasttime;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_serializer;
        };


        class FileDataLogger : public DataLogger {

        public:

            KARABO_CLASSINFO(FileDataLogger, "FileDataLogger", "2.6")
                    
            FileDataLogger(const karabo::util::Hash& input);

            virtual ~FileDataLogger();

            DeviceData::Pointer create(const karabo::util::Hash& config);

            void setupDirectory(const DeviceData::Pointer& data);

            void handleChanged(const karabo::util::Hash& config, const std::string& user, const DeviceData::Pointer& data);

            /// Helper function to update data.m_idxprops, returns whether data.m_idxprops changed.
            bool updatePropsToIndex(FileDeviceData& data);

            /// Helper to ensure archive file is closed.
            /// Must only be called from functions posted on 'data.m_strand'.
            void ensureFileClosed(FileDeviceData& data);

            void doFlush();

            void flushOne(const FileDeviceData::Pointer& data);

            int determineLastIndex(const std::string& deviceId) const;

            int incrementLastIndex(const std::string& deviceId);

            void handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceData::Pointer& data);

        };
    }
}

#endif	/* FILEDATALOGGER_HH */

