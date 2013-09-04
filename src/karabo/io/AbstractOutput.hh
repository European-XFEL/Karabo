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
namespace karabo {

    namespace io {

        class AbstractOutput : public boost::enable_shared_from_this<AbstractOutput> {

            boost::any m_ioEventHandler;
            std::string m_instanceId;

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
            AbstractOutput(const karabo::util::Hash& input) {
            }

            virtual ~AbstractOutput() {
            }

            void setInstanceId(const std::string& instanceId) {
                m_instanceId = instanceId;
            }

            const std::string& getInstanceId() const {
                return m_instanceId;
            }

            template <class OutputType>
            void registerIOEventHandler(const boost::function<void (const boost::shared_ptr<OutputType>&) >& ioEventHandler) {
                m_ioEventHandler = ioEventHandler;
            }

            //virtual void onInputAvailable(const std::string& instanceId) {
            //}

            virtual karabo::util::Hash getInformation() const {
                return karabo::util::Hash();
            }

            virtual void update() {
            }
            
            virtual void signalEndOfStream() {
            }

            virtual bool canCompute() const {
                return true;
            }

        protected:

            template <class OutputType>
            void triggerIOEvent() {
                if (!m_ioEventHandler.empty()) (boost::any_cast < boost::function<void (const boost::shared_ptr<OutputType>&) > >(m_ioEventHandler))(boost::static_pointer_cast< OutputType > (shared_from_this()));
            }
        };
    }
}

#endif	
