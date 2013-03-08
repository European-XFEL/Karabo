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

namespace karabo {

    namespace io {

        class AbstractInput : public boost::enable_shared_from_this<AbstractInput> {

        public:

            KARABO_CLASSINFO(AbstractInput, "AbstractInput", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            AbstractInput() {
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

            template <class InputType>
            void registerIOEventHandler(const boost::function<void (const boost::shared_ptr<InputType>&) >& ioEventHandler) {
                m_ioEventHandler = ioEventHandler;
            }

            virtual bool needsDeviceConnection() const { // TODO Check if we can get rid of this
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
                if (!m_ioEventHandler.empty()) (boost::any_cast < boost::function<void (const boost::shared_ptr<InputType>&) > >(m_ioEventHandler))(boost::static_pointer_cast< InputType >(shared_from_this()));
            }

        private:

            std::string m_instanceId;
            boost::any m_ioEventHandler;

        };
    }
}

#endif	
