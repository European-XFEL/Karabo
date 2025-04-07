/*
 * $Id$
 *
 * File:   Output.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 29, 2011, 9:44 AM
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


#ifndef KARABO_DATA_IO_OUTPUT_HH
#define KARABO_DATA_IO_OUTPUT_HH

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/schema/SimpleElement.hh"

namespace karabo {

    namespace data {

        /**
         * @class Output
         * @brief The Output class provides a base for outputs in the Karabo
         *        system.
         *
         * Outputs act as sinks. Their specific implementation defines what
         * is done with data written to them. They may e.g. pass it on a network
         * or in-memory connection or persist it e.g. to a specific file using
         * a specific format.
         */
        template <class T>
        class Output {
           protected:
            bool m_appendModeEnabled;

           public:
            KARABO_CLASSINFO(Output, "Output", "1.0")

            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::data::Schema& expected) {
                using namespace karabo::data;

                BOOL_ELEMENT(expected)
                      .key("enableAppendMode")
                      .description(
                            "If set to true a different internal structure is used, which buffers consecutive "
                            "calls to write(). The update() function must then be called to trigger final outputting "
                            "of the accumulated sequence of data.")
                      .displayedName("Enable append mode")
                      .assignmentOptional()
                      .defaultValue(false)
                      .init()
                      .commit();
            }

            Output(const karabo::data::Hash& config) {
                config.get<bool>("enableAppendMode", m_appendModeEnabled);
            }

            virtual ~Output() {}

            /**
             * Write an object to the output channel. Output channels may accept
             * multiple writes before an update leads to processing of the written
             * data.
             * @param object
             */
            virtual void write(const T& object) = 0;

            /**
             * Calling update tells that the output channel should cycle its
             * state to be ready for new data written to it.
             */
            virtual void update() {}
        };
    } // namespace data
} // namespace karabo

#endif
