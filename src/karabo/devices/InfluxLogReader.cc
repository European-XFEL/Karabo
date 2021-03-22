/*
 * File:   InfluxLogReader.cc
 *
 * Created on November 4, 2019, 9:09 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <algorithm>
#include <complex>
#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>

#include <nlohmann/json.hpp>

#include "karabo/net/EventLoop.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/util/Base64.hh"
#include "karabo/util/ClassInfo.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/DateTimeString.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/FromLiteral.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/OverwriteElement.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/StringTools.hh"
#include "karabo/util/TimeDuration.hh"
#include "karabo/util/Types.hh"

#include "InfluxLogReader.hh"

namespace karabo {

    namespace devices {

        using namespace krb_log4cpp;
        using namespace karabo::core;
        using namespace karabo::io;
        using namespace karabo::util;
        using namespace karabo::net;
        using namespace karabo::xms;

        namespace nl = nlohmann;

        PropertyHistoryContext::PropertyHistoryContext(const std::string &deviceId,
                                                       const std::string &property,
                                                       const karabo::util::Epochstamp &from,
                                                       const karabo::util::Epochstamp &to,
                                                       int maxDataPoints,
                                                       const karabo::xms::SignalSlotable::AsyncReply &aReply) :
            deviceId(deviceId), property(property),
            from(from), to(to), maxDataPoints(maxDataPoints),
            aReply(aReply) {
        };

        double PropertyHistoryContext::getInterval() const {
            const TimeDuration d = to - from;
            return std::round(static_cast<double>(d) / maxDataPoints * 1'000'000);
        }

        ConfigFromPastContext::ConfigFromPastContext(const std::string &deviceId,
                                                     const karabo::util::Epochstamp &atTime,
                                                     const karabo::xms::SignalSlotable::AsyncReply &aReply) :
            deviceId(deviceId), atTime(atTime), configTimePoint(Epochstamp(0, 0)), aReply(aReply) {
        };


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice,
                                          karabo::core::Device<karabo::core::OkErrorFsm>,
                                          DataLogReader,
                                          InfluxLogReader)

        const unsigned long InfluxLogReader::kSecConversionFactor = 1000000;
        const unsigned long InfluxLogReader::kFracConversionFactor = 1000000000000;
        const int InfluxLogReader::kMaxHistorySize = 10000;

        void InfluxLogReader::expectedParameters(karabo::util::Schema &expected) {

            STRING_ELEMENT(expected).key("url")
                    .displayedName("Influxdb URL")
                    .description("URL should be given in form: tcp://host:port")
                    .assignmentOptional().defaultValue("tcp://localhost:8086")
                    .commit();

            STRING_ELEMENT(expected).key("dbname")
                    .displayedName("Database name")
                    .description("Name of the database in which the data resides")
                    .assignmentMandatory()
                    .commit();

            INT32_ELEMENT(expected).key("maxHistorySize")
                    .displayedName("Max. Property History Size")
                    .description("Maximum value allowed for the 'maxNumData' parameter in a call to slot 'getPropertyHistory'.")
                    .assignmentOptional().defaultValue(kMaxHistorySize)
                    .init()
                    .commit();
        }


        InfluxLogReader::InfluxLogReader(const karabo::util::Hash & cfg) :
                karabo::devices::DataLogReader(cfg),
                m_hashSerializer(BinarySerializer<Hash>::create("Bin")),
                m_schemaSerializer(BinarySerializer<Schema>::create("Bin")),
                m_maxHistorySize(cfg.get<int>("maxHistorySize")),
                kNumberTypes({
                    Types::to<ToLiteral>(Types::INT8),
                    Types::to<ToLiteral>(Types::UINT8),
                    Types::to<ToLiteral>(Types::INT16),
                    Types::to<ToLiteral>(Types::UINT16),
                    Types::to<ToLiteral>(Types::INT32),
                    Types::to<ToLiteral>(Types::UINT32),
                    Types::to<ToLiteral>(Types::INT64),
                    // Warning! this is dangerous, arithmetic operators will be performed
                    // server side on the INT64 cast of the UINT64 value.
                    Types::to<ToLiteral>(Types::UINT64),
                    Types::to<ToLiteral>(Types::FLOAT),
                    Types::to<ToLiteral>(Types::DOUBLE)})
        {
            // TODO: Use more appropriate names for the env var names - the names below are the ones currently used by
            //       the CI environment.
            std::string dbUser;
            if (getenv("KARABO_INFLUXDB_QUERY_USER")) {
                dbUser = getenv("KARABO_INFLUXDB_QUERY_USER");
            } else {
                dbUser = "infadm";
            }
            std::string dbPassword;
            if (getenv("KARABO_INFLUXDB_QUERY_PASSWORD")) {
                dbPassword = getenv("KARABO_INFLUXDB_QUERY_PASSWORD");
            } else {
                dbPassword = "admpwd";
            }

            const std::string dbName(cfg.get<std::string>("dbname"));
            const std::string url(cfg.get<std::string>("url"));

            Hash dbClientCfg;
            dbClientCfg.set<std::string>("dbname", dbName);
            dbClientCfg.set<std::string>("url", url);
            dbClientCfg.set<std::string>("durationUnit", "u");
            dbClientCfg.set<std::string>("dbUser", dbUser);
            dbClientCfg.set<std::string>("dbPassword", dbPassword);
            m_influxClient = Configurator<InfluxDbClient>::create("InfluxDbClient", dbClientCfg);
            m_durationUnit = toInfluxDurationUnit(TIME_UNITS::MICROSEC);
        }


        InfluxLogReader::~InfluxLogReader() {
            KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " being destroyed.";
        }


        void InfluxLogReader::slotGetPropertyHistoryImpl(const std::string &deviceId,
                                                         const std::string &property,
                                                         const Hash &params) {
            Epochstamp from;
            if (params.has("from"))
                from = Epochstamp(params.get<std::string>("from"));
            Epochstamp to;
            if (params.has("to"))
                to = Epochstamp(params.get<std::string>("to"));
            int maxNumData = m_maxHistorySize;
            if (params.has("maxNumData"))
                maxNumData = params.get<int>("maxNumData");
            if (maxNumData == 0) {
                // 0 is interpreted as unlimited, but for the Influx case a limit is
                // always enforced.
                maxNumData = m_maxHistorySize;
            }

            if (maxNumData < 0 || maxNumData > m_maxHistorySize) {
                throw KARABO_PARAMETER_EXCEPTION("'maxNumData' parameter is intentionally limited to a maximum of '"
                                                 + karabo::util::toString(m_maxHistorySize) + "'. "
                                                 + "Property History polling is not designed for Scientific Data Analysis.");
            }

            // This prevents the slot from sending an automatic empty response at the end of
            // the slot method execution. Either a success reply or an error reply must be
            // sent exactly once from one of the other methods involved in the processing of
            // the slot call. A successful reply can be sent through the AsyncReply operator
            // () - AsyncReply is a functor. An error reply can be sent through AsyncReply
            // error method.
            SignalSlotable::AsyncReply aReply(this);

            auto ctxtPtr(boost::make_shared<PropertyHistoryContext>(deviceId, property,
                                                                    from, to,
                                                                    maxNumData, aReply));

            asyncDataCountForProperty(ctxtPtr);
        }


        void InfluxLogReader::asyncDataCountForProperty(const boost::shared_ptr<PropertyHistoryContext> &ctxt) {

            std::ostringstream iqlQuery;

            /* The query for data count, differently from the query for the property values (or samples) that will
               be executed later,  doesn't select the '_tid' field. The goal of this query is to count how many
               entries will exist in the property history and '_tid' field entries only make into the resulting
               property history as attributes of entries. */
            iqlQuery << "SELECT COUNT(/^" << ctxt->property << "-.*/) FROM \""
                    << ctxt->deviceId << "\" WHERE time >= " << epochAsMicrosecString(ctxt->from) << m_durationUnit
                    << " AND time <= " << epochAsMicrosecString(ctxt->to) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                        bind_weak(&InfluxLogReader::onDataCountForProperty, this, _1, ctxt));
            } catch (const std::exception &e) {
                const std::string &errMsg = std::string("Error querying data count for property: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                // As this is in the same thread at which the slot call started, if we send the async reply directly,
                // the reply will be sent, and then unregistered from the SignalSlotable. When this method execution
                // finishes soon after and the control returns to the SignalSlotable, it won't find any asynchronous
                // reply registered and will send the default empty reply, ignoring that a reply has already been sent.
                // That's the reason for posting the reply to the event loop instead of sending it directly. The
                // remaining calls to ctxt->aReply.error in the processing of the slot can be sent directly.
                boost::weak_ptr<karabo::xms::SignalSlotable> weakThis(weak_from_this());
                EventLoop::getIOService().post([weakThis, ctxt, errMsg]() {
                    // Only sends a reply if the InfluxLogReader instance is still alive - lock() call is successful.
                    boost::shared_ptr<karabo::xms::SignalSlotable> ptr = weakThis.lock();
                    if (ptr) {
                        ctxt->aReply.error(errMsg);
                    }
                });
            }
        }


        void InfluxLogReader::onDataCountForProperty(const karabo::net::HttpResponse& dataCountResp,
                                                     const boost::shared_ptr<PropertyHistoryContext> &ctxt) {

            bool errorHandled = handleHttpResponseError(dataCountResp, ctxt->aReply);

            if (errorHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }
            // The format of the data received is available here
            //  https://docs.influxdata.com/influxdb/v1.7/guides/querying_data/
            nl::json respObj = nl::json::parse(dataCountResp.payload);
            const auto &values = respObj["results"][0]["series"][0]["values"];
            int dataCount = 0;
            for (const auto &value : values) {
                auto countValue = value[1].get<int>();

                dataCount += countValue;
            }

            const auto &columns = respObj["results"][0]["series"][0]["columns"];
            bool allNumbers = true; // check if all fields support statistical aggregators
            for (const auto &column : columns) {
                const std::string& columnStr = column.get<std::string>();
                const std::string::size_type typeSeparatorPos = columnStr.rfind("-");
                if (typeSeparatorPos != std::string::npos) {
                    const std::string typeName = columnStr.substr(typeSeparatorPos + 1);
                    if (kNumberTypes.find(typeName) != kNumberTypes.end()){
                        continue;
                    } else {
                        allNumbers = false;
                        break;
                    }
                } else {
                    allNumbers = false;
                    KARABO_LOG_FRAMEWORK_ERROR << "Query for property '" << ctxt->deviceId << "." << ctxt->property << "'"
                                               << " returned column without type seperator '" << column << "'";
                }
            }
            if (dataCount < 1) {
                // No data point for the given period.
                ctxt->aReply(ctxt->deviceId, ctxt->property, std::vector<Hash>());
            } else if (dataCount <= ctxt->maxDataPoints) {
                asyncGetPropertyValues(ctxt);
            } else if (allNumbers){ // group by mean
                asyncGetPropertyValuesMean(ctxt);
            } else { // sample down
                asyncGetPropertyValuesSamples(ctxt);
            }
        }


        void InfluxLogReader::asyncGetPropertyValues(const boost::shared_ptr<PropertyHistoryContext> &ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << "SELECT /^" << ctxt->property << "-.*|_tid/ FROM \""
                    << ctxt->deviceId << "\" WHERE time >= " << epochAsMicrosecString(ctxt->from) << m_durationUnit
                    << " AND time <= " << epochAsMicrosecString(ctxt->to) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                        bind_weak(&InfluxLogReader::onPropertyValues, this, _1, "", ctxt));
            } catch (const std::exception &e) {
                const std::string &errMsg = std::string("Error querying property values: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }
        }

        void InfluxLogReader::asyncGetPropertyValuesSamples(const boost::shared_ptr<PropertyHistoryContext> &ctxt) {

            std::ostringstream iqlQuery;

            iqlQuery << "SELECT SAMPLE(/^" << ctxt->property << "-.*/, " << ctxt->maxDataPoints << ") FROM \""
                    << ctxt->deviceId << "\" WHERE time >= " << epochAsMicrosecString(ctxt->from) << m_durationUnit
                    << " AND time <= " << epochAsMicrosecString(ctxt->to) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                        bind_weak(&InfluxLogReader::onPropertyValues, this, _1, "sample_", ctxt));
            } catch (const std::exception &e) {
                const std::string &errMsg = std::string("Error querying property values samples: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onPropertyValues(const karabo::net::HttpResponse &valuesResp,
                                               const std::string &columnPrefixToRemove,
                                               const boost::shared_ptr<PropertyHistoryContext> &ctxt) {

            bool errorHandled = handleHttpResponseError(valuesResp, ctxt->aReply);

            if (errorHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            try {
                InfluxResultSet influxResult;
                jsonResultsToInfluxResultSet(valuesResp.payload, influxResult, columnPrefixToRemove);

                std::vector<Hash> propValues;
                influxResultSetToVectorHash(influxResult, propValues);

                ctxt->aReply(ctxt->deviceId, ctxt->property, propValues);

            } catch (const std::exception &e) {
                std::ostringstream oss;
                oss << "Error retrieving values of property '"
                        << ctxt->property << "' of device '" << ctxt->deviceId << "' between '"
                        << ctxt->from.toIso8601Ext() << "' and '" << ctxt->to.toIso8601Ext() << "':\n"
                        << e.what();
                const std::string &errMsg = oss.str();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }
        }

        void InfluxLogReader::asyncGetPropertyValuesMean(const boost::shared_ptr<PropertyHistoryContext> &ctxt) {

            std::ostringstream iqlQuery;
            iqlQuery << "SELECT MEAN(/^" << ctxt->property << "-.*/) FROM \""
                    << ctxt->deviceId << "\" WHERE time >= " <<  epochAsMicrosecString(ctxt->from) << m_durationUnit
                    << " AND time <= " << epochAsMicrosecString(ctxt->to) << m_durationUnit
                    << " GROUP BY time(" << toString(ctxt->getInterval()) << m_durationUnit << ") fill(none)";

            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                        bind_weak(&InfluxLogReader::onMeanPropertyValues, this, _1, ctxt));
            } catch (const std::exception &e) {
                const std::string &errMsg = std::string("Error querying property values samples: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }
        }



        void InfluxLogReader::onMeanPropertyValues(const karabo::net::HttpResponse &valuesResp,
                                                   const boost::shared_ptr<PropertyHistoryContext> &ctxt) {

            bool errorHandled = handleHttpResponseError(valuesResp, ctxt->aReply);

            if (errorHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            try {
                InfluxResultSet influxResult;
                jsonResultsToInfluxResultSet(valuesResp.payload, influxResult, "");

                std::vector<Hash> propValues;
                propValues.reserve(influxResult.second.size());
                // Converts each row of values into a Hash.
                // Gets the data type names of each column.
                std::vector<std::string>colTypeNames(influxResult.first.size());
                std::vector<std::string>colKeyNames(influxResult.first.size());
                for (size_t col = 0; col < influxResult.first.size(); col++) {
                    const std::string::size_type typeSeparatorPos = influxResult.first[col].rfind("-");
                    if (typeSeparatorPos != std::string::npos) {
                        const std::string typeName = influxResult.first[col].substr(typeSeparatorPos + 1);
                        colTypeNames[col] = typeName;
                    }
                    colKeyNames[col] = "v"; // the mean value will passed on in the key "v" to match the protocol
                }
                for (const std::vector<boost::optional < std::string>> &valuesRow : influxResult.second) {
                    unsigned long long time = std::stoull(*(valuesRow[0])); // This is safe as Influx always returns time.
                    Epochstamp epoch(time / kSecConversionFactor, (time % kSecConversionFactor) * kFracConversionFactor);
                    Hash hash;
                    for (size_t col = 1; col < influxResult.first.size(); col++) {
                        if (!valuesRow[col]) {
                            // Skips any null value in the result set - any row returned by Influx will have at least
                            // one non null value (may be an empty string).
                            continue;
                        }
                        // For nan/inf floating points we added "_INF" we skip.
                        const std::string& typeNameInflux = colTypeNames[col];
                        const Types::ReferenceType type = Types::from<FromLiteral>(typeNameInflux);
                        addNodeToHash(hash, colKeyNames[col], type, 0ull, epoch, *(valuesRow[col]));
                        // skip further columns. In the rare case of schema evolution in the same interval
                        // we take the first one reported. Multiple entries on the same timestamp will be an issue.
                        continue;
                    }
                    if (hash.has("v")) {
                        // TODO: the timestamp is the beginning of the interval group.
                        //       we should add half the time interval to center the time interval
                        //       the last interval should be half of the beginning of the interval and the
                        //       end of the query (ctxt->to).
                        // https://docs.influxdata.com/influxdb/v1.8/query_language/explore-data/#the-group-by-clause
                        propValues.push_back(std::move(hash));
                    }
                }

                ctxt->aReply(ctxt->deviceId, ctxt->property, propValues);

            } catch (const std::exception &e) {
                std::ostringstream oss;
                oss << "Error retrieving values of property '"
                        << ctxt->property << "' of device '" << ctxt->deviceId << "' between '"
                        << ctxt->from.toIso8601Ext() << "' and '" << ctxt->to.toIso8601Ext() << "':\n"
                        << e.what();
                const std::string &errMsg = oss.str();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }
        }

        void InfluxLogReader::slotGetConfigurationFromPastImpl(const std::string &deviceId,
                                                               const std::string &timepoint) {

            Epochstamp atTime(timepoint);
            SignalSlotable::AsyncReply aReply(this);

            ConfigFromPastContext ctxt(deviceId, atTime, aReply);

            asyncLastLoginBeforeTime(boost::make_shared<ConfigFromPastContext>(ctxt));
        }


        void InfluxLogReader::asyncLastLoginBeforeTime(const boost::shared_ptr<ConfigFromPastContext> &ctxt) {

            std::ostringstream iqlQuery;

            iqlQuery << "SELECT LAST(karabo_user) FROM \""
                    << ctxt->deviceId << "__EVENTS\" WHERE \"type\" = '\"+LOG\"' AND time <= "
                    << epochAsMicrosecString(ctxt->atTime) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                        bind_weak(&InfluxLogReader::onLastLoginBeforeTime, this, _1, ctxt));
            } catch (const std::exception &e) {
                const std::string &errMsg = std::string("Error querying last login before time: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                boost::weak_ptr<karabo::xms::SignalSlotable> weakThis(weak_from_this());
                EventLoop::getIOService().post([weakThis, ctxt, errMsg]() {
                    // Only sends a reply if the InfluxLogReader instance is still alive - lock() call is successful.
                    boost::shared_ptr<karabo::xms::SignalSlotable> ptr = weakThis.lock();
                    if (ptr) {
                        ctxt->aReply.error(errMsg);
                    }
                });
            }
        }


        void InfluxLogReader::onLastLoginBeforeTime(const karabo::net::HttpResponse &valueResp,
                                                    const boost::shared_ptr<ConfigFromPastContext> &ctxt) {

            bool errorHandled = handleHttpResponseError(valueResp, ctxt->aReply);

            if (errorHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            unsigned long long lastLoginBeforeTime = 0UL;

            nl::json respObj = nl::json::parse(valueResp.payload);
            auto value = respObj["results"][0]["series"][0]["values"][0][0];
            if (!value.is_null()) {
                // Db has a Login event before time.
                lastLoginBeforeTime = value.get<unsigned long long>();
            }
            ctxt->lastLoginBeforeTime = lastLoginBeforeTime;

            asyncLastLogoutBeforeTime(ctxt);
        }


        void InfluxLogReader::asyncLastLogoutBeforeTime(const boost::shared_ptr<ConfigFromPastContext> &ctxt) {

            std::ostringstream iqlQuery;

            iqlQuery << "SELECT LAST(karabo_user) FROM \""
                    << ctxt->deviceId << "__EVENTS\" WHERE \"type\" = '\"-LOG\"' AND time <= "
                    << epochAsMicrosecString(ctxt->atTime) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                        bind_weak(&InfluxLogReader::onLastLogoutBeforeTime, this, _1, ctxt));
            } catch (const std::exception &e) {
                const std::string &errMsg = std::string("Error querying last logout before time: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onLastLogoutBeforeTime(const karabo::net::HttpResponse &valueResp,
                                                     const boost::shared_ptr<ConfigFromPastContext> &ctxt) {

            bool errorHandled = handleHttpResponseError(valueResp, ctxt->aReply);

            if (errorHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            unsigned long long lastLogoutBeforeTime = 0UL;

            nl::json respObj = nl::json::parse(valueResp.payload);
            auto value = respObj["results"][0]["series"][0]["values"][0][0];
            if (!value.is_null()) {
                // Db has a Logout event before time.
                lastLogoutBeforeTime = value.get<unsigned long long>();
            }
            ctxt->lastLogoutBeforeTime = lastLogoutBeforeTime;

            asyncLastSchemaDigestBeforeTime(ctxt);
        }


        void InfluxLogReader::asyncLastSchemaDigestBeforeTime(const boost::shared_ptr<ConfigFromPastContext> &ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << "SELECT LAST(schema_digest) FROM \""
                    << ctxt->deviceId << "__EVENTS\" WHERE \"type\" = '\"SCHEMA\"' AND time <= "
                    << epochAsMicrosecString(ctxt->atTime) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                        bind_weak(&InfluxLogReader::onLastSchemaDigestBeforeTime, this, _1, ctxt));
            } catch (const std::exception &e) {
                const std::string errMsg = std::string("Error querying last schema digest before time: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }

        }


        void InfluxLogReader::onLastSchemaDigestBeforeTime(const karabo::net::HttpResponse &valueResp,
                                                           const boost::shared_ptr<ConfigFromPastContext> &ctxt) {

            bool errorHandled = handleHttpResponseError(valueResp, ctxt->aReply);

            if (errorHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            // If no digest is found returns an empty config and schema from this point.
            // Otherwise proceeds to schema retrieval.
            nl::json respObj = nl::json::parse(valueResp.payload);
            auto value = respObj["results"][0]["series"][0]["values"][0][1];
            if (value.is_null()) {
                // No digest has been found - it's not possible to go ahead.
                ctxt->aReply.error("Failed to query schema digest");
            } else {
                const std::string digest = value.get<std::string>();
                asyncSchemaForDigest(digest, ctxt);
            }
        }


        void InfluxLogReader::asyncSchemaForDigest(const std::string &digest,
                                                   const boost::shared_ptr<ConfigFromPastContext> &ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << "SELECT LAST(schema) FROM \""
                    << ctxt->deviceId << "__SCHEMAS\" WHERE \"digest\" = '\"" << digest << "\"' ";

            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                        bind_weak(&InfluxLogReader::onSchemaForDigest, this, _1, ctxt));
            } catch (const std::exception &e) {
                const std::string &errMsg = std::string("Error querying schema for digest: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }

        }


        void InfluxLogReader::onSchemaForDigest(const karabo::net::HttpResponse &schemaResp,
                                                const boost::shared_ptr<ConfigFromPastContext> &ctxt) {

            bool errorHandled = handleHttpResponseError(schemaResp, ctxt->aReply);

            if (errorHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            nl::json respObj = nl::json::parse(schemaResp.payload);
            const auto &value = respObj["results"][0]["series"][0]["values"][0][1];
            if (value.is_null()) {
                // No schema corresponding to the digest has been found - it's not possible to go ahead.
                ctxt->aReply.error("Failed to query schema");
            } else {
                // A schema has been found - processing it means base64 decoding it, deserializing it and
                // then traverse it capturing all the properties keys and their types for further processing.
                try {
                    const std::string encodedSch = value.get<std::string>();
                    std::vector<unsigned char> base64Decoded;
                    base64Decode(encodedSch, base64Decoded);
                    const char* decoded = reinterpret_cast<const char *> (base64Decoded.data());
                    m_schemaSerializer->load(ctxt->configSchema, decoded, base64Decoded.size());
                    const Schema &schema = ctxt->configSchema;

                    // Stores the properties keys and types in the context.
                    ctxt->propNamesAndTypes.clear();
                    std::vector<std::string> schPaths = schema.getDeepPaths();
                    for (const std::string &path : schPaths) {
                        if (schema.isLeaf(path) &&
                            !(schema.hasArchivePolicy(path) && schema.getArchivePolicy(path) == Schema::NO_ARCHIVING)) {
                            // Current path is for a leaf node that set is to archive (more literally, not set to not
                            // archive).
                            const Types::ReferenceType valType = schema.getValueType(path);
                            ctxt->propNamesAndTypes.push_back(make_pair(path, valType));
                        }
                    }

                    // Triggers the sequence of configuration value retrievals. The configuration
                    // values retrievals are an interplay between asyncPropValueBeforeTime and
                    // onAsyncPropValueBeforeTime - they will both consume the propNamesAndTypes
                    // vector, sending a response back to the slotGetConfigurationFromPast caller
                    // when the last property value is retrieved.
                    asyncPropValueBeforeTime(ctxt, false);

                } catch (const std::exception &e) {
                    std::ostringstream oss;
                    oss << "Error retrieving schema for digest while getting configuration of device '"
                            << ctxt->deviceId << "' at '" << ctxt->atTime.toIso8601Ext()
                            << "':\n" << e.what()
                            << "Encoded schema had '" << value.get<std::string>().size() << "' bytes.";
                    const std::string &errMsg = oss.str();
                    KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                    ctxt->aReply.error(errMsg);
                }
            }
        }


        void InfluxLogReader::asyncPropValueBeforeTime(const boost::shared_ptr<ConfigFromPastContext> &ctxt, bool infinite) {
            const auto nameAndType = ctxt->propNamesAndTypes.front();
            // A 'SELECT LAST(/^dProp-DOUBLE|dProp-DOUBLE_INF/) ...' query returns the last values of both fields,
            // but with a zero timestamp! So we have to request individually both. We start with the 'normal' field.
            std::string fieldKey = nameAndType.first + "-" + Types::to<ToLiteral>(nameAndType.second);
            if (infinite) {
                // query the special field for nan and inf (only for DOUBLE or FLOAT)
                fieldKey += "_INF";
            }
            if (infinite || (nameAndType.second != Types::FLOAT && nameAndType.second != Types::DOUBLE)) {
                // 'infinite' marks the 2nd query for a floating point variable, for normal types there is only one query:
                // Drop - i.e. pop - from list of properties still to query!
                ctxt->propNamesAndTypes.pop_front();
            }
            std::ostringstream iqlQuery;
            iqlQuery << "SELECT LAST(\"" << fieldKey << "\") FROM \""
                    << ctxt->deviceId << "\" WHERE time <= " << epochAsMicrosecString(ctxt->atTime) << m_durationUnit;
            const std::string queryStr = iqlQuery.str();

            try {
                m_influxClient->queryDb(queryStr,
                                    bind_weak(&InfluxLogReader::onPropValueBeforeTime,
                                              this, nameAndType.first, nameAndType.second, infinite, _1, ctxt));
            } catch (const std::exception &e) {
                const std::string &errMsg = std::string("Error querying property value before time: ") + e.what();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onPropValueBeforeTime(const std::string &propName,
                                                    const karabo::util::Types::ReferenceType &propType,
                                                    bool infinite,
                                                    const karabo::net::HttpResponse &propValueResp,
                                                    const boost::shared_ptr<ConfigFromPastContext> &ctxt) {

            bool errorHandled = handleHttpResponseError(propValueResp, ctxt->aReply);

            if (errorHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            nl::json respObj = nl::json::parse(propValueResp.payload);
            const auto &value = respObj["results"][0]["series"][0]["values"][0][1];
            if (!value.is_null()) {
                auto timeObj = respObj["results"][0]["series"][0]["values"][0][0];
                unsigned long long time = timeObj.get<unsigned long long>();
                unsigned long long timeSec = time / kSecConversionFactor;
                unsigned long long timeFrac = (time % kSecConversionFactor) * kFracConversionFactor;
                Epochstamp timeEpoch(timeSec, timeFrac);
                if (timeEpoch > ctxt->configTimePoint) {
                    ctxt->configTimePoint = timeEpoch;
                }
                const boost::optional<std::string> valueAsString = jsonValueAsString(value);
                try {
                    if (valueAsString) {
                        if (!ctxt->configHash.has(propName)) {
                            // The normal case - the result is not yet there
                            addNodeToHash(ctxt->configHash, propName, propType, 0, timeEpoch, *valueAsString);
                            if (propType == Types::DOUBLE || propType == Types::FLOAT) {
                                // Query again query for the special field for nan and inf
                                asyncPropValueBeforeTime(ctxt, true);
                                return;
                            }
                        } else {
                            // Second query for field for nan and inf of FLOAT and DOUBLE
                            const Epochstamp stampQuery1(Epochstamp::fromHashAttributes(ctxt->configHash.getAttributes(propName)));
                            if (stampQuery1 < timeEpoch) {
                                // This (i.e. the 2nd query) has more recent result
                                addNodeToHash(ctxt->configHash, propName, propType, 0, timeEpoch, *valueAsString);
                            }
                        }
                    }
                } catch (const std::exception &e) {
                    // Do not bail out, but just go on with other properties (Is that the correct approach?)
                    KARABO_LOG_FRAMEWORK_ERROR << "Error adding node to hash:"
                            << "\nValue type: " << propType
                            << "\nValue (as string): " << *valueAsString
                            << "\nTimestamp: " << timeEpoch.toIso8601Ext()
                            << "\nError: " << e.what();
                }
            } else {
                // No value means the query returns "empty" result ...
                // FLOAT and DOUBLE should be tested for nan and +-inf if not tested yet...
                // ... if tested already (infinite==true) then just skip and go for the next parameter
                if (!infinite && (propType == Types::DOUBLE || propType == Types::FLOAT)) {
                    asyncPropValueBeforeTime(ctxt, true);
                    return;
                }
            }

            if (ctxt->propNamesAndTypes.size() > 0) {
                // There is at least one more property whose value should be retrieved.
                asyncPropValueBeforeTime(ctxt, false);
            } else {
                // All properties have been retrieved. Reply to the slot caller.
                bool configAtTimePoint = ctxt->lastLogoutBeforeTime < ctxt->lastLoginBeforeTime;
                ctxt->aReply(ctxt->configHash, ctxt->configSchema, configAtTimePoint, ctxt->configTimePoint.toIso8601Ext());
            }
        }


        std::string InfluxLogReader::unescapeLoggedString(const std::string &loggedStr) {
            std::string unescaped = boost::replace_all_copy(loggedStr, "\\\"", "\"");
            boost::replace_all(unescaped, "\\\\", "\\");
            boost::replace_all(unescaped, DATALOG_NEWLINE_MANGLE, "\n");
            return unescaped;
        }

        void InfluxLogReader::influxResultSetToVectorHash(const InfluxResultSet &influxResult,
                                                          std::vector<Hash> &vectHash)
        {
            // Finds the position of the trainId column, if it is in the result set.
            int tidCol = -1;
            auto iter = std::find(influxResult.first.begin(), influxResult.first.end(), "_tid");
            if (iter != influxResult.first.end()) {
                tidCol = std::distance(influxResult.first.begin(), iter);
            }

            // Gets the data type names of each column.
            std::vector<std::string>colTypeNames(influxResult.first.size());
            for (size_t col = 0; col < influxResult.first.size(); col++) {
                const std::string::size_type typeSeparatorPos = influxResult.first[col].rfind("-");
                if (typeSeparatorPos != std::string::npos) {
                    const std::string typeName = influxResult.first[col].substr(typeSeparatorPos + 1);
                    colTypeNames[col] = typeName;
                }
            }

            vectHash.reserve(influxResult.second.size());
            // Converts each row of values into a Hash.
            for (const std::vector<boost::optional < std::string>> &valuesRow : influxResult.second) {
                unsigned long long time = std::stoull(*(valuesRow[0])); // This is safe as Influx always returns time.
                Epochstamp epoch(time / kSecConversionFactor, (time % kSecConversionFactor) * kFracConversionFactor);
                unsigned long long tid = 0;
                if (tidCol >= 0) {
                    if (valuesRow[tidCol]) {
                        tid = std::stoull(*(valuesRow[tidCol]));
                    } else {
                        KARABO_LOG_FRAMEWORK_WARN << "Missing train_id (_tid) for property at timestamp '"
                                << time << "'. '0' will be used for the train_id value.";
                    }
                }
                Hash hash;
                for (size_t col = 1; col < influxResult.first.size(); col++) {
                    if (static_cast<int> (col) == tidCol) continue; // Skips the trainId column
                    try {
                        if (!valuesRow[col]) {
                            // Skips any null value in the result set - any row returned by Influx will have at least
                            // one non null value (may be an empty string).
                            continue;
                        }
                        // Figure out the real Karabo type:
                        // For nan/inf floating points we added "_INF" when writing to influxDB (and stored as strings).
                        const std::string& typeNameInflux = colTypeNames[col];
                        const size_t posInf = typeNameInflux.rfind("_INF");
                        const std::string& typeName = (posInf != std::string::npos && posInf == typeNameInflux.size() - 4ul
                                                       ? typeNameInflux.substr(0, posInf)
                                                       : typeNameInflux);

                        const Types::ReferenceType type = Types::from<FromLiteral>(typeName);
                        addNodeToHash(hash, "v", type, tid, epoch, *(valuesRow[col]));
                    } catch (const std::exception &e) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Error adding node to hash:"
                                << "\nValue type: " << colTypeNames[col]
                                << "\nValue (as string): " << *(valuesRow[col])
                                << "\nTimestamp: " << epoch.toIso8601Ext()
                                << "\nError: " << e.what();
                    }
                }
                if (hash.has("v")) {
                    vectHash.push_back(std::move(hash));
                }
            }
        }

        void InfluxLogReader::addNodeToHash(karabo::util::Hash& hash,
                                            const std::string& path,
                                            const karabo::util::Types::ReferenceType& type,
                                            unsigned long long trainId,
                                            const karabo::util::Epochstamp& epoch,
                                            const std::string& valueAsString) {
            Hash::Node *node = nullptr;

            switch (type) {
                case Types::VECTOR_HASH:
                {
                    // Vectors of Hashes are binary serialized and then base64 encoded by the Influx Logger.
                    std::vector<unsigned char> base64Decoded;
                    base64Decode(valueAsString, base64Decoded);
                    const char* decoded = reinterpret_cast<const char *> (base64Decoded.data());
                    node = &hash.set(path, std::vector<Hash>());
                    std::vector<Hash> &value = node->getValue<std::vector < Hash >> ();
                    m_hashSerializer->load(value, decoded, base64Decoded.size());
                    break;
                }
                case Types::VECTOR_STRING:
                {
                    // Convert value from base64 -> JSON -> vector<string> ...
                    node = &hash.set(path, std::vector<std::string>());
                    std::vector<std::string>& value = node->getValue<std::vector<std::string> >();
                    std::vector<unsigned char> decoded;
                    base64Decode(valueAsString, decoded);
                    nl::json j = nl::json::parse(decoded.begin(), decoded.end());
                    for (nl::json::iterator ii = j.begin(); ii != j.end(); ++ii) {
                        value.push_back(*ii);
                    }
                    break;
                }
                case Types::VECTOR_CHAR:
                {
                    node = &hash.set(path, std::vector<char>());
                    std::vector<char> &value = node->getValue<std::vector<char>> ();
                    base64Decode(valueAsString, *reinterpret_cast<std::vector<unsigned char>*>(&value));
                    break;
                }
                case Types::CHAR:
                {
                    std::vector<unsigned char> decoded;
                    base64Decode(valueAsString, decoded);
                    if (decoded.size() != 1ul) {
                       throw KARABO_PARAMETER_EXCEPTION("Base64 Encoded char of wrong size: " + decoded.size());
                    }
                    node = &hash.set(path, static_cast<char>(decoded[0]));
                    break;
                }
                case Types::VECTOR_UINT8:
                {
                    // The fromString specialisation for vector<unsigned char> as used in the HANDLE_VECTOR_TYPE below
                    // erroneously does base64 decoding. We do not dare to fix that now, but workaround it here:
                    node = &hash.set(path, std::vector<unsigned char>());
                    node->getValue<std::vector<unsigned char>>() = fromStringForSchemaOptions<unsigned char>(valueAsString, ",");
                    break;
                }
#define HANDLE_VECTOR_TYPE(VectorType, ElementType) \
                case Types::VectorType: \
                { \
                    node = &hash.set(path, std::vector<ElementType>()); \
                    std::vector<ElementType> &value = node->getValue<std::vector<ElementType>>(); \
                    if (!valueAsString.empty()) { \
                        value = std::move(fromString<ElementType, std::vector>(valueAsString, ",")); \
                    } \
                    break; \
                }

                HANDLE_VECTOR_TYPE(VECTOR_BOOL, bool);
                HANDLE_VECTOR_TYPE(VECTOR_INT8, signed char);
                HANDLE_VECTOR_TYPE(VECTOR_INT16, short);
                HANDLE_VECTOR_TYPE(VECTOR_UINT16, unsigned short);
                HANDLE_VECTOR_TYPE(VECTOR_INT32, int);
                HANDLE_VECTOR_TYPE(VECTOR_UINT32, unsigned int);
                HANDLE_VECTOR_TYPE(VECTOR_INT64, long long);
                HANDLE_VECTOR_TYPE(VECTOR_UINT64, unsigned long long);
                HANDLE_VECTOR_TYPE(VECTOR_FLOAT, float);
                HANDLE_VECTOR_TYPE(VECTOR_DOUBLE, double);
                HANDLE_VECTOR_TYPE(VECTOR_COMPLEX_FLOAT, std::complex<float>);
                HANDLE_VECTOR_TYPE(VECTOR_COMPLEX_DOUBLE, std::complex<double>);
#undef HANDLE_VECTOR_TYPE
                case Types::STRING:
                {
                    std::string unescaped = unescapeLoggedString(valueAsString);
                    node = &hash.set<std::string>(path, unescaped);
                    break;
                }
                case Types::UINT64:
                {
                    // behavior on simple casting is implementation defined. We memcpy instead to be sure of the results
                    unsigned long long uv;
                    long long sv = std::move(fromString<long long>(valueAsString));
                    memcpy(&uv, &sv, sizeof(unsigned long long));
                    node = &hash.set<unsigned long long>(path, uv);
                    node->setType(type);
                }
                default:
                {
                    node = &hash.set<std::string>(path, valueAsString);
                    node->setType(type);
                }
            }
            Hash::Attributes &attrs = node->getAttributes();
            Timestamp(epoch, trainId).toHashAttributes(attrs);
        }


        std::string InfluxLogReader::toInfluxDurationUnit(const TIME_UNITS &karaboDurationUnit) {
            std::string influxDU;

            switch (karaboDurationUnit) {
                case TIME_UNITS::DAY:
                    influxDU = "d";
                    break;
                case TIME_UNITS::HOUR:
                    influxDU = "h";
                    break;
                case TIME_UNITS::MINUTE:
                    influxDU = "m";
                    break;
                case TIME_UNITS::SECOND:
                    influxDU = "s";
                    break;
                case TIME_UNITS::MILLISEC:
                    influxDU = "ms";
                    break;
                case TIME_UNITS::MICROSEC:
                    influxDU = "u";
                    break;
                case TIME_UNITS::NANOSEC:
                    influxDU = "ns";
                    break;
                default:
                    std::ostringstream errMsg;
                    errMsg << "There's no InfluxDb duration corresponding to Karabo's TIME_UNITS '"
                            << karaboDurationUnit << "'.";
                    throw KARABO_PARAMETER_EXCEPTION(errMsg.str());
            }

            return influxDU;
        }


        std::string InfluxLogReader::epochAsMicrosecString(const Epochstamp &ep) const {

            std::ostringstream epStr;
            const std::string fract(DateTimeString::fractionalSecondToString(TIME_UNITS::MICROSEC,
                                                                             ep.getFractionalSeconds(), true));
            // It is safe to use fract because fractionalSecondToString fills the leading positions with zeros.
            epStr << ep.getSeconds() << fract;
            return epStr.str();
        }


        bool InfluxLogReader::handleHttpResponseError(const karabo::net::HttpResponse &httpResponse,
                                                      const karabo::xms::SignalSlotable::AsyncReply &asyncReply) {

            bool errorHandled = false;

            if (httpResponse.code >= 300) {
                // Some error happened while processing the request.
                std::ostringstream errMsg;
                errMsg << "InfluxDb response status code: " << httpResponse.code << ". ";
                if (httpResponse.payload.empty() && httpResponse.message.empty()) {
                    errMsg << "Description: Could not process request.";
                } else {
                    errMsg << "Response payload: " << httpResponse.payload
                    << "\nResponse message: " << httpResponse.message;
                }
                asyncReply.error(errMsg.str());
                errorHandled = true;
            }

            return errorHandled;
        }

    } // namespace devices

} // namespace karabo
