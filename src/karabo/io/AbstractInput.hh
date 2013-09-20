/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on June 05, 2012, 6:04 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_ABSTRACTINPUT_HH
#define	KARABO_IO_ABSTRACTINPUT_HH

#include <boost/function.hpp>
#include <boost/any.hpp>
#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/io/InputHandler.hh>

namespace karabo {
    namespace io {

        class AbstractInput : public boost::enable_shared_from_this<AbstractInput> {

        public:

            KARABO_CLASSINFO(AbstractInput, "AbstractInput", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected) {
            }

            AbstractInput() {
            }

            AbstractInput(const karabo::util::Hash& configuration) {
            }

            virtual ~AbstractInput() {
            }

            virtual void reconfigure(const karabo::util::Hash& input) {
            }

            void setInstanceId(const std::string& instanceId) {
                m_instanceId = instanceId;
            }

            const std::string& getInstanceId() const {
                return m_instanceId;
            }

            void setInputHandlerType(const std::string& handlerType) {
                std::string capitalType = boost::algorithm::to_upper_copy(handlerType);
                if (capitalType == "C++")
                    m_handlers = karabo::util::Factory<InputHandler>::create("CppInputHandler", shared_from_this());
                else if (capitalType == "PYTHON")
                    m_handlers = karabo::util::Factory<InputHandler>::create("PythonInputHandler", shared_from_this());
                else
                    throw KARABO_PARAMETER_EXCEPTION("Handler type " + handlerType + " is not supported.  Supported types (case-insensitive) are C++, Python");
            }

            InputHandler::Pointer getInputHandler() {
                return m_handlers;
            }

            void registerIOEventHandler(const boost::any& ioEventHandler) {
                if (!m_handlers)
                    throw KARABO_LOGIC_EXCEPTION("Handler storage not initialized: call 'setInputHandlerType' first.");
                m_handlers->registerIOEventHandler(ioEventHandler);
            }

            void registerEndOfStreamEventHandler(const boost::any& endOfStreamEventHandler) {
                if (!m_handlers)
                    throw KARABO_LOGIC_EXCEPTION("Handler storage not initialized: call 'setInputHandlerType' first.");
                m_handlers->registerEndOfStreamEventHandler(endOfStreamEventHandler);
            }

            virtual bool needsDeviceConnection() const { // TODO Check if we can get rid of this
                return false;
            }

            virtual std::vector<karabo::util::Hash> getConnectedOutputChannels() {
                return std::vector<karabo::util::Hash > ();
                std::vector<int> v;
            }

            virtual void connectNow(const karabo::util::Hash& outputChannelInfo) {
            }

            virtual bool canCompute() const {
                return true;
            }

            virtual void update() {
            }

            virtual void setEndOfStream() const {
            }

        protected:

            void triggerIOEvent() {
                if (m_handlers) {
                    m_handlers->triggerIOEvent();
                }
            }

            void triggerEndOfStreamEvent() {
                if (m_handlers) {
                    m_handlers->triggerEndOfStreamEvent();
                }
            }

        private:
            boost::shared_ptr<InputHandler> m_handlers;
            std::string m_instanceId;
        };
    }
}

#endif	
