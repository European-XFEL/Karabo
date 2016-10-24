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

#include "HashWrap.hh"
#include "karabo/xms/Slot.hh"
#include "ScopedGILAcquire.hh"
#include <boost/python.hpp>

namespace bp = boost::python;

namespace karabo {

    namespace xms {
        // Forward SignalSlotable
        class SignalSlotable;
    }
}

namespace karathon {

    class SlotWrap : public karabo::xms::Slot {


        bp::object m_slotFunction;
        size_t m_arity; // arity of position arguments, except *args
        bool m_varargs; // flag of *args
        bool m_varkeywords; // flag of **kwargs

    public:

        SlotWrap(const std::string& slotFunction);

        virtual ~SlotWrap();

        void registerSlotFunction(const bp::object& slotHandler);

    private: // function

        void doCallRegisteredSlotFunctions(const karabo::util::Hash& body);

        void callFunction0(const karabo::util::Hash& body);

        void callFunction1(const karabo::util::Hash& body);

        void callFunction2(const karabo::util::Hash& body);

        void callFunction3(const karabo::util::Hash& body);

        void callFunction4(const karabo::util::Hash& body);

        void rethrowPythonException();
    };
}

#endif

