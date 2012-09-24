/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 20, 2012, 4:15 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_PYEXFEL_MEMBERSLOTWRAP_HH
#define	EXFEL_PYEXFEL_MEMBERSLOTWRAP_HH

#include <boost/python.hpp>
#include <exfel/xms/Slot.hh>
#include <exfel/net/BrokerChannel.hh>
#include "HashWrap.hh"

namespace bp = boost::python;

namespace exfel {

    namespace xms {
        // Forward SignalSlotable
        class SignalSlotable;
    }

    namespace pyexfel {

        class MemberSlotWrap : public exfel::xms::Slot {
            typedef std::string SlotFunction;
            typedef PyObject* SelfObject;

        public:

            MemberSlotWrap(exfel::xms::SignalSlotable* signalSlotable, const exfel::net::BrokerChannel::Pointer& channel, const std::string& slotInstanceId, const std::string& slotFunction)
            : exfel::xms::Slot(signalSlotable, channel, slotInstanceId, slotFunction) {
                m_channel->readAsyncHashHash(boost::bind(&MemberSlotWrap::callRegisteredSlotFunctions, this, _1, _2, _3));
            }

            virtual ~MemberSlotWrap() {
            }

            void registerSlotFunction(const SlotFunction& slotHandler, const SelfObject& selfObject) {
                m_slotFunction = slotHandler;
                m_selfObject = selfObject;
            }

        private: // function

            void callRegisteredSlotFunctions(exfel::net::BrokerChannel::Pointer /*channel*/, const exfel::util::Hash& body, const exfel::util::Hash& header) {

                startSlotProcessing();

                PyGILState_STATE gstate = PyGILState_Ensure();

                try {

                    size_t arity = body.size();
                    switch (arity) {
                        case 4:
                            if (callFunction4(body)) break;
                        case 3:
                            if (callFunction3(body)) break;
                        case 2:
                            if (callFunction2(body)) break;
                        case 1:
                            if(callFunction1(body)) break;
                        case 0:
                            if (callFunction0(body)) break;
                        default:
                            throw SIGNALSLOT_EXCEPTION("Too many arguments send to python slot (max 4 are currently supported");
                    }

                    handlePossibleReply(header);
                    PyGILState_Release(gstate);

                    stopSlotProcessing();

                } catch (const exfel::util::Exception& e) {
                    std::cout << e.userFriendlyMsg();
                    PyGILState_Release(gstate);
                    stopSlotProcessing();
                }

            }

            bool callFunction0(const exfel::util::Hash& body) {
                try {
                    bp::call_method<void>(m_selfObject, m_slotFunction.c_str());
                } catch (...) {
                    return false;
                }
                return true;
            }

            bool callFunction1(const exfel::util::Hash& body) {
                exfel::util::Hash::const_iterator it = body.begin();
                bp::object a1 = HashWrap::pythonGetArgIt(body, it);
                try {
                    bp::call_method<void>(m_selfObject, m_slotFunction.c_str(), a1);
                } catch (...) {
                    return false;
                }
                return true;
            }

            bool callFunction2(const exfel::util::Hash& body) {
                exfel::util::Hash::const_iterator it = body.begin();
                bp::object a1 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a2 = HashWrap::pythonGetArgIt(body, it);
                try {
                    bp::call_method<void>(m_selfObject, m_slotFunction.c_str(), a1, a2);
                } catch (...) {
                    return false;
                }
                return true;
            }

            bool callFunction3(const exfel::util::Hash& body) {
                exfel::util::Hash::const_iterator it = body.begin();
                bp::object a1 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a2 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a3 = HashWrap::pythonGetArgIt(body, it);
                try {
                    bp::call_method<void>(m_selfObject, m_slotFunction.c_str(), a1, a2, a3);
                } catch (...) {
                    return false;
                }
                return true;
            }

            bool callFunction4(const exfel::util::Hash& body) {
                exfel::util::Hash::const_iterator it = body.begin();
                bp::object a1 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a2 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a3 = HashWrap::pythonGetArgIt(body, it++);
                bp::object a4 = HashWrap::pythonGetArgIt(body, it);
                try {
                    bp::call_method<void>(m_selfObject, m_slotFunction.c_str(), a1, a2, a3, a4);
                } catch (...) {
                    return false;
                }
                return true;
            }

        private: // member

            SlotFunction m_slotFunction;
            SelfObject m_selfObject;
        };




    }
}

#endif	/* EXFEL_PYEXFEL_SLOTWRAP_HH */

