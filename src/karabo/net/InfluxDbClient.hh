/* 
 * File:   InfluxDbClient.hh
 * Author: <serguei.essenov@xfel.eu>, <raul.costa@xfel.eu>
 *
 * Created on November 14, 2019, 9:57 AM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_NET_INFLUXDBCLIENT_HH
#define	KARABO_NET_INFLUXDBCLIENT_HH

#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <future>

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/uuid/random_generator.hpp>

#include "karabo/net/Connection.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/util/ClassInfo.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/TimeDuration.hh"

namespace karabo {
    namespace net {


        using InfluxResponseHandler = boost::function<void(const HttpResponse &) >;
        typedef boost::function<void()> AsyncHandler;


        class InfluxDbClient : public boost::enable_shared_from_this<InfluxDbClient> {
        public:

            KARABO_CLASSINFO(InfluxDbClient, "InfluxDbClient", "2.6")

            InfluxDbClient(const karabo::util::Hash& input);

            virtual ~InfluxDbClient();

            void connectDbIfDisconnected(const AsyncHandler& hook = AsyncHandler());

            bool isConnected() const { return m_dbChannel && m_dbChannel->isOpen(); }

            void getQueryDb(const std::string& digest, const InfluxResponseHandler& action);

            void queryDb(const std::string& digest, const InfluxResponseHandler& action);

            void postQueryDb(const std::string& statement, const InfluxResponseHandler& action);

            void writeDb(const std::string& message);

            void writeDbComplete(const std::string& message, const boost::shared_ptr<void>& r);

            void getPingDb(const InfluxResponseHandler& action);

            boost::shared_ptr<std::string> postWriteDbStr(const std::string& batch,
                                                          const InfluxResponseHandler& action);

            void postWriteDb(const std::string& batch, const InfluxResponseHandler& action);

            static std::string generateUUID();

            bool connectWait(std::size_t millis);

        private:

            void onDbConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, const AsyncHandler& hook);

            void onDbRead(const karabo::net::ErrorCode& ec, const std::string& data);

            void onDbWrite(const karabo::net::ErrorCode& ec, boost::shared_ptr<std::vector<char> > p);

            void onDbComplete(const karabo::net::ErrorCode& ec,
                              boost::shared_ptr<std::vector<char> > p,
                              InfluxDbClient::Pointer q, boost::shared_ptr<void> r);

        private:

            std::string m_url;
            karabo::net::Connection::Pointer m_dbConnection;
            karabo::net::Channel::Pointer m_dbChannel;
            boost::mutex m_connectionRequestedMutex;
            bool m_connectionRequested;
            // maps Request-Id to pair of request string and action callback
            std::unordered_map<std::string, std::pair<std::string, InfluxResponseHandler> > m_registeredInfluxResponseHandlers;
            HttpResponse m_response;
            std::string m_hostname;
            std::string m_dbname;
            std::string m_durationUnit;
            static boost::mutex m_uuidGeneratorMutex;
            static boost::uuids::random_generator m_uuidGenerator;
            static const unsigned int k_connTimeoutMs;
        };

    } // namespace net
    
} // namespace karabo



#endif	/* KARABO_NET_INFLUXDBCLIENT_HH */

