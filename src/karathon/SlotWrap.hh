/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARATHON_SLOTWRAP_HH
#define KARATHON_SLOTWRAP_HH

#include <boost/python.hpp>
#include <memory>

#include "HashWrap.hh"
#include "karabo/xms/Slot.hh"

namespace bp = boost::python;

namespace karathon {

    class SlotWrap : public karabo::xms::Slot {
        // unique_ptr seems more light weight than shared_ptr (no thread guarantees)
        std::unique_ptr<bp::object> m_slotFunction;
        size_t m_arity; // arity of position arguments, except *args

       public:
        SlotWrap(const std::string& slotFunction);

        virtual ~SlotWrap();

        void registerSlotFunction(const bp::object& slotHandler, int numArgs);

       private: // function
        void doCallRegisteredSlotFunctions(const karabo::util::Hash& body);

        void callFunction0(const karabo::util::Hash& body);

        void callFunction1(const karabo::util::Hash& body);

        void callFunction2(const karabo::util::Hash& body);

        void callFunction3(const karabo::util::Hash& body);

        void callFunction4(const karabo::util::Hash& body);

        /// Helper for callFunction<N> to get argument with given key ("a1", ..., or "a4") out of body.
        /// Throws if that key is missing.
        bp::object getBodyArgument(const karabo::util::Hash& body, const char* key) const;

        void rethrowPythonException();
    };
} // namespace karathon

#endif
