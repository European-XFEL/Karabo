/*
 * Author: <gero.flucke@xfel.eu>
 *
 * Created on March 31, 2025
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
 *
 */

#ifndef KARABO_UTIL_NDARRAYELEMENT_HH
#define KARABO_UTIL_NDARRAYELEMENT_HH

#include <vector>

#include "CustomNodeElement.hh"
#include "NDArray.hh"
#include "StringTools.hh"
#include "Units.hh"

namespace karabo {
    namespace util {

        class Schema;

        class NDArrayDescription {
           public:
            // Let NDArrayDescription::classInfo().getClassId() match the classId of our NDArray.
            // So Schema::getCustomNodeClass(..) will be "NDArray".
            KARABO_CLASSINFO(NDArrayDescription, NDArray::classInfo().getClassId(), "3.0");

            static void expectedParameters(karabo::util::Schema& s);
        };

        class NDArrayElement : public karabo::util::CustomNodeElement<NDArrayElement, NDArrayDescription> {
            typedef karabo::util::CustomNodeElement<NDArrayElement, NDArrayDescription> ParentType;

           public:
            NDArrayElement(karabo::util::Schema& s) : ParentType(s) {}

            NDArrayElement& dtype(const karabo::util::Types::ReferenceType type) {
                return setDefaultValue("type", static_cast<int>(type));
            }

            NDArrayElement& shape(const std::string& shp) {
                std::vector<unsigned long long> tmp = karabo::util::fromString<unsigned long long, std::vector>(shp);
                return shape(tmp);
            }

            NDArrayElement& shape(const std::vector<unsigned long long>& shp) {
                return setDefaultValue("shape", shp);
            }

            NDArrayElement& unit(const UnitType& unit) {
                return setUnit("data", unit);
            }

            NDArrayElement& metricPrefix(const MetricPrefixType& metricPrefix) {
                return setMetricPrefix("data", metricPrefix);
            }

            void commit() {
                // As NDArrayElement is only used for channel descriptions, it should always be read only.
                readOnly();
                ParentType::commit();
            }
        };

        typedef NDArrayElement NDARRAY_ELEMENT;
    } // namespace util
} // namespace karabo

#endif
