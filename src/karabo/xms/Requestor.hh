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

#ifndef KARABO_XMS_REQUESTOR_HH
#define	KARABO_XMS_REQUESTOR_HH

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/asio.hpp>

#include <karabo/util/Configurator.hh>
#include <karabo/net/BrokerChannel.hh>

namespace karabo {
    namespace xms {

        class SignalSlotable;
        
        class Requestor {
            
        protected:

            SignalSlotable* m_signalSlotable;
            
            std::string m_replyId;
            bool m_isRequested;
            bool m_isReceived;                       
            int m_timeout;
            static boost::uuids::random_generator m_uuidGenerator;

        public:

            KARABO_CLASSINFO(Requestor, "Requestor", "1.0")

            explicit Requestor(SignalSlotable* signalSlotable);

            virtual ~Requestor() {
            }

            Requestor& setResponder(const std::string& instanceId, const std::string& responseFunction);

            Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash());
                return *this;
            }

            template <class A1>
            Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1));
                return *this;
            }

            template <class A1, class A2>
            Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2));
                return *this;
            }

            template <class A1, class A2, class A3>
            Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
                return *this;
            }

            template <class A1, class A2, class A3, class A4>
            Requestor& request(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
                return *this;
            }
            
            void receive(const boost::function<void () >& replyCallback) {
                m_signalSlotable->registerSlot(replyCallback, m_replyId);
            }
            
            template <class A1>
            void receive(const boost::function<void (const A1&) >& replyCallback) {
                m_signalSlotable->registerSlot(replyCallback, m_replyId);
            }
            
            template <class A1, class A2>
            void receive(const boost::function<void (const A1&, const A2&) >& replyCallback) {
                m_signalSlotable->registerSlot(replyCallback, m_replyId);
            }
            
            template <class A1, class A2, class A3>
            void receive(const boost::function<void (const A1&, const A2&, const A3&) >& replyCallback) {
                m_signalSlotable->registerSlot(replyCallback, m_replyId);
            }
            
            template <class A1, class A2, class A3, class A4>
            void receive(const boost::function<void (const A1&, const A2&, const A3&, const A4&) >& replyCallback) {
                m_signalSlotable->registerSlot(replyCallback, m_replyId);
            }                                                          
            
            Requestor& requestNoWait(
            const std::string& requestSlotInstanceId, 
            const std::string& requestSlotFunction,
            const std::string replySlotInstanceId, 
            const std::string& replySlotFunction) {
                sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash());
                return *this;
            }

            template <class A1>
            Requestor& requestNoWait(
            const std::string& requestSlotInstanceId, 
            const std::string& requestSlotFunction,
            const std::string replySlotInstanceId, 
            const std::string& replySlotFunction, 
            const A1& a1) {
                sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash("a1", a1));
                return *this;
            }

            template <class A1, class A2>
            Requestor& requestNoWait(
            const std::string& requestSlotInstanceId, 
            const std::string& requestSlotFunction,
            const std::string replySlotInstanceId, 
            const std::string& replySlotFunction,
            const A1& a1, const A2& a2) {
                sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash("a1", a1, "a2", a2));
                return *this;
            }

            template <class A1, class A2, class A3>
            Requestor& requestNoWait(  
            const std::string& requestSlotInstanceId, 
            const std::string& requestSlotFunction,
            const std::string replySlotInstanceId, 
            const std::string& replySlotFunction, const A1& a1, const A2& a2, const A3& a3) {
                sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
                return *this;
            }

            template <class A1, class A2, class A3, class A4>
            Requestor& requestNoWait( const std::string& requestSlotInstanceId, 
            const std::string& requestSlotFunction,
            const std::string replySlotInstanceId, 
            const std::string& replySlotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                sendRequest(prepareHeaderNoWait(requestSlotInstanceId, requestSlotFunction, replySlotInstanceId, replySlotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
                return *this;
            }

            void receive() {
                try {
                    karabo::util::Hash::Pointer header, body;                  
                    receiveResponse(header, body);
                    if (header->has("error")) throw KARABO_SIGNALSLOT_EXCEPTION(header->get<std::string>("error"));                    
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
            }

            template <class A1>
            void receive(A1& a1) {
                try {                   
                    karabo::util::Hash::Pointer header, body;
                    receiveResponse(header, body);
                    if (header->has("error")) throw KARABO_SIGNALSLOT_EXCEPTION(header->get<std::string>("error")); 
                    a1 = body->get<A1 > ("a1");
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
            }

            template <class A1, class A2>
            void receive(A1& a1, A2& a2) {
                try {                    
                    karabo::util::Hash::Pointer body, header;
                    receiveResponse(header, body);
                    if (header->has("error")) throw KARABO_SIGNALSLOT_EXCEPTION(header->get<std::string>("error"));
                    a1 = body->get<A1 > ("a1");
                    a2 = body->get<A2 > ("a2");
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
            }

            template <class A1, class A2, class A3>
            void receive(A1& a1, A2& a2, A3& a3) {
                try {
                    karabo::util::Hash::Pointer body, header;
                    receiveResponse(header, body);
                    if (header->has("error")) throw KARABO_SIGNALSLOT_EXCEPTION(header->get<std::string>("error"));
                    a1 = body->get<A1 > ("a1");
                    a2 = body->get<A2 > ("a2");
                    a3 = body->get<A3 > ("a3");
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
            }

            template <class A1, class A2, class A3, class A4>
            void receive(A1& a1, A2& a2, A3& a3, A4& a4) {
                try {
                    karabo::util::Hash::Pointer body, header;
                    receiveResponse(header, body);
                    if (header->has("error")) throw KARABO_SIGNALSLOT_EXCEPTION(header->get<std::string>("error"));
                    a1 = body->get<A1 > ("a1");
                    a2 = body->get<A2 > ("a2");
                    a3 = body->get<A3 > ("a3");
                    a4 = body->get<A4 > ("a4");
                } catch (const karabo::util::TimeoutException&) {
                    KARABO_RETHROW_AS(KARABO_TIMEOUT_EXCEPTION("Response timed out"));
                } catch (const karabo::util::CastException&) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION("Received unexpected (incompatible) response type"));
                } catch (const karabo::util::NetworkException&) {
                    KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("Could not send request"));
                }
            }

            Requestor& timeout(const int& milliseconds);

        protected: // functions

            karabo::util::Hash prepareHeader(const std::string& slotInstanceId, const std::string& slotFunction);
            
            karabo::util::Hash prepareHeaderNoWait(const std::string& requestSlotInstanceId, const std::string& requestSlotFunction,
            const std::string& replySlotInstanceId, const std::string& replySlotFunction);

            void registerRequest();

            static std::string generateUUID();

            void sendRequest(const karabo::util::Hash& header, const karabo::util::Hash& body) const;

            void receiveResponse(karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body);

       

        };

    }
}
#endif	

