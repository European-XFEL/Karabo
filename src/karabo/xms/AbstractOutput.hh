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


#ifndef EXFEL_XMS_ABSTRACTOUTPUT_HH
#define	EXFEL_XMS_ABSTRACTOUTPUT_HH

#include <boost/function.hpp>

#include <karabo/util/Factory.hh>

namespace exfel {

    namespace xms {

        class AbstractOutput : public boost::enable_shared_from_this<AbstractOutput> {
        public:

            EXFEL_CLASSINFO(AbstractOutput, "AbstractOutput", "1.0")
            EXFEL_FACTORY_BASE_CLASS

            typedef boost::function<void (const boost::shared_ptr<AbstractOutput>&) > IOEventHandler;

            AbstractOutput() {
            }

            virtual ~AbstractOutput() {
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(exfel::util::Schema& expected) {

            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const exfel::util::Hash& input) {
            }

            void setInstanceId(const std::string& instanceId) {
                m_instanceId = instanceId;
            }
            
             const std::string& getInstanceId() const {
                return m_instanceId;
            }

            void registerIOEventHandler(const IOEventHandler& ioEventHandler) {
                m_ioEventHandler = ioEventHandler;
            }

            virtual void onInputAvailable(const std::string& instanceId) {

            }

            virtual exfel::util::Hash getInformation() const {
                return exfel::util::Hash();
            }

            virtual void update() {
            }

            virtual bool canCompute() = 0;

        protected:

            void triggerIOEvent() {
                if (m_ioEventHandler) m_ioEventHandler(shared_from_this());
            }

        private:

            IOEventHandler m_ioEventHandler;
            std::string m_instanceId;
        };
    }
}

#endif	
