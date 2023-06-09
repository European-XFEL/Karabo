/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 30, 2011, 2:51 PM
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

#ifndef KARABO_XMS_STATICS_HH
#define KARABO_XMS_STATICS_HH

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xms {

        /**
         * The Statics class.
         */
        class Statics {
           public:
            /**
             * Default constructor.
             */
            Statics(){};

            /**
             * Destructor.
             */
            virtual ~Statics(){};

            static std::string generateUUID() {
                return boost::uuids::to_string(m_uuidGenerator());
            }

            static unsigned int generateServerPort() {
                return 10000 + ((m_serverPorts++) % 50000);
            }

           private: // members
            static boost::uuids::random_generator m_uuidGenerator;

            static unsigned int m_serverPorts;

           private: // functions
        };

    } // namespace xms
} // namespace karabo

#endif
