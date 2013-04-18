/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 10, 2012, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_PYKARABO_REQUESTORWRAP_HH
#define	KARABO_PYKARABO_REQUESTORWRAP_HH

#include <boost/python.hpp>
#include <karabo/xms/Requestor.hh>
#include <karabo/net/BrokerChannel.hh>
#include "HashWrap.hh"
#include "ScopedGILRelease.hh"

namespace bp = boost::python;

namespace karabo {

    namespace pyexfel {

        class RequestorWrap : public karabo::xms::Requestor {
        public:

            RequestorWrap(const karabo::net::BrokerChannel::Pointer& channel, const std::string& requestInstanceId) :
            karabo::xms::Requestor(channel, requestInstanceId) {
            }

            RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                registerRequest();
                m_body.clear();
                return *this;
            }

            template <class A1>
            RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                registerRequest();
                m_body.clear();
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a1", a1);
                return *this;

            }

            template <class A1, class A2>
            RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                registerRequest();
                m_body.clear();
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a1", a1);
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a2", a2);
                return *this;
            }

            template <class A1, class A2, class A3>
            RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                registerRequest();
                m_body.clear();
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a1", a1);
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a2", a2);
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a3", a3);
                return *this;
            }

            template <class A1, class A2, class A3, class A4>
            RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                registerRequest();
                m_body.clear();
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a1", a1);
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a2", a2);
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a3", a3);
                karabo::pyexfel::HashWrap::pythonSet(m_body, "a4", a4);
                return *this;
            }

            bp::tuple waitForReply(const int& milliseconds) {
                try {
                    timeout(milliseconds);
                    karabo::util::Hash body, header;

                    {
                        ScopedGILRelease nogil;
                        sendRequest();
                        receiveResponse(body, header);
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
                bp::object a1 = HashWrap::pythonGet(body, "a1");
                return bp::make_tuple(a1);
            }

            bp::tuple prepareTuple2(const karabo::util::Hash& body) {
                bp::object a1 = HashWrap::pythonGet(body, "a1");
                bp::object a2 = HashWrap::pythonGet(body, "a2");
                return bp::make_tuple(a1, a2);
            }

            bp::tuple prepareTuple3(const karabo::util::Hash& body) {
                bp::object a1 = HashWrap::pythonGet(body, "a1");
                bp::object a2 = HashWrap::pythonGet(body, "a2");
                bp::object a3 = HashWrap::pythonGet(body, "a3");
                return bp::make_tuple(a1, a2, a3);
            }

            bp::tuple prepareTuple4(const karabo::util::Hash& body) {
                bp::object a1 = HashWrap::pythonGet(body, "a1");
                bp::object a2 = HashWrap::pythonGet(body, "a2");
                bp::object a3 = HashWrap::pythonGet(body, "a3");
                bp::object a4 = HashWrap::pythonGet(body, "a4");
                return bp::make_tuple(a1, a2, a3, a4);
            }
        };
    }
}

#endif	/* KARABO_PYKARABO_REQUESTORWRAP_HH */

