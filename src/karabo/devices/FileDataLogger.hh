/* 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DEVICES_FILEDATALOGGER_HH
#define	KARABO_DEVICES_FILEDATALOGGER_HH

#include "DataLogger.hh"

namespace karabo {
    
    namespace devices {
        
        struct FileDeviceData : public karabo::devices::DeviceData {

            KARABO_CLASSINFO(FileDeviceData, "FileDataLoggerDeviceData", "2.6")

            FileDeviceData(const karabo::util::Hash& input);

            virtual ~FileDeviceData();

            void handleChanged(const karabo::util::Hash& config, const std::string& user) override;

            void logValue(const std::string& deviceId, const std::string& path,
                          const karabo::util::Timestamp& ts, const std::string& value,
                          const karabo::util::Types::ReferenceType& type);

            void flushOne() override;

            /// Helper function to update data.m_idxprops, returns whether data.m_idxprops changed.
            bool updatePropsToIndex();

            /// Helper to ensure archive file is closed.
            /// Must only be called from functions posted on 'data.m_strand'.
            void ensureFileClosed();

            int determineLastIndex(const std::string& deviceId) const;

            int incrementLastIndex(const std::string& deviceId);

            void handleSchemaUpdated(const karabo::util::Schema& schema) override;

            void setupDirectory();

            std::string m_directory;
            int m_maxFileSize;
            std::fstream m_configStream;

            unsigned int m_lastIndex;

            std::map<std::string, karabo::util::MetaData::Pointer> m_idxMap;
            std::vector<std::string> m_idxprops;
            size_t m_propsize;
            time_t m_lasttime;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_serializer;
        };


        class FileDataLogger : public karabo::devices::DataLogger {

        public:

            KARABO_CLASSINFO(FileDataLogger, "FileDataLogger", "2.6")

            static void expectedParameters(karabo::util::Schema& expected);
                    
            FileDataLogger(const karabo::util::Hash& input);

            virtual ~FileDataLogger();

        private:

            DeviceData::Pointer createDeviceData(const karabo::util::Hash& config) override;

            void initializeBackend(const DeviceData::Pointer& data) override;

        };
    }
}

#endif	/* KARABO_DEVICES_FILEDATALOGGER_HH */

