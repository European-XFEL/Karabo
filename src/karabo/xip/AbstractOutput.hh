/*
 * $Id$
 *
 * File:   Output.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 29, 2011, 6:04 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_XIP_ABSTRACTOUTPUT_HH
#define	EXFEL_XIP_ABSTRACTOUTPUT_HH

#include <exfel/util/Factory.hh>

namespace exfel {

    namespace xip {

        class AbstractOutput {

        public:

            EXFEL_CLASSINFO(AbstractOutput, "AbstractOutput", "1.0")
            EXFEL_FACTORY_BASE_CLASS

            typedef boost::function<void () > IOEventHandler;

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

            void registerIOEventHandler(const IOEventHandler& ioEventHandler) {
                m_ioEventHandler = ioEventHandler;
            }

            virtual void onInputAvailable(const std::string& instanceId) {

            }

            virtual exfel::util::Hash getInformation() const {
                return exfel::util::Hash();
            }

            virtual void onComputeFinished() {
            }

            virtual bool canCompute() = 0;

        protected:

            void triggerIOEvent() const {
                if (m_ioEventHandler) m_ioEventHandler();
            }

        private:


            IOEventHandler m_ioEventHandler;

        };
    }
}

#endif	
