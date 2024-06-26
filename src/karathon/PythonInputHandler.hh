/*
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
/*
 * File:   PythonInputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 5:37 PM
 */

#ifndef KARATHON_PYTHONINPUTHANDLER_HH
#define KARATHON_PYTHONINPUTHANDLER_HH

#include <boost/any.hpp>
#include <boost/python.hpp>
#include <karabo/io/AbstractInput.hh>
#include <karabo/io/InputHandler.hh>
#include <karabo/util/Configurator.hh>

#include "ScopedGILAcquire.hh"
#include "Wrapper.hh"

namespace bp = boost::python;

namespace karathon {

    template <class InputType>
    class PythonInputHandler : public karabo::io::InputHandler {
       public:
        KARABO_CLASSINFO(PythonInputHandler, "PythonInputHandler" + std::string(typeid(InputType).name()), "1.0")

        PythonInputHandler() {}

        PythonInputHandler(const karabo::io::AbstractInput::Pointer& input)
            : m_input(boost::static_pointer_cast<InputType>(input)) {}

        virtual ~PythonInputHandler() {}

        void registerIOEventHandler(const boost::any& ioEventHandler) {
            bp::object handler = boost::any_cast<bp::object>(ioEventHandler);
            if (handler == bp::object()) {
                m_ioEventHandler = bp::object();
                return;
            }
            // Check if input handler is a python function at least
            if (!Wrapper::hasattr(handler, "__name__")) {
                throw KARABO_PYTHON_EXCEPTION("This python object is not a function.");
            }
            m_ioEventHandler = handler;
        }

        void registerEndOfStreamEventHandler(const boost::any& endOfStreamEventHandler) {
            bp::object handler = boost::any_cast<bp::object>(endOfStreamEventHandler);
            if (handler == bp::object()) {
                m_endOfStreamEventHandler = bp::object();
                return;
            }
            if (!Wrapper::hasattr(handler, "__name__")) {
                throw KARABO_PYTHON_EXCEPTION("This python object is not a function.");
            }
            m_endOfStreamEventHandler = handler;
        }

        void triggerIOEvent() {
            ScopedGILAcquire gil;
            if (m_ioEventHandler != bp::object()) {
                if (typename InputType::Pointer in = m_input.lock()) m_ioEventHandler(bp::object(in));
            }
        }

        void triggerEndOfStreamEvent() {
            ScopedGILAcquire gil;
            if (m_endOfStreamEventHandler != bp::object()) m_endOfStreamEventHandler();
        }

       private:
        boost::weak_ptr<InputType> m_input;
        bp::object m_ioEventHandler;
        bp::object m_endOfStreamEventHandler;
    };
} // namespace karathon

#endif /* KARATHON_PYTHONINPUTHANDLER_HH */
