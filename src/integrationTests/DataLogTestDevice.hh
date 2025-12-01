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
#ifndef DATALOGTESTDEVICE_HH
#define DATALOGTESTDEVICE_HH

#include <karabo/karabo.hpp>

namespace karabo {

    class DataLogTestDevice : public karabo::core::Device {
       public:
        KARABO_CLASSINFO(DataLogTestDevice, "DataLogTestDevice",
                         "integrationTests-" + karabo::util::Version::getVersion())

        static const karabo::data::Epochstamp THREE_DAYS_AGO;

        static void expectedParameters(karabo::data::Schema& expected);

        DataLogTestDevice(const karabo::data::Hash& input);
        virtual ~DataLogTestDevice();

       private:
        void initialize();
        void slotIncreaseValue();
        void slotUpdateConfigGeneric(const karabo::data::Hash conf);
        void slotUpdateSchema(const karabo::data::Schema sch);
    };

} // namespace karabo

#endif /* DATALOGTESTDEVICE_HH */
