/* 
 * File:   CppOutputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 6:12 PM
 */

#ifndef KARABO_IO_CPPOUTPUTHANDLER_HH
#define	KARABO_IO_CPPOUTPUTHANDLER_HH

#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include "OutputHandler.hh"
#include "AbstractOutput.hh"

namespace karabo {
    namespace io {

        class CppOutputHandler : public OutputHandler {

            public:

            KARABO_CLASSINFO(CppOutputHandler, "CppOutputHandler", "1.0")

            CppOutputHandler() {
            }

            CppOutputHandler(const AbstractOutput::Pointer& output) : m_output(output) {
            }

            virtual ~CppOutputHandler() {
            }

            void registerIOEventHandler(const boost::any& ioEventHandler) {
                m_ioEventHandler = boost::any_cast < boost::function<void (const AbstractOutput::Pointer&) > >(ioEventHandler);
            }

            void triggerIOEvent() {
                if (!m_ioEventHandler.empty()) {
                    if (AbstractOutput::Pointer out = m_output.lock()) m_ioEventHandler(out);
                }
            }

        private:
            boost::weak_ptr<AbstractOutput> m_output;
            boost::function<void (const AbstractOutput::Pointer&) > m_ioEventHandler;
        };
    }
}

#endif	/* KARABO_IO_CPPOUTPUTHANDLER_HH */

