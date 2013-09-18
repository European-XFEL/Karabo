/* 
 * File:   CppInputHandler.hh
 * Author: esenov
 *
 * Created on September 17, 2013, 5:23 PM
 */

#ifndef KARABO_IO_CPPINPUTHANDLER_HH
#define	KARABO_IO_CPPINPUTHANDLER_HH

#include <karabo/io/AbstractInput.hh>
#include <karabo/io/InputHandler.hh>

namespace karabo {
    namespace io {

        class CppInputHandler : public InputHandler {

        public:

            KARABO_CLASSINFO(CppInputHandler, "CppInputHandler", "1.0")

            CppInputHandler() {
            }

            CppInputHandler(const AbstractInput::Pointer& input) :  m_input(input) {
            }

            virtual ~CppInputHandler() {
            }

            void registerIOEventHandler(const boost::any& ioEventHandler) {
                m_ioEventHandler = boost::any_cast < boost::function<void (const AbstractInput::Pointer&) > >(ioEventHandler);
            }

            void registerEndOfStreamEventHandler(const boost::any& endOfStreamEventHandler) {
                m_endOfStreamEventHandler = boost::any_cast < boost::function<void ()> >(endOfStreamEventHandler);
            }

            void triggerIOEvent() {
                std::cout << "CppInputHandler::triggerIOEvent coming" << std::endl;
                if (!m_ioEventHandler.empty()) m_ioEventHandler(m_input);
            }

            void triggerEndOfStreamEvent() {
                if (!m_endOfStreamEventHandler.empty())
                    m_endOfStreamEventHandler();
            }

        private:
            AbstractInput::Pointer m_input;
            boost::function<void (const AbstractInput::Pointer&) > m_ioEventHandler;
            boost::function<void() > m_endOfStreamEventHandler;
        };

    }
}

#endif	/* KARABO_IO_CPPINPUTHANDLER_HH */

