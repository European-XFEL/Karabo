/*
 * File:   InfluxLogReader.hh
 *
 * Created on November 4, 2019, 9:09 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef INFLUXLOGREADER_HH
#define	INFLUXLOGREADER_HH

#include <deque>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include "karabo/core/Device.hh"
#include "karabo/core/NoFsm.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/net/InfluxDbClient.hh"
#include "karabo/util/ClassInfo.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/Types.hh"
#include "karabo/xms/SignalSlotable.hh"

#include "DataLogReader.hh"

namespace karabo {

    namespace devices {

        // InfluxResultSet is a pair with a vector of column names in its first position and
        // a vector of rows of values represented as optional strings in its second position.
        // The optional strings have no value when they correspond to nulls returned by Influx.
        using InfluxResultSet =
                std::pair<
                /* first  */ std::vector<std::string>,
                /* second */ std::vector<std::vector<boost::optional<std::string>>>
                >;

        // Context of an ongoing slotGetPropertyHistory process.
        struct PropertyHistoryContext {
            PropertyHistoryContext(const std::string &deviceId,
                                   const std::string &property,
                                   const karabo::util::Epochstamp &from,
                                   const karabo::util::Epochstamp &to,
                                   int maxDataPoints,
                                   const karabo::xms::SignalSlotable::AsyncReply &aReply);

            std::string deviceId;
            std::string property;
            karabo::util::Epochstamp from;
            karabo::util::Epochstamp to;
            int maxDataPoints;
            karabo::xms::SignalSlotable::AsyncReply aReply;
        };


        // Context of an ongoing slotGetConfigurationFromPast process.
        struct ConfigFromPastContext {
            ConfigFromPastContext(const std::string &deviceId,
                                  const karabo::util::Epochstamp &atTime,
                                  const karabo::xms::SignalSlotable::AsyncReply &aReply);

            std::string deviceId;
            karabo::util::Epochstamp atTime;
            karabo::util::Epochstamp configTimePoint;
            unsigned long long lastLoginBeforeTime;
            unsigned long long lastLogoutBeforeTime;
            karabo::util::Schema configSchema;
            karabo::util::Hash configHash;

            // Pairs with the names and types of the properties that will be returned in the configuration.
            std::deque<std::pair<std::string, karabo::util::Types::ReferenceType>> propNamesAndTypes;

            karabo::xms::SignalSlotable::AsyncReply aReply;
        };


        class InfluxLogReader : public DataLogReader {

        public:

            KARABO_CLASSINFO(InfluxLogReader, "InfluxLogReader", "1.0")

            static void expectedParameters(karabo::util::Schema &expected);

            InfluxLogReader(const karabo::util::Hash &cfg);

            virtual ~InfluxLogReader();

        protected:

            virtual void slotGetPropertyHistoryImpl(const std::string& deviceId,
                                                    const std::string& property,
                                                    const karabo::util::Hash& params) override;

            virtual void slotGetConfigurationFromPastImpl(const std::string& deviceId,
                                                          const std::string& timepoint) override;

        private:

            /**
             * Triggers the retrieval of the number of data points for a given device property during a
             * time interval.
             *
             * @param context
             */
            void asyncDataCountForProperty(const boost::shared_ptr<PropertyHistoryContext> &context);

            /**
             * Handles the retrieval of the number of data points for an ongoing GetPropertyHistory
             * process. Responsible for invoking the appropriate async method for retrieving the
             * property values depending on the number of data points received.
             *
             * @param dataCountResponse
             * @param context
             */
            void onDataCountForProperty(const karabo::net::HttpResponse &dataCountResp,
                                        const boost::shared_ptr<PropertyHistoryContext> &ctxt);

            /**
             * Triggers the retrieval of the property values in an ongoing GetPropertyHistory process.
             *
             * @param context
             */
            void asyncGetPropertyValues(const boost::shared_ptr<PropertyHistoryContext> &ctxt);

            /**
             * Triggers the retrieval of the property values samples in an ongoing GetPropertyHistory process.
             * This is used when the number of available data points for the property is larger than the
             * maximum allowed be the slot caller.
             *
             * @param context
             */
            void asyncGetPropertyValuesSamples(const boost::shared_ptr<PropertyHistoryContext> &ctxt);

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
            void onPropertyValues(const karabo::net::HttpResponse &valuesResp,
                                  const std::string &columnPrefixToRemove,
                                  const boost::shared_ptr<PropertyHistoryContext> &ctxt);

            void asyncLastLogoutBeforeTime(const boost::shared_ptr<ConfigFromPastContext> &ctxt);
            void onLastLogoutBeforeTime(const karabo::net::HttpResponse &valueResp,
                                        const boost::shared_ptr<ConfigFromPastContext> &ctxt);

            void asyncLastLoginBeforeTime(const boost::shared_ptr<ConfigFromPastContext> &ctxt);
            void onLastLoginBeforeTime(const karabo::net::HttpResponse &valueResp,
                                       const boost::shared_ptr<ConfigFromPastContext> &ctxt);

            void asyncLastSchemaDigestBeforeTime(const boost::shared_ptr<ConfigFromPastContext> &ctxt);
            void onLastSchemaDigestBeforeTime(const karabo::net::HttpResponse &valueResp,
                                              const boost::shared_ptr<ConfigFromPastContext> &ctxt);

            void asyncSchemaForDigest(const std::string &digest,
                                      const boost::shared_ptr<ConfigFromPastContext> &ctxt);
            void onSchemaForDigest(const karabo::net::HttpResponse &schemaResp,
                                   const boost::shared_ptr<ConfigFromPastContext> &ctxt);

            void asyncPropValueBeforeTime(const boost::shared_ptr<ConfigFromPastContext> &ctxt);
            void onPropValueBeforeTime(const std::string &propName,
                                       const karabo::util::Types::ReferenceType &propType,
                                       const karabo::net::HttpResponse &propValueResp,
                                       const boost::shared_ptr<ConfigFromPastContext> &ctxt);

            std::string toInfluxDurationUnit(const karabo::util::TIME_UNITS &karaboDurationUnit);

            std::string epochAsMicrosecString(const karabo::util::Epochstamp &ep) const;

            void jsonResultsToInfluxResultSet(const std::string &jsonResult, InfluxResultSet &influxResult,
                                              const std::string &columnPrefixToRemove = "");

            void influxResultSetToVectorHash(const InfluxResultSet &influxResult,
                                             std::vector<karabo::util::Hash> &vectHash);

            void addNodeToHash(karabo::util::Hash &hash,
                               const std::string &path,
                               const karabo::util::Types::ReferenceType &type,
                               unsigned long long trainId,
                               const karabo::util::Epochstamp &epoch,
                               const std::string &valueAsString);

           /**
             * Unescapes a logged string. A logged string has its new lines mangled, then its double slashes
             * escaped and then its double quotes scaped. This functions applies those transformations in the
             * reverse order.
             * @param loggedStr The string as it has been escaped by the Influx Logger.
             * @return The unescaped original string.
             */
            std::string unescapeLoggedString(const std::string &loggedStr);

            /**
             * Handles a given Http response whenever it indicates an error.
             *
             * In the InfluxDb client <-> server communication context, any response with a status
             * code greater or equal to 300 is considered an error and will be handled by this method.
             *
             * The error handling consists of sending the appropriate error reply to the caller of
             * the InfluxLogReader slot affected by the error.
             *
             * @param httpResponse the response that potentially indicates an error.
             * @param asyncReply the reply to be sent to the caller of the slot where the error
             *                   happened.
             * @return true if the httpResponse indicated an error that has been handled. false if
             *              the httpResponse didn't indicate an error.
             */
            bool handleHttpResponseError(const karabo::net::HttpResponse &httpResponse,
                                         const karabo::xms::SignalSlotable::AsyncReply &asyncReply);

            karabo::net::InfluxDbClient::Pointer m_influxClient;
            std::string m_durationUnit;
            karabo::io::BinarySerializer<karabo::util::Schema>::Pointer m_schemaSerializer;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_hashSerializer;

            static const unsigned long kSecConversionFactor;
            static const unsigned long kFracConversionFactor;
        };


    } // namespace devices

} // namespace karabo

#endif	/* INFLUXLOGREADER_HH */

