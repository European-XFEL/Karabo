/*
 * $Id$
 *
 * File:   Input.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on June 05, 2012, 6:04 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XMS_ABSTRACTINPUT_HH
#define	KARABO_XMS_ABSTRACTINPUT_HH

#include <boost/function.hpp>
#include <boost/any.hpp>
#include <karabo/util/Factory.hh>

namespace karabo {

    namespace xms {

        class AbstractInput : public boost::enable_shared_from_this<AbstractInput> {
        public:

            KARABO_CLASSINFO(AbstractInput, "AbstractInput", "1.0")
            KARABO_FACTORY_BASE_CLASS

            AbstractInput() {
            }

            virtual ~AbstractInput() {
            }

//            /**
//             * Necessary method as part of the factory/configuration system
//             * @param expected [out] Description of expected parameters for this object (Schema)
//             */
//            static void expectedParameters(karabo::util::Schema& expected) {
//            }
//
//            /**
//             * If this object is constructed using the factory/configuration system this method is called
//             * @param input Validated (@see expectedParameters) and default-filled configuration
//             */
//            void configure(const karabo::util::Hash& input) {
//               
//            }
            
            virtual void reconfigure(const karabo::util::Hash& input) {}
            
            void setInstanceId(const std::string& instanceId) {
                m_instanceId = instanceId;
            }
            
            const std::string& getInstanceId() const {
                return m_instanceId;
            }

            template <class InputType>
            void registerIOEventHandler(const boost::function<void (const boost::shared_ptr<InputType>&) >& ioEventHandler) {
                m_ioEventHandler = ioEventHandler;
            }
            
            virtual bool needsDeviceConnection() const {
                return false;
            }

            virtual std::vector<karabo::util::Hash> getConnectedOutputChannels() {
                return std::vector<karabo::util::Hash > ();
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

            template <class InputType>
            void triggerIOEvent() {
                if(!m_ioEventHandler.empty()) (boost::any_cast<boost::function<void (const boost::shared_ptr<InputType>&) > >(m_ioEventHandler))(boost::static_pointer_cast< InputType >(shared_from_this()));
            }
            
        private:

            std::string m_instanceId;
            boost::any m_ioEventHandler;

        };
    }
}

#endif	
