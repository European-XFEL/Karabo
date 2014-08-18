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

            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash());
                return *this;
            }

            template <class A1>
            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1));
                return *this;
            }

            template <class A1, class A2>
            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2));
                return *this;
            }

            template <class A1, class A2, class A3>
            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
                return *this;
            }

            template <class A1, class A2, class A3, class A4>
            Requestor& call(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));
                return *this;
            }

            Requestor& receive() {
                try {
                    karabo::util::Hash header, body;                  
                    receiveResponse(header, body);
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
                    karabo::util::Hash header, body;
                    receiveResponse(header, body);
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

            Requestor& timeout(const int& milliseconds);

        protected: // functions

            karabo::util::Hash prepareHeader(const std::string& slotInstanceId, const std::string& slotFunction);

            void registerRequest();

            static std::string generateUUID();

            void sendRequest(const karabo::util::Hash& header, const karabo::util::Hash& body) const;

            void receiveResponse(karabo::util::Hash& header, karabo::util::Hash& body);

       

        };

    }
}
#endif	

