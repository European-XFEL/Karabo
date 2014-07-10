/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARATHON_SLOTWRAP_HH
#define	KARATHON_SLOTWRAP_HH

#include <boost/python.hpp>
#include <karabo/xms/Slot.hh>
#include <karabo/net/BrokerChannel.hh>
#include "ScopedGILAcquire.hh"
#include "HashWrap.hh"

namespace bp = boost::python;

namespace karabo {

    namespace xms {
        // Forward SignalSlotable
        class SignalSlotable;
    }
}

namespace karathon {

    class SlotWrap : public karabo::xms::Slot {

        typedef PyObject* SlotFunction;

    public:

        SlotWrap(karabo::xms::SignalSlotable* signalSlotable, const karabo::net::BrokerChannel::Pointer& channel, const std::string& slotInstanceId, const std::string& slotFunction)
        : karabo::xms::Slot(signalSlotable, channel, slotInstanceId, slotFunction) {
            m_channel->readAsyncHashHash(boost::bind(&SlotWrap::callRegisteredSlotFunctions, this, _1, _2, _3));
        }

        virtual ~SlotWrap() {
        }

        void registerSlotFunction(const SlotFunction& slotHandler) {
            m_slotFunction = slotHandler;
        }

    private: // function

        void callRegisteredSlotFunctions(karabo::net::BrokerChannel::Pointer /*channel*/, const karabo::util::Hash& body, const karabo::util::Hash& header) {

            ScopedGILAcquire gil;

            extractSenderInformation(header);

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
                        if (callFunction1(body)) break;
                    case 0:
                        if (callFunction0(body)) break;
                        throw KARABO_LOGIC_EXCEPTION("TypeError exception happened \"somewhere\" in Python code");
                    default:
                        throw KARABO_SIGNALSLOT_EXCEPTION("Too many arguments send to python slot (max 4 are currently supported");
                }

                handlePossibleReply(header);
               

            } catch (const karabo::util::Exception& e) {
                std::cout << e.userFriendlyMsg();
                invalidateSenderInformation();
            }
            invalidateSenderInformation();
        }

        bool callFunction0(const karabo::util::Hash& body) {
            try {
                bp::call<void>(m_slotFunction);
            } catch (const bp::error_already_set&) {
                PyErr_Print();
                return false;
            }
            return true;
        }

        bool callFunction1(const karabo::util::Hash& body) {
            bp::object a1 = HashWrap::get(body, "a1");
            try {
                bp::call<void>(m_slotFunction, a1);
            } catch (const bp::error_already_set&) {
                tryToPrint();
                return false;
            }
            return true;
        }

        bool callFunction2(const karabo::util::Hash& body) {
            bp::object a1 = HashWrap::get(body, "a1");
            bp::object a2 = HashWrap::get(body, "a2");
            try {
                bp::call<void>(m_slotFunction, a1, a2);
            } catch (const bp::error_already_set&) {
                tryToPrint();
                return false;
            }
            return true;
        }

        bool callFunction3(const karabo::util::Hash& body) {
            bp::object a1 = HashWrap::get(body, "a1");
            bp::object a2 = HashWrap::get(body, "a2");
            bp::object a3 = HashWrap::get(body, "a3");
            try {
                bp::call<void>(m_slotFunction, a1, a2, a3);
            } catch (const bp::error_already_set&) {
                tryToPrint();
                return false;
            }
            return true;
        }

        bool callFunction4(const karabo::util::Hash& body) {
            bp::object a1 = HashWrap::get(body, "a1");
            bp::object a2 = HashWrap::get(body, "a2");
            bp::object a3 = HashWrap::get(body, "a3");
            bp::object a4 = HashWrap::get(body, "a4");
            try {
                bp::call<void>(m_slotFunction, a1, a2, a3, a4);
            } catch (const bp::error_already_set&) {
                tryToPrint();
                return false;
            }
            return true;
        }

        void tryToPrint() {
            PyObject *e, *v, *t;
            bool printing = true;

            // get the error indicators
            PyErr_Fetch(&e, &v, &t); // ref count incremented

            if (PyErr_GivenExceptionMatches(e, PyExc_TypeError))
                printing = false;

            // we reset it for later processing
            PyErr_Restore(e, v, t); // ref count decremented
            if (printing)
                PyErr_Print();
            else
                PyErr_Clear();
        }

    private: // member

        SlotFunction m_slotFunction;
    };

}

#endif	/* KARATHON_SLOTWRAP_HH */

