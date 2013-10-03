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
                if (!m_ioEventHandler.empty()) {
                    if (AbstractInput::Pointer in = m_input.lock()) m_ioEventHandler(in);
                }
            }

            void triggerEndOfStreamEvent() {
                if (!m_endOfStreamEventHandler.empty()) m_endOfStreamEventHandler();
            }

        private:
            boost::weak_ptr<AbstractInput> m_input;
            boost::function<void (const AbstractInput::Pointer&) > m_ioEventHandler;
            boost::function<void() > m_endOfStreamEventHandler;
        };

    }
}

#endif	/* KARABO_IO_CPPINPUTHANDLER_HH */

