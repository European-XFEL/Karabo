/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   PythonOutputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 7:59 PM
 */

#ifndef KARATHON_PYTHONOUTPUTHANDLER_HH
#define KARATHON_PYTHONOUTPUTHANDLER_HH

#include <boost/any.hpp>
#include <boost/python.hpp>
#include <karabo/io/AbstractOutput.hh>
#include <karabo/io/OutputHandler.hh>
#include <karabo/util/Configurator.hh>

#include "ScopedGILAcquire.hh"
#include "Wrapper.hh"

namespace bp = boost::python;

namespace karathon {

    class PythonOutputHandler : public karabo::io::OutputHandler {
       public:
        KARABO_CLASSINFO(PythonOutputHandler, "PythonOutputHandler", "1.0")

        PythonOutputHandler() {}

        PythonOutputHandler(const karabo::io::AbstractOutput::Pointer& output) : m_output(output) {}

        virtual ~PythonOutputHandler() {}

        void registerIOEventHandler(const boost::any& ioEventHandler) {
            m_ioEventHandler = boost::any_cast<bp::object>(ioEventHandler);
        }

        void triggerIOEvent() {
            ScopedGILAcquire gil;
            if (m_ioEventHandler != bp::object()) {
                if (karabo::io::AbstractOutput::Pointer out = m_output.lock()) m_ioEventHandler(out);
            }
        }

       private:
        boost::weak_ptr<karabo::io::AbstractOutput> m_output;
        bp::object m_ioEventHandler;
    };
} // namespace karathon

#endif /* KARATHON_PYTHONOUTPUTHANDLER_HH */
