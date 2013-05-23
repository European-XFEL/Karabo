/*
 * $Id$
 *
 * File:   Input.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 29, 2011, 6:04 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_XIP_ABSTRACTINPUT_HH
#define	EXFEL_XIP_ABSTRACTINPUT_HH

#include <boost/function.hpp>

#include <exfel/util/Factory.hh>

namespace exfel {

    namespace xip {

        class AbstractInput : public boost::enable_shared_from_this<AbstractInput> {

        public:

            EXFEL_CLASSINFO(AbstractInput, "AbstractInput", "1.0")
            EXFEL_FACTORY_BASE_CLASS

            typedef boost::function<void () > IOEventHandler;
            typedef boost::function<void (const boost::shared_ptr<AbstractInput>&) > CanReadEventHandler;

            AbstractInput() {
            }

            virtual ~AbstractInput() {
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(exfel::util::Schema& expected) {
                using namespace exfel::util;

                UINT32_ELEMENT(expected).key("minData")
                        .displayedName("Minimum number of data")
                        .description("The number of elements to be read before any computation is started (0 = all)")
                        .assignmentOptional().defaultValue(1)
                        .commit();

            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const exfel::util::Hash& input) {
                input.get("minData", m_nData);
            }

            void registerIOEventHandler(const IOEventHandler& ioEventHandler) {
                m_ioEventHandler = ioEventHandler;
            }

            void registerCanReadEventHandler(const CanReadEventHandler& canReadEventHandler) {
                m_canReadEventHandler = canReadEventHandler;
            }

            virtual bool needsDeviceConnection() const {
                return false;
            }

            virtual std::vector<exfel::util::Hash> getConnectedOutputChannels() {
                return std::vector<exfel::util::Hash > ();
            }

            virtual void connectNow(const std::string& instanceId, const exfel::util::Hash& outputChannelInfo) {
            }

            virtual exfel::util::Hash getIOStatus() const {
                return exfel::util::Hash();
            }

            virtual bool canCompute() {
                return true;
            }

            virtual void onComputeFinished() {
            }

            virtual void setEndOfStream() const {
            }

        protected:

            unsigned int getMinimumNumberOfData() const {
                return m_nData;
            }

            void triggerCanReadEvent() {
                if (m_canReadEventHandler) m_canReadEventHandler(shared_from_this());
            }

            void triggerIOEvent() const {
                if (m_ioEventHandler) m_ioEventHandler();
            }

        private:

            unsigned int m_nData;

            CanReadEventHandler m_canReadEventHandler;
            IOEventHandler m_ioEventHandler;





        };
    }
}

#endif	
