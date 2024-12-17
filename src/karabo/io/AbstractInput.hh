/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on June 05, 2012, 6:04 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_IO_ABSTRACTINPUT_HH
#define KARABO_IO_ABSTRACTINPUT_HH

#include <boost/any.hpp>
#include <functional>
#include <karabo/io/InputHandler.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>

namespace karabo {
    namespace io {

        /**
         * @class AbstractInput
         * @bried The AbstractInput class is the base for input classes in Karabo
         *
         * Their specific implementation defines how the input acquires data. This
         * can be either through a network or in memory connection, or through a
         * data source accessing persited data.
         */
        class AbstractInput : public std::enable_shared_from_this<AbstractInput> {
           public:
            KARABO_CLASSINFO(AbstractInput, "AbstractInput", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected) {}

            AbstractInput() {}

            AbstractInput(const karabo::util::Hash& configuration) {}

            virtual ~AbstractInput() {}

            /**
             * Reconfigure the input, e.g. to use a different data source
             * @param input
             */
            virtual void reconfigure(const karabo::util::Hash& input) {}

            /**
             * Register a SignalSlotable instance to this input
             * @param instanceId
             */
            void setInstanceId(const std::string& instanceId) {
                m_instanceId = instanceId;
            }

            /**
             * Get the registered SignalSlotable instance
             * @return
             */
            const std::string& getInstanceId() const {
                return m_instanceId;
            }

            /**
             * Set language handling input parsing
             * @param language may be C++ or PYTHOn
             * @param inputType type of input
             */
            void setInputHandlerType(const std::string& language, const std::string& inputType) {
                std::string capitalType = boost::algorithm::to_upper_copy(language);
                if (capitalType == "C++")
                    m_handler = karabo::util::Factory<InputHandler>::create("CppInputHandler" + inputType,
                                                                            shared_from_this());
                else if (capitalType == "PYTHON")
                    m_handler = karabo::util::Factory<InputHandler>::create("PythonInputHandler" + inputType,
                                                                            shared_from_this());
                else
                    throw KARABO_PARAMETER_EXCEPTION(
                          "Handler type " + language +
                          " is not supported.  Supported types (case-insensitive) are C++, Python");
            }

            /**
             * Get the registered input Handler
             * @return
             */
            InputHandler::Pointer getInputHandler() {
                return m_handler;
            }

            /**
             * Register a handler to be called for I/O events
             */
            void registerIOEventHandler(const boost::any& ioEventHandler) {
                if (!m_handler)
                    throw KARABO_LOGIC_EXCEPTION("Handler storage not initialized: call 'setInputHandlerType' first.");
                m_handler->registerIOEventHandler(ioEventHandler);
            }

            /**
             * Register a handler to be called for end of stream events. End of stream event are used
             * to signify that a group of related data tokens is complete and that a new group of
             * data token follow
             * @param endOfStreamEventHandler
             */
            void registerEndOfStreamEventHandler(const boost::any& endOfStreamEventHandler) {
                if (!m_handler)
                    throw KARABO_LOGIC_EXCEPTION("Handler storage not initialized: call 'setInputHandlerType' first.");
                m_handler->registerEndOfStreamEventHandler(endOfStreamEventHandler);
            }

            /**
             * Return if this input needs to used in the context of a device
             * @return
             */
            virtual bool needsDeviceConnection() const { // TODO Check if we can get rid of this
                return false;
            }

            /**
             * Get the output channels connected to this input
             * @return
             */
            virtual std::vector<karabo::util::Hash> getConnectedOutputChannels() {
                return std::vector<karabo::util::Hash>();
                std::vector<int> v;
            }

            /**
             * Connect this input to an output channel as specified by its configuration.
             * @param outputChannelInfo
             */
            virtual void connect(const karabo::util::Hash& outputChannelInfo) {}

            /**
             * Disconnect the output channel specified by its configuration
             * @param outputChannelInfo
             */
            virtual void disconnect(const karabo::util::Hash& outputChannelInfo) {}

            /**
             * Should return true if the input can handle more data
             * @return
             */
            virtual bool canCompute() const {
                return true;
            }

            /**
             * Update the input to an receiving state
             */
            virtual void update() {}

            /**
             * Check if the input responds to end of stream events, e.g. by calling
             * the registered handler.
             * @return
             */
            virtual bool respondsToEndOfStream() {
                return true;
            }

           protected:
            /**
             * Trigger an I/O event in the event handler
             */
            void triggerIOEvent() {
                if (m_handler) {
                    m_handler->triggerIOEvent();
                }
            }

            /**
             * Trigger an end of stream event in the event handler.
             */
            void triggerEndOfStreamEvent() {
                if (m_handler) {
                    m_handler->triggerEndOfStreamEvent();
                }
            }

           private:
            std::shared_ptr<InputHandler> m_handler;
            std::string m_instanceId;
        };
    } // namespace io
} // namespace karabo

#endif
