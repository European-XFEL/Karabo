/*
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
#ifndef DATALOGNANTESTDEVICE_HH
#define DATALOGNANTESTDEVICE_HH

#include <karabo/karabo.hpp>

namespace karabo {
    // A device with float and double properties without limits to be able to set inf and nan.
    // Otherwise copy PropertyTest behaviour as needed for testNans().
    class DataLogNanTestDevice : public karabo::core::Device {
       public:
        KARABO_CLASSINFO(DataLogNanTestDevice, "DataLogNanTestDevice",
                         "integrationTests-" + karabo::util::Version::getVersion())

        static void expectedParameters(karabo::data::Schema& expected);

        DataLogNanTestDevice(const karabo::data::Hash& input);

        void preReconfigure(karabo::data::Hash& incomingReconfiguration);
    };

} // namespace karabo

#endif /* DATALOGNANTESTDEVICE_HH */
