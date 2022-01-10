/*
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_DEVICES_INFLUXDATALOGGER_HH
#define KARABO_DEVICES_INFLUXDATALOGGER_HH

#include <fstream>
#include <karabo/net/HttpResponse.hh>
#include <karabo/net/InfluxDbClient.hh>

#include "DataLogger.hh"
#include "karabo/util/Version.hh"

namespace karabo {

    namespace devices {

        class InfluxDataLogger;


        typedef boost::function<void()> AsyncHandler;
        typedef boost::function<void(const karabo::net::HttpResponse&)> InfluxResponseHandler;

        struct InfluxDeviceData : public karabo::devices::DeviceData {
            KARABO_CLASSINFO(InfluxDeviceData, "InfluxDataLoggerDeviceData", "2.6")

            InfluxDeviceData(const karabo::util::Hash& input);

            virtual ~InfluxDeviceData();

            void handleChanged(const karabo::util::Hash& config, const std::string& user) override;

            void logValue(std::stringstream& query, const std::string& deviceId, const std::string& path,
                          const std::string& value, karabo::util::Types::ReferenceType type, bool isFinite);

            /**
             * Helper to store logging start event
             *
             * @param configuration full device configuration received when logging starts
             * @param sortedPaths full paths of configuration, sorted by increasing timestamp
             */
            void login(const karabo::util::Hash& configuration, const std::vector<std::string>& sortedPaths);

            void terminateQuery(std::stringstream& query, const karabo::util::Timestamp& stamp);

            void checkSchemaInDb(const karabo::util::Timestamp& stamp, const std::string& schDigest,
                                 const karabo::net::HttpResponse& o);

            void handleSchemaUpdated(const karabo::util::Schema& schema, const karabo::util::Timestamp& stamp) override;

            void stopLogging() override;

            karabo::net::InfluxDbClient::Pointer m_dbClientRead;
            karabo::net::InfluxDbClient::Pointer m_dbClientWrite;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;

            std::vector<char> m_archive;
            int m_maxTimeAdvance;
            bool m_hasRejectedData;

            karabo::util::Timestamp m_loggingStartStamp;
        };


        class InfluxDataLogger : public karabo::devices::DataLogger {
           public:
            friend class InfluxDeviceData;

            KARABO_CLASSINFO(InfluxDataLogger, "InfluxDataLogger", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::util::Schema& expected);

            InfluxDataLogger(const karabo::util::Hash& input);

            virtual ~InfluxDataLogger();

            void preDestruction() override;

           protected:
            DeviceData::Pointer createDeviceData(const karabo::util::Hash& config) override;

            void initializeLoggerSpecific() override;

            void flushImpl(const boost::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr) override;

           private:
            void checkDb(bool connected);

            void onPingDb(const karabo::net::HttpResponse& o);

            void showDatabases(const InfluxResponseHandler& action);

            void onShowDatabases(const karabo::net::HttpResponse& o);

            void createDatabase(const InfluxResponseHandler& action);

            void onCreateDatabase(const karabo::net::HttpResponse& o);

           private:
            karabo::net::InfluxDbClient::Pointer m_clientRead;
            karabo::net::InfluxDbClient::Pointer m_clientWrite;
            const std::string m_dbName;
            std::string m_urlWrite;
            std::string m_urlQuery;
            int m_maxTimeAdvance;
            static const unsigned int k_httpResponseTimeoutMs;
        };
    } // namespace devices
} // namespace karabo

#endif /* KARABO_DEVICES_INFLUXDATALOGGER_HH */
