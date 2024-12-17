/*
 * File:   InfluxLogReader.hh
 *
 * Created on November 4, 2019, 9:09 AM
 *
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


#ifndef INFLUXLOGREADER_HH
#define INFLUXLOGREADER_HH

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "DataLogReader.hh"
#include "karabo/core/Device.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/net/InfluxDbClient.hh"
#include "karabo/util/ClassInfo.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/TimeDuration.hh"
#include "karabo/util/Types.hh"
#include "karabo/util/Version.hh"
#include "karabo/xms/SignalSlotable.hh"

namespace karabo {

    namespace devices {

        // Context of an ongoing slotGetPropertyHistory process.
        struct PropertyHistoryContext {
            PropertyHistoryContext(const std::string& deviceId, const std::string& property,
                                   const karabo::util::Epochstamp& from, const karabo::util::Epochstamp& to,
                                   int maxDataPoints, const karabo::xms::SignalSlotable::AsyncReply& aReply,
                                   const karabo::net::InfluxDbClient::Pointer& influxClient);

            std::string deviceId;
            std::string property;
            karabo::util::Epochstamp from;
            karabo::util::Epochstamp to;
            unsigned int maxDataPoints;
            karabo::xms::SignalSlotable::AsyncReply aReply;
            karabo::net::InfluxDbClient::Pointer influxClient;

            // return sampling interval in microsecond
            double getInterval() const;
        };


        struct PropFromPastInfo {
            PropFromPastInfo(const std::string& name, const karabo::util::Types::ReferenceType type,
                             bool infiniteOrNan);

            std::string name;
            karabo::util::Types::ReferenceType type;
            bool infiniteOrNan;
        };


        // Context of an ongoing slotGetConfigurationFromPast process.
        struct ConfigFromPastContext {
            ConfigFromPastContext(const std::string& deviceId, const karabo::util::Epochstamp& atTime,
                                  const karabo::xms::SignalSlotable::AsyncReply& aReply,
                                  const karabo::net::InfluxDbClient::Pointer& influxClient);

            std::string deviceId;
            karabo::util::Epochstamp atTime;
            karabo::util::Epochstamp configTimePoint;
            unsigned long long lastLoginBeforeTime;
            unsigned long long lastLogoutBeforeTime;
            karabo::util::Schema configSchema;
            karabo::util::Hash configHash;
            // Log format version: version 1 introduces truncation of property
            // timestamps in the past - those past timestamp are replaced with
            // the timestamp of the start of the current lifetime of the device
            // (or the lifetime of the data logger that is logging the device).
            int logFormatVersion;

            // Properties to be returned in the past configuration.
            std::deque<PropFromPastInfo> propsInfo;

            karabo::xms::SignalSlotable::AsyncReply aReply;
            karabo::net::InfluxDbClient::Pointer influxClient;
        };


        class InfluxLogReader : public DataLogReader {
           public:
            KARABO_CLASSINFO(InfluxLogReader, "InfluxLogReader", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::util::Schema& expected);

            explicit InfluxLogReader(const karabo::util::Hash& cfg);

            ~InfluxLogReader();

           protected:
            void slotGetPropertyHistoryImpl(const std::string& deviceId, const std::string& property,
                                            const karabo::util::Hash& params) override;

            void slotGetConfigurationFromPastImpl(const std::string& deviceId, const std::string& timepoint) override;

           private:
            /**
             * Triggers the retrieval of the number of data points for a given device property during a
             * time interval.
             *
             * @param context
             */
            void asyncDataCountForProperty(const std::shared_ptr<PropertyHistoryContext>& context);

            /**
             * Handles the retrieval of the number of data points for an ongoing GetPropertyHistory
             * process. Responsible for invoking the appropriate async method for retrieving the
             * property values depending on the number of data points received.
             *
             * @param dataCountResponse
             * @param context
             */
            void onDataCountForProperty(const karabo::net::HttpResponse& dataCountResp,
                                        const std::shared_ptr<PropertyHistoryContext>& ctxt);

            /**
             * Triggers the retrieval of the property values in an ongoing GetPropertyHistory process.
             *
             * @param context
             */
            void asyncGetPropertyValues(const std::shared_ptr<PropertyHistoryContext>& ctxt);

            /**
             * Triggers the retrieval of the property values mean in an ongoing GetPropertyHistory process.
             * This is used when the number of available data points for the property is larger than the
             * maximum requested by the slot caller and all values are scalar numbers.
             * The UINT64 properties are included in this despite being reinterpreted as INT64 on the backend
             * and possibly returning incorrect data.
             *
             * @param context
             */
            void asyncGetPropertyValuesMean(const std::shared_ptr<PropertyHistoryContext>& ctxt);

            /**
             * Triggers the retrieval of the property values samples in an ongoing GetPropertyHistory process.
             * This is used when the number of available data points for the property is larger than the
             * maximum requested by the slot caller.
             *
             * @param context
             */
            void asyncGetPropertyValuesSamples(const std::shared_ptr<PropertyHistoryContext>& ctxt);

            /**
             * Handles the retrieval of the values of a property in an ongoing GetPropertyHistory
             * process. Responsible for transforming the json formatted values received from
             * InfluxDbClient into a vector of hashes suitable to be returned to the slot caller.
             * Also responsible for replying to the slot caller.
             *
             * @param valuesResp
             * @param columnPrefixToRemove
             * @param context
             */
            void onPropertyValues(const karabo::net::HttpResponse& valuesResp, const std::string& columnPrefixToRemove,
                                  const std::shared_ptr<PropertyHistoryContext>& ctxt);
            /**
             * Handles the retrieval of the values of a property in an ongoing GetPropertyHistory
             * process. Responsible for transforming the json formatted values received from
             * InfluxDbClient into a vector of hashes suitable to be returned to the slot caller.
             * This function extends the functionality of `onPropertyValues` while keeping the
             * property history protocol.
             *
             * Also responsible for replying to the slot caller.
             *
             * @param valuesResp
             * @param context
             */
            void onMeanPropertyValues(const karabo::net::HttpResponse& valuesResp,
                                      const std::shared_ptr<PropertyHistoryContext>& ctxt);

            void asyncLastLogoutBeforeTime(const std::shared_ptr<ConfigFromPastContext>& ctxt);
            void onLastLogoutBeforeTime(const karabo::net::HttpResponse& valueResp,
                                        const std::shared_ptr<ConfigFromPastContext>& ctxt);

            void asyncLastLoginFormatBeforeTime(const std::shared_ptr<ConfigFromPastContext>& ctxt);
            void onLastLoginFormatBeforeTime(const karabo::net::HttpResponse& valueResp,
                                             const std::shared_ptr<ConfigFromPastContext>& ctxt);

            void asyncLastSchemaDigestBeforeTime(const std::shared_ptr<ConfigFromPastContext>& ctxt);
            void onLastSchemaDigestBeforeTime(const karabo::net::HttpResponse& valueResp,
                                              const std::shared_ptr<ConfigFromPastContext>& ctxt);

            void asyncSchemaForDigest(const std::string& digest, const std::shared_ptr<ConfigFromPastContext>& ctxt);
            void onSchemaForDigest(const karabo::net::HttpResponse& schemaResp,
                                   const std::shared_ptr<ConfigFromPastContext>& ctxt, const std::string& digest);


            void asyncPropValueBeforeTime(const std::shared_ptr<ConfigFromPastContext>& ctxt);
            void onPropValueBeforeTime(const std::vector<PropFromPastInfo>& propInfos,
                                       const karabo::net::HttpResponse& propValueResp,
                                       const std::shared_ptr<ConfigFromPastContext>& ctxt);

            void slotGetBadData(const std::string& fromStr, const std::string& toStr);
            void onGetBadData(const karabo::net::HttpResponse& response, karabo::xms::SignalSlotable::AsyncReply aReply,
                              const karabo::net::InfluxDbClient::Pointer& /* influxClient */);

            void influxResultSetToVectorHash(const karabo::util::InfluxResultSet& influxResult,
                                             std::vector<karabo::util::Hash>& vectHash);

            void addNodeToHash(karabo::util::Hash& hash, const std::string& path,
                               const karabo::util::Types::ReferenceType& type, unsigned long long trainId,
                               const karabo::util::Epochstamp& epoch, const std::string& valueAsString);

            /**
             * Unescapes a logged string. A logged string has its new lines mangled, then its double slashes
             * escaped and then its double quotes escaped. This functions applies those transformations in the
             * reverse order.
             * @param loggedStr The string as it has been escaped by the Influx Logger.
             * @return The unescaped original string.
             */
            std::string unescapeLoggedString(const std::string& loggedStr);

            /**
             * Handles a given Http response whenever it indicates an error.
             *
             * In the InfluxDb client <-> server communication context, any response with a status
             * code greater or equal to 300 is considered an error and will be handled by this method.
             *
             * The error handling consists of sending the appropriate error reply to the caller of
             * the InfluxLogReader slot affected by the error and of optionally disconnecting the
             * InfluxDbClient used by the slot.
             *
             * @param httpResponse the response that potentially indicates an error.
             * @param asyncReply the reply to be sent to the caller of the slot where the error
             *                   happened.
             * @return true if the httpResponse indicated an error that has been handled. false if
             *              the httpResponse didn't indicate an error.
             */
            bool handleHttpResponseError(const karabo::net::HttpResponse& httpResponse,
                                         const karabo::xms::SignalSlotable::AsyncReply& asyncReply);

            /**
             * Convert a time point from influx to karabo Epochstamp
             */
            karabo::util::Epochstamp toEpoch(unsigned long long timeFromInflux) const;

            /**
             * Builds and returns the configuration Hash for instantiating an InfluxDbClient to
             * be used in the execution of one of the slots supported by the reader.
             *
             * @param dbUrlForSlot the URL to be used in the configuration - each slot can use a
             * different database URL.
             *
             * @returns the configuration Hash for the InfluxDbClient.
             */
            karabo::util::Hash buildInfluxClientConfig(const std::string& dbUrlForSlot) const;

            std::string m_dbName;
            std::string m_dbUser;
            std::string m_dbPassword;
            std::string m_durationUnit;
            std::string m_urlConfigSchema;
            std::string m_urlPropHistory;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_hashSerializer;
            karabo::io::BinarySerializer<karabo::util::Schema>::Pointer m_schemaSerializer;
            int m_maxHistorySize;

            static const unsigned long kFracConversionFactor;
            static const int kMaxHistorySize;
            // Maximum delay, in seconds, assumed for data written to Influx to be available for reading.
            static const karabo::util::TimeValue kMaxInfluxDataDelaySecs;
            const std::unordered_set<std::string> kNumberTypes;
        };


    } // namespace devices

} // namespace karabo

#endif /* INFLUXLOGREADER_HH */
