/*
 * $Id$
 *
 * File:   AbstractOutput.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on June 05, 2011, 6:04 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_ABSTRACTOUTPUT_HH
#define	KARABO_IO_ABSTRACTOUTPUT_HH

#include <boost/function.hpp>
#include <boost/any.hpp>
#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>
#include "OutputHandler.hh"

namespace karabo {
    namespace io {

        class AbstractOutput : public boost::enable_shared_from_this<AbstractOutput> {

        public:

            KARABO_CLASSINFO(AbstractOutput, "AbstractOutput", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            AbstractOutput(const karabo::util::Hash& configuration) {
            }

            AbstractOutput() {
            }

            virtual ~AbstractOutput() {
            }

            void setInstanceId(const std::string& instanceId) {
                m_instanceId = instanceId;
            }

            const std::string& getInstanceId() const {
                return m_instanceId;
            }

            void setOutputHandlerType(const std::string& handlerType) {
                std::string capitalType = boost::algorithm::to_upper_copy(handlerType);
                if (capitalType == "C++")
                    m_handlers = karabo::util::Factory<OutputHandler>::create<AbstractOutput::Pointer>("CppOutputHandler", shared_from_this());
                else if (capitalType == "PYTHON")
                    m_handlers = karabo::util::Factory<OutputHandler>::create<AbstractOutput::Pointer>("PythonOutputHandler", shared_from_this());
                else
                    throw KARABO_PARAMETER_EXCEPTION("Handler type " + handlerType + " is not supported.  Supported types (case-insensitive) are C++, Python");
            }

            OutputHandler::Pointer getOutputHandler() {
                return m_handlers;
            }

            void registerIOEventHandler(const boost::any& ioEventHandler) {
                if (!m_handlers)
                    throw KARABO_LOGIC_EXCEPTION("Handler storage not initialized: call 'setOutputHandler' first.");
                m_handlers->registerIOEventHandler(ioEventHandler);
            }

            //virtual void onInputAvailable(const std::string& instanceId) {
            //}

            virtual karabo::util::Hash getInformation() const {
                return karabo::util::Hash();
            }

            
            virtual void update(const bool& unsetEndOfStreamFlag = true) {
                
            }

            virtual void signalEndOfStream() {
            }

            virtual bool canCompute() const {
                return true;
            }

        protected:
            
            void triggerIOEvent() {
                if (m_handlers) {
                    m_handlers->triggerIOEvent();
                }
            }
                       
        protected:

            std::string m_instanceId;
            OutputHandler::Pointer m_handlers;
        };
    }
}

#endif	
