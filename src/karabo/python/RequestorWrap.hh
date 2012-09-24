/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 10, 2012, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_PYEXFEL_REQUESTORWRAP_HH
#define	EXFEL_PYEXFEL_REQUESTORWRAP_HH

#include <boost/python.hpp>
#include <exfel/xms/Requestor.hh>
#include <exfel/net/BrokerChannel.hh>
#include "HashWrap.hh"

namespace bp = boost::python;

namespace exfel {

    namespace pyexfel {

        class RequestorWrap : public exfel::xms::Requestor {
        public:

            RequestorWrap(const exfel::net::BrokerChannel::Pointer& channel, const std::string& requestInstanceId) :
            exfel::xms::Requestor(channel, requestInstanceId) {
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
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a1", a1);
                return *this;

            }

            template <class A1, class A2>
            RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                registerRequest();
                m_body.clear();
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a1", a1);
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a2", a2);
                return *this;
            }

            template <class A1, class A2, class A3>
            RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                registerRequest();
                m_body.clear();
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a1", a1);
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a2", a2);
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a3", a3);
                return *this;
            }

            template <class A1, class A2, class A3, class A4>
            RequestorWrap& callPy(const std::string& slotInstanceId, const std::string& slotFunction, const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
                prepareHeaderAndFilter(slotInstanceId, slotFunction);
                registerRequest();
                m_body.clear();
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a1", a1);
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a2", a2);
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a3", a3);
                exfel::pyexfel::HashWrap::pythonSet(m_body, "a4", a4);
                return *this;
            }

            bp::tuple waitForReply(const int& milliseconds) {
                try {
                    timeout(milliseconds);
                    exfel::util::Hash body, header;
                    sendRequest();
                    receiveResponse(body, header);
                    
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
                            throw SIGNALSLOT_EXCEPTION("Too many arguments send as response (max 4 are currently supported");
                    }
                } catch (const exfel::util::Exception& e) {
                    std::cout << e << std::endl;
                    return bp::make_tuple();
                }
            }

            bp::tuple prepareTuple0(const exfel::util::Hash& body) {
                return bp::make_tuple();
            }

            bp::tuple prepareTuple1(const exfel::util::Hash& body) {
                exfel::util::Hash::const_iterator it = body.begin();
                bp::object a1 = HashWrap::pythonGetArgIt(body, it);
                return bp::make_tuple(a1);
            }

            bp::tuple prepareTuple2(const exfel::util::Hash& body) {
                exfel::util::Hash::const_iterator it = body.begin();
                bp::object a1 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a2 = HashWrap::pythonGetArgIt(body, it);
                return bp::make_tuple(a1, a2);
            }

            bp::tuple prepareTuple3(const exfel::util::Hash& body) {
                exfel::util::Hash::const_iterator it = body.begin();
                bp::object a1 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a2 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a3 = HashWrap::pythonGetArgIt(body, it);
                return bp::make_tuple(a1, a2, a3);
            }

            bp::tuple prepareTuple4(const exfel::util::Hash& body) {
                exfel::util::Hash::const_iterator it = body.begin();
                bp::object a1 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a2 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a3 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a4 = HashWrap::pythonGetArgIt(body, it);
                return bp::make_tuple(a1, a2, a3, a4);
            }
        };
    }
}

#endif	/* EXFEL_PYEXFEL_REQUESTORWRAP_HH */

