/*
 * $Id$
 *
 * File:   AbstractOutput.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on June 05, 2011, 6:04 PM
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


#ifndef KARABO_IO_ABSTRACTOUTPUT_HH
#define KARABO_IO_ABSTRACTOUTPUT_HH

#include <any>
#include <functional>
#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>

#include "OutputHandler.hh"

namespace karabo {
    namespace io {

        class AbstractOutput {
           public:
            KARABO_CLASSINFO(AbstractOutput, "AbstractOutput", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {}

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            AbstractOutput(const karabo::util::Hash& configuration) {}

            AbstractOutput() {}

            virtual ~AbstractOutput() {}

            //            void setOutputHandlerType(const std::string& handlerType) {
            //                std::string capitalType = boost::algorithm::to_upper_copy(handlerType);
            //                if (capitalType == "C++")
            //                    m_handlers =
            //                    karabo::util::Factory<OutputHandler>::create<AbstractOutput::Pointer>("CppOutputHandler",
            //                    shared_from_this());
            //                else if (capitalType == "PYTHON")
            //                    m_handlers =
            //                    karabo::util::Factory<OutputHandler>::create<AbstractOutput::Pointer>("PythonOutputHandler",
            //                    shared_from_this());
            //                else
            //                    throw KARABO_PARAMETER_EXCEPTION("Handler type " + handlerType + " is not supported.
            //                    Supported types (case-insensitive) are C++, Python");
            //            }

            //            OutputHandler::Pointer getOutputHandler() {
            //                return m_handlers;
            //            }

            //            void registerIOEventHandler(const std::any& ioEventHandler) {
            //                if (!m_handlers)
            //                    throw KARABO_LOGIC_EXCEPTION("Handler storage not initialized: call 'setOutputHandler'
            //                    first.");
            //                m_handlers->registerIOEventHandler(ioEventHandler);
            //            }

            // virtual void onInputAvailable(const std::string& instanceId) {
            // }

            //            virtual karabo::util::Hash getInformation() const {
            //                return karabo::util::Hash();
            //            }

            virtual void update() {}
        };
    } // namespace io
} // namespace karabo

#endif
