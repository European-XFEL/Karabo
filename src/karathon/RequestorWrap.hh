/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 10, 2012, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARATHON_REQUESTORWRAP_HH
#define	KARATHON_REQUESTORWRAP_HH

#include <boost/python.hpp>
#include <karabo/xms/Requestor.hh>
#include "HashWrap.hh"
#include "ScopedGILRelease.hh"

namespace bp = boost::python;

//namespace karabo {
//    namespace xms {
//        class SignalSlotable;
//    }
//}

namespace karathon {

    class RequestorWrap : public karabo::xms::Requestor {

    public:

        explicit RequestorWrap(karabo::xms::SignalSlotable* signalSlotable) :
        karabo::xms::Requestor(signalSlotable) {
        }

        RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction) {
            {
                ScopedGILRelease nogil;
                sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash());
            }
            return *this;
        }

        RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const bp::object& a1) {            
            sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1));
            //registerRequest();
            return *this;
        }

        template <class A1, class A2>
        RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) {
            sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2));            
            return *this;
        }

        template <class A1, class A2, class A3>
        RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) {
            sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3));
            return *this;
        }

        template <class A1, class A2, class A3, class A4>
        RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
            sendRequest(prepareHeader(slotInstanceId, slotFunction), karabo::util::Hash("a1", a1, "a2", a2, "a3", a3, "a4", a4));          
            return *this;
        }

        bp::tuple waitForReply(const int& milliseconds) {
            try {
                timeout(milliseconds);
                karabo::util::Hash body, header;

                {
                    ScopedGILRelease nogil;                    
                    receiveResponse(header, body);
                }

                size_t arity = body.size();
                switch (arity) {
                    case 0:
                        return prepareTuple0(body);
                    case 1:
                        return prepareTuple1(body);
                    case 2:
                        return prepareTuple2(body);
                    case 3:
                        return prepareTuple3(body);
                    case 4:
                        return prepareTuple4(body);
                    default:
                        throw KARABO_SIGNALSLOT_EXCEPTION("Too many arguments send as response (max 4 are currently supported");
                }
            } catch (const karabo::util::Exception& e) {
                std::cout << e << std::endl;
                return bp::make_tuple();
            }
        }

        bp::tuple prepareTuple0(const karabo::util::Hash& body) {
            return bp::make_tuple();
        }

        bp::tuple prepareTuple1(const karabo::util::Hash& body) {
            bp::object a1 = HashWrap::get(body, "a1");
            return bp::make_tuple(a1);
        }

        bp::tuple prepareTuple2(const karabo::util::Hash& body) {
            bp::object a1 = HashWrap::get(body, "a1");
            bp::object a2 = HashWrap::get(body, "a2");
            return bp::make_tuple(a1, a2);
        }

        bp::tuple prepareTuple3(const karabo::util::Hash& body) {
            bp::object a1 = HashWrap::get(body, "a1");
            bp::object a2 = HashWrap::get(body, "a2");
            bp::object a3 = HashWrap::get(body, "a3");
            return bp::make_tuple(a1, a2, a3);
        }

        bp::tuple prepareTuple4(const karabo::util::Hash& body) {
            bp::object a1 = HashWrap::get(body, "a1");
            bp::object a2 = HashWrap::get(body, "a2");
            bp::object a3 = HashWrap::get(body, "a3");
            bp::object a4 = HashWrap::get(body, "a4");
            return bp::make_tuple(a1, a2, a3, a4);
        }
    };
}

#endif	/* KARATHON_REQUESTORWRAP_HH */

