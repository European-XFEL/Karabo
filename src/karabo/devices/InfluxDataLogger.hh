/* 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DEVICES_INFLUXDATALOGGER_HH
#define	KARABO_DEVICES_INFLUXDATALOGGER_HH

#include <fstream>
#include "DataLogger.hh"
#include <karabo/net/HttpResponse.hh>
#include <karabo/net/InfluxDbClient.hh>

namespace karabo {
    
    namespace devices {

        class InfluxDataLogger;


        typedef boost::function<void()> AsyncHandler;
        typedef boost::function<void(const karabo::net::HttpResponse&)> InfluxResponseHandler;

        struct InfluxDeviceData :
        public karabo::devices::DeviceData, boost::enable_shared_from_this<InfluxDeviceData> {

            KARABO_CLASSINFO(InfluxDeviceData, "InfluxDataLoggerDeviceData", "2.6")

            InfluxDeviceData(const karabo::util::Hash& input);

            virtual ~InfluxDeviceData();

            void handleChanged(const karabo::util::Hash& config, const std::string& user);

            void logValue(const std::string& deviceId, const std::string& path,
                          const std::string& value, const karabo::util::Types::ReferenceType& type);

            void terminateQuery();

            void checkSchemaInDb(const karabo::net::HttpResponse& o);

            void handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceData::Pointer& devicedata);

            void flushOne();

            void stopLogging() override;

            karabo::net::InfluxDbClient::Pointer m_dbClient;

            std::stringstream m_query;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;

            std::string m_digest;

            std::vector<char> m_archive;

        };


        class InfluxDataLogger : public karabo::devices::DataLogger {

        public:

            friend class InfluxDeviceData;

            KARABO_CLASSINFO(InfluxDataLogger, "InfluxDataLogger", "2.6")

            static void expectedParameters(karabo::util::Schema& expected);

            InfluxDataLogger(const karabo::util::Hash& input);

            virtual ~InfluxDataLogger();

            void preDestruction() override;

        protected:

            DeviceData::Pointer createDeviceData(const karabo::util::Hash& config) override;

            void initializeLoggerSpecific() override;

            void flushOne(const DeviceData::Pointer& devicedata) override;

            void handleChanged(const karabo::util::Hash& config, const std::string& user,
                               const DeviceData::Pointer& devicedata) override;

            void handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceData::Pointer& devicedata) override;

        private:

            void checkDb();

            void onPingDb(const karabo::net::HttpResponse& o);

            void showDatabases(const InfluxResponseHandler& action);

            void onShowDatabases(const karabo::net::HttpResponse& o);

            void createDatabase(const std::string& dbname, const InfluxResponseHandler& action);

            void onCreateDatabase(const karabo::net::HttpResponse& o);

        private:

            karabo::net::InfluxDbClient::Pointer m_client;
            std::string m_topic;
            std::string m_hostname;
            static const unsigned int k_httpResponseTimeoutMs;

        };
    }
}

#endif	/* KARABO_DEVICES_INFLUXDATALOGGER_HH */

