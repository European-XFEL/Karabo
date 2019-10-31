/* 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DEVICES_INFLUXDATALOGGER_HH
#define	KARABO_DEVICES_INFLUXDATALOGGER_HH


#include "DataLogger.hh"

namespace karabo {
    
    namespace devices {

        class InfluxDataLogger;

        struct InfluxDeviceData : public karabo::devices::DeviceData {

            KARABO_CLASSINFO(InfluxDeviceData, "InfluxDataLoggerDeviceData", "2.6")

            InfluxDeviceData(const karabo::util::Hash& input);

            virtual ~InfluxDeviceData();

            InfluxDataLogger* m_this;

            std::stringstream m_query;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_serializer;

            void logValue(const std::string& deviceId, const std::string& path,
                          const karabo::util::Timestamp& ts, const std::string& value,
                          const karabo::util::Types::ReferenceType& type);

            void terminateQuery();

        };


        class InfluxDataLogger : public karabo::devices::DataLogger {
            
            friend class InfluxDeviceData;

            std::string m_url;
            karabo::net::Connection::Pointer m_dbConnection;
            karabo::net::Channel::Pointer m_dbChannel;
            bool m_isDbConnected;
            boost::mutex m_bufferMutex;
            std::stringstream m_buffer;
            std::size_t m_bufferLen;
            std::string m_topic;

        public:

            KARABO_CLASSINFO(InfluxDataLogger, "InfluxDataLogger", "2.6")

            static void expectedParameters(karabo::util::Schema& expected);

            InfluxDataLogger(const karabo::util::Hash& input);

            virtual ~InfluxDataLogger();

        private:

            DeviceData::Pointer createDeviceData(const karabo::util::Hash& config) override;

            void initializeBackend(const DeviceData::Pointer& data) override;

            void handleChanged(const karabo::util::Hash& config, const std::string& user, const DeviceData::Pointer& data) override;

            void enqueueQuery(const std::stringstream& ss);

            void flushOne(const DeviceData::Pointer& data) override;

            void handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceData::Pointer& data) override;

            void onDbConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

            void onDbRead(const karabo::net::ErrorCode& ec, const std::string& data);

            void onDbWrite(const karabo::net::ErrorCode& ec);

        };
    }
}

#endif	/* KARABO_DEVICES_INFLUXDATALOGGER_HH */

