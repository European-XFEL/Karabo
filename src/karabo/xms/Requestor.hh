/*
 * $Id$
 *
 * File:   Request.hh
 * Author: <your.email@xfel.eu>
 *
 * Created on February 2, 2012, 6:23 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_REQUEST_HH
#define	KARABO_XMS_REQUEST_HH

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/asio.hpp>

#include <karabo/util/Factory.hh>
#include <karabo/net/BrokerChannel.hh>

namespace karabo {
    namespace xms {

        class Requestor {
        public:

            KARABO_CLASSINFO(Requestor, "Requestor", "1.0")

            Requestor(const karabo::net::BrokerChannel::Pointer& channel, const std::string& requestInstanceId) :
            m_channel(channel), m_requestInstanceId(requestInstanceId), m_replyId(generateUUID()), m_isRequested(false), m_isReceived(false) {
            }
            
            virtual ~Requestor() {
            }

            Requestor& setResponder(const std::string& instanceId, const std::string& responseFunction);

            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                return request();
            }

            template <class A1>
            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                return request(a1);
                
            }

            template <class A1, class A2>
            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                return request(a1, a2);
            }

            template <class A1, class A2, class A3>
            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                return request(a1, a2, a3);
            }

            template <class A1, class A2, class A3, class A4>
            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                return request(a1, a2, a3, a4);
            }

           
            Requestor& receive() {
                try {
                    karabo::util::Hash body, header;
                    sendRequest();
                    receiveResponse(body, header);
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
                return *this;
            }

            template <class A1>
            Requestor& receive(A1& a1) {
                try {
                    karabo::util::Hash body, header;
                    sendRequest();
                    receiveResponse(body, header);
                    a1 = body.get<A1 > ("a1");
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
                return *this;
            }

            template <class A1, class A2>
            Requestor& receive(A1& a1, A2& a2) {
                try {
                    karabo::util::Hash body, header;
                    sendRequest();
                    receiveResponse(body, header);
                    a1 = body.get<A1 > ("a1");
                    a2 = body.get<A2 > ("a2");
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
                return *this;
            }

            template <class A1, class A2, class A3>
            Requestor& receive(A1& a1, A2& a2, A3& a3) {
                try {
                    karabo::util::Hash body, header;
                    sendRequest();
                    receiveResponse(body, header);
                    a1 = body.get<A1 > ("a1");
                    a2 = body.get<A2 > ("a2");
                    a3 = body.get<A3 > ("a3");
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
                return *this;
            }

            template <class A1, class A2, class A3, class A4>
            Requestor& receive(A1& a1, A2& a2, A3& a3, A4& a4) {
                try {
                    karabo::util::Hash body, header;
                    sendRequest();
                    receiveResponse(body, header);
                    a1 = body.get<A1 > ("a1");
                    a2 = body.get<A2 > ("a2");
                    a3 = body.get<A3 > ("a3");
                    a4 = body.get<A4 > ("a4");
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
                return *this;
            }

            Requestor& timeout(const int& milliseconds) {
                m_channel->setTimeoutSyncRead(milliseconds);
                return *this;
            }

        protected: // functions
            
            Requestor& request() {
                registerRequest();
                m_body.clear();
                return *this;
            }

            template <class A1>
            Requestor& request(const A1& a1) {
                registerRequest();
                m_body.clear();
                m_body.set("a1", a1);
                return *this;
            }

            template <class A1, class A2>
            Requestor& request(const A1& a1, const A2& a2) {
                registerRequest();
                m_body.clear();
                m_body.set("a1", a1);
                m_body.set("a2", a2);
                return *this;
            }

            template <class A1, class A2, class A3>
            Requestor& request(const A1& a1, const A2& a2, const A3& a3) {
                registerRequest();
                m_body.clear();
                m_body.set("a1", a1);
                m_body.set("a2", a2);
                m_body.set("a3", a3);
                return *this;
            }

            template <class A1, class A2, class A3, class A4>
            Requestor& request(const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                registerRequest();
                m_body.clear();
                m_body.set("a1", a1);
                m_body.set("a2", a2);
                m_body.set("a3", a3);
                m_body.set("a4", a4);
                return *this;
            }

            void prepareHeaderAndFilter(const std::string& slotInstanceId, const std::string& slotFunction) {
                m_header.set("slotInstanceId", slotInstanceId + "|");
                m_header.set("slotFunction", slotFunction + "|");
                m_header.set("hostName", boost::asio::ip::host_name());
                m_header.set("replyTo", m_replyId);
                m_header.set("classId", "Requestor");
                m_channel->setFilter("replyFrom = '" + m_replyId + "'");
            }

            void registerRequest() {
                if (m_isRequested) throw KARABO_SIGNALSLOT_EXCEPTION("You have to receive an answer before you can send a new request");
                m_isRequested = true;
                m_isReceived = false;
            }

            static std::string generateUUID() {
                return boost::uuids::to_string(m_uuidGenerator());
            }

            void sendRequest() const {
                try {
                    m_channel->preRegisterSynchronousRead();
                    m_channel->write(m_body, m_header);
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Problems sending request"));
                }
            }

            void receiveResponse(karabo::util::Hash& body, karabo::util::Hash& header) {
                try {
                    if (m_isReceived) throw KARABO_SIGNALSLOT_EXCEPTION("You have to send a request before you can receive a response");
                    m_channel->read(body, header);
                    m_isReceived = true;
                    m_isRequested = false;
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Problems reading response"));
                }
            }

        protected: // members

            karabo::net::BrokerChannel::Pointer m_channel;
            std::string m_requestInstanceId;

            std::string m_replyId;
            bool m_isRequested;
            bool m_isReceived;

            karabo::util::Hash m_header;
            karabo::util::Hash m_body;

            static boost::uuids::random_generator m_uuidGenerator;

        };

    }
}
#endif	

