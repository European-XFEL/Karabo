/*
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

#ifndef KARABO_DEVICES_INFLUXDATALOGGER_HH
#define KARABO_DEVICES_INFLUXDATALOGGER_HH

#include <deque>
#include <fstream>
#include <karabo/net/HttpResponse.hh>
#include <karabo/net/InfluxDbClient.hh>
#include <unordered_map>

#include "DataLogger.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/Version.hh"

namespace karabo {

    namespace devices {

        class InfluxDataLogger;


        using AsyncHandler = std::function<void()>;
        using InfluxResponseHandler = std::function<void(const karabo::net::HttpResponse&)>;

        enum class RejectionType {
            TOO_MANY_ELEMENTS = 0, // Vector property values with more elements than allowed.
            VALUE_STRING_SIZE,     // Property values whose string form exceeds the maximum allowed.
            PROPERTY_WRITE_RATE,   // Writes that would exceed the maximum property logging rate allowed for a device.
            SCHEMA_WRITE_RATE,     // Writes that would exceed the maximum schema logging rate allowed for a device.
            FAR_AHEAD_TIME         // Property values whose timestamps are too much in the future.
        };

        struct RejectedData {
            RejectionType type;
            std::string dataPath; // ${deviceId} || ${deviceId}.${propertyPath} || ${deviceId}"::schema"
            std::string details;
        };

        struct InfluxDeviceData : public karabo::devices::DeviceData {
            KARABO_CLASSINFO(InfluxDeviceData, "InfluxDataLoggerDeviceData", "2.6")

            /**
             * @brief The size, in characters, and the epoch seconds of a device log entry saved to Influx.
             *
             * Used for calculating the logging rates associated to a device.
             */
            struct LoggingRecord {
                std::size_t sizeChars;
                karabo::util::Epochstamp epoch;
                LoggingRecord(std::size_t sz, karabo::util::Epochstamp t) : sizeChars(sz), epoch(t) {}
            };

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

            void terminateQuery(std::stringstream& query, const karabo::util::Timestamp& stamp,
                                std::vector<RejectedData>& rejectedPathReasons);

            void onCheckSchemaInDb(const karabo::util::Timestamp& stamp, const std::string& schDigest,
                                   const std::shared_ptr<std::vector<char>>& schemaArchive,
                                   const karabo::net::HttpResponse& o);

            void handleSchemaUpdated(const karabo::util::Schema& schema, const karabo::util::Timestamp& stamp) override;

            void stopLogging() override;

            /**
             * @brief Calculates what the value of the property logging rate of the device will be when the logging of
             * a value with a given size and a given timestamp is taken into account.
             *
             * @param prop The path of the property whose current logging rate will be evaluated.
             * @param currentStamp The current property update timestamp.
             * @param currentSize The size for the new data to be logged - this is used along with the other
             * records in the current log rating window to calculate the new value for the property logging rate.
             * @return The updated value of the property logging rate, in bytes/sec, taking the logging of the
             * value into account.
             */
            unsigned int newPropLogRate(const std::string& propPath, karabo::util::Epochstamp currentStamp,
                                        std::size_t currentSize);

            /**
             * @brief Calculates what the value of the schema logging rate of the device will be when the logging of
             * a schema with a given size is taken into account. As schemas currently don't have associated time
             * information, the current system time is used for all the timing references.
             *
             * @param schemaSize The size for the new schema to be logged - this is used along with the other
             * records in the current log rating window to calculate the new value for the schema logging rate.
             * @return The updated value of the schema logging rate, in bytes/sec, taking the logging of the
             * schema into account.
             */
            unsigned int newSchemaLogRate(std::size_t schemaSize);


            /**
             * @brief Logs a new schema into the corresponding device's __SCHEMA measurement.
             * It is assumed that the verification of the uniquiness of the device schema has already been verified
             * based on its digest.
             *
             * @param schemaDigest The digest (assumed unique) of the new schema to be saved.
             * @param schemaArchive The serialised schema to be saved
             *
             * @return true If the new schema has been successfuly submitted for logging.
             * @return false If the logging of the new schema has not been submitted for logging. Currently, this
             * happens if logging the new schema would be above the allowed schema logging rate threshold for a device.
             */
            bool logNewSchema(const std::string& schemaDigest, const std::vector<char>& schemaArchive);

            /**
             * @brief Logs the given set of rejected data in the __BAD__DATA__ measurement and to the Karabo log. To
             * avoid spanning of the Karabo log, log is emmitted for each device only once in a period of 30 secs.
             *
             * @param rejects The rejected data to be logged.
             * @param ts An epoch with the precision expected in the InfluxDb.
             */
            void logRejectedData(const std::vector<RejectedData>& rejects, unsigned long long ts);

            /**
             * @brief Logs the given rejected data record in the __BAD__DATA__ measurement and to the Karabo log. To
             * avoid spanning of the Karabo log, log is emmitted for each device only once in a period of 30 secs.
             *
             * @param rejects The rejected data to be logged.
             */
            void logRejectedDatum(const RejectedData& rejects);

            karabo::net::InfluxDbClient::Pointer m_dbClientRead;
            karabo::net::InfluxDbClient::Pointer m_dbClientWrite;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;

            int m_maxTimeAdvance;
            size_t m_maxVectorSize;
            size_t m_maxValueStringSize;
            unsigned long long m_secsOfLogOfRejectedData; // epoch seconds of last logging of rejected data

            unsigned int m_maxPropLogRateBytesSec; // in bytes/sec.
            unsigned int m_propLogRatePeriod;
            // Logging records for the device property in the current log rating window.
            std::unordered_map<std::string, std::deque<LoggingRecord>> m_propLogRecs;

            unsigned int m_maxSchemaLogRateBytesSec;
            unsigned int m_schemaLogRatePeriod;
            // Logging records for the device schema in the current log rating window.
            std::deque<LoggingRecord> m_schemaLogRecs;

            karabo::util::Timestamp m_loggingStartStamp;
            karabo::util::TimeDuration m_safeSchemaRetentionDuration;
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

            void flushImpl(const std::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr) override;

           private:
            void checkDb(bool connected);

            void onPingDb(const karabo::net::HttpResponse& o);

            void asyncCreateDbIfNeededAndStart();

            void onShowDatabases(const karabo::net::HttpResponse& o);

            void createDatabase(const InfluxResponseHandler& action);

            void onCreateDatabase(const karabo::net::HttpResponse& o);

           private:
            karabo::net::InfluxDbClient::Pointer m_clientRead;
            karabo::net::InfluxDbClient::Pointer m_clientWrite;
            const std::string m_dbName;
            std::string m_urlWrite;
            std::string m_urlQuery;
            static const unsigned int k_httpResponseTimeoutMs;
        };
    } // namespace devices
} // namespace karabo

#endif /* KARABO_DEVICES_INFLUXDATALOGGER_HH */
