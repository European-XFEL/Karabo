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
/*
 * File:   Units.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 29, 2013, 12:13 PM
 */

#ifndef KARABO_DATA_TYPES_UNITS_HH
#define KARABO_DATA_TYPES_UNITS_HH

#include <boost/algorithm/string.hpp>

#include "Exception.hh"
#include "karaboDll.hh"

namespace karabo {

    namespace data {

        enum class MetricPrefix {

            YOTTA,
            ZETTA,
            EXA,
            PETA,
            TERA,
            GIGA,
            MEGA,
            KILO,
            HECTO,
            DECA,
            NONE,
            DECI,
            CENTI,
            MILLI,
            MICRO,
            NANO,
            PICO,
            FEMTO,
            ATTO,
            ZEPTO,
            YOCTO
        };

        // Compile time conversion

        template <MetricPrefix MetricEnum>
        inline std::pair<std::string, std::string> getMetricPrefix() {
            throw KARABO_PARAMETER_EXCEPTION("");
        }

#define _KARABO_HELPER_MACRO(metricEnum, symbol)                                             \
    template <>                                                                              \
    inline std::pair<std::string, std::string> getMetricPrefix<MetricPrefix::metricEnum>() { \
        std::string name(#metricEnum);                                                       \
        boost::to_lower(name);                                                               \
        return std::make_pair(name, symbol);                                                 \
    }

        _KARABO_HELPER_MACRO(YOTTA, "Y")
        _KARABO_HELPER_MACRO(ZETTA, "Z")
        _KARABO_HELPER_MACRO(EXA, "E")
        _KARABO_HELPER_MACRO(PETA, "P")
        _KARABO_HELPER_MACRO(TERA, "T")
        _KARABO_HELPER_MACRO(GIGA, "G")
        _KARABO_HELPER_MACRO(MEGA, "M")
        _KARABO_HELPER_MACRO(KILO, "k")
        _KARABO_HELPER_MACRO(HECTO, "h")
        _KARABO_HELPER_MACRO(DECA, "da")
        template <>
        inline std::pair<std::string, std::string> getMetricPrefix<MetricPrefix::NONE>() {
            return std::make_pair("", "");
        }

        _KARABO_HELPER_MACRO(DECI, "d")
        _KARABO_HELPER_MACRO(CENTI, "c")
        _KARABO_HELPER_MACRO(MILLI, "m")
        _KARABO_HELPER_MACRO(MICRO, "u")
        _KARABO_HELPER_MACRO(NANO, "n")
        _KARABO_HELPER_MACRO(PICO, "p")
        _KARABO_HELPER_MACRO(FEMTO, "f")
        _KARABO_HELPER_MACRO(ATTO, "a")
        _KARABO_HELPER_MACRO(ZEPTO, "z")
        _KARABO_HELPER_MACRO(YOCTO, "y")

#undef _KARABO_HELPER_MACRO

        // Runtime conversion
        inline std::pair<std::string, std::string> getMetricPrefix(const MetricPrefix metricPrefix) {
#define _KARABO_HELPER_MACRO(enumerator) \
    case MetricPrefix::enumerator:       \
        return getMetricPrefix<MetricPrefix::enumerator>();
            switch (metricPrefix) {
                _KARABO_HELPER_MACRO(YOTTA)
                _KARABO_HELPER_MACRO(ZETTA)
                _KARABO_HELPER_MACRO(EXA)
                _KARABO_HELPER_MACRO(PETA)
                _KARABO_HELPER_MACRO(TERA)
                _KARABO_HELPER_MACRO(GIGA)
                _KARABO_HELPER_MACRO(MEGA)
                _KARABO_HELPER_MACRO(KILO)
                _KARABO_HELPER_MACRO(HECTO)
                _KARABO_HELPER_MACRO(DECA)
                _KARABO_HELPER_MACRO(NONE)
                _KARABO_HELPER_MACRO(DECI)
                _KARABO_HELPER_MACRO(CENTI)
                _KARABO_HELPER_MACRO(MILLI)
                _KARABO_HELPER_MACRO(MICRO)
                _KARABO_HELPER_MACRO(NANO)
                _KARABO_HELPER_MACRO(PICO)
                _KARABO_HELPER_MACRO(FEMTO)
                _KARABO_HELPER_MACRO(ATTO)
                _KARABO_HELPER_MACRO(ZEPTO)
                _KARABO_HELPER_MACRO(YOCTO)
                default:
                    throw KARABO_PARAMETER_EXCEPTION("No string translation registered for given metricPrefix");
            }
#undef _KARABO_HELPER_MACRO
        }

        enum class Unit {

            NUMBER,
            COUNT,
            METER,
            GRAM,
            SECOND,
            AMPERE,
            KELVIN,
            MOLE,
            CANDELA,
            HERTZ,
            RADIAN,
            DEGREE,
            STERADIAN,
            NEWTON,
            PASCAL,
            JOULE,
            ELECTRONVOLT,
            WATT,
            COULOMB,
            VOLT,
            FARAD,
            OHM,
            SIEMENS,
            WEBER,
            TESLA,
            HENRY,
            DEGREE_CELSIUS,
            LUMEN,
            LUX,
            BECQUEREL,
            GRAY,
            SIEVERT,
            KATAL,
            MINUTE,
            HOUR,
            DAY,
            YEAR,
            BAR,
            PIXEL,
            BYTE,
            BIT,
            METER_PER_SECOND,
            VOLT_PER_SECOND,
            AMPERE_PER_SECOND,
            PERCENT,
            NOT_ASSIGNED,
            REVOLUTIONS_PER_MINUTE
        };

        // Compile time conversion

        template <Unit UnitEnum>
        inline std::pair<std::string, std::string> getUnit() {
            throw KARABO_PARAMETER_EXCEPTION("");
        }

#define _KARABO_HELPER_MACRO(enumerator, symbol)                             \
    template <>                                                              \
    inline std::pair<std::string, std::string> getUnit<Unit::enumerator>() { \
        std::string name(#enumerator);                                       \
        boost::to_lower(name);                                               \
        return std::make_pair(name, symbol);                                 \
    }

        _KARABO_HELPER_MACRO(NUMBER, "")
        _KARABO_HELPER_MACRO(COUNT, "#")
        _KARABO_HELPER_MACRO(METER, "m")
        _KARABO_HELPER_MACRO(GRAM, "g")
        _KARABO_HELPER_MACRO(SECOND, "s")
        _KARABO_HELPER_MACRO(AMPERE, "A")
        _KARABO_HELPER_MACRO(KELVIN, "K")
        _KARABO_HELPER_MACRO(MOLE, "mol")
        _KARABO_HELPER_MACRO(CANDELA, "cd")
        _KARABO_HELPER_MACRO(HERTZ, "Hz")
        _KARABO_HELPER_MACRO(RADIAN, "rad")
        _KARABO_HELPER_MACRO(DEGREE, "deg")
        _KARABO_HELPER_MACRO(STERADIAN, "sr")
        _KARABO_HELPER_MACRO(NEWTON, "N")
        _KARABO_HELPER_MACRO(PASCAL, "Pa")
        _KARABO_HELPER_MACRO(JOULE, "J")
        _KARABO_HELPER_MACRO(ELECTRONVOLT, "eV")
        _KARABO_HELPER_MACRO(WATT, "W")
        _KARABO_HELPER_MACRO(COULOMB, "C")
        _KARABO_HELPER_MACRO(VOLT, "V")
        _KARABO_HELPER_MACRO(FARAD, "F")
        _KARABO_HELPER_MACRO(OHM, "Ω")
        _KARABO_HELPER_MACRO(SIEMENS, "S")
        _KARABO_HELPER_MACRO(WEBER, "Wb")
        _KARABO_HELPER_MACRO(TESLA, "T")
        _KARABO_HELPER_MACRO(HENRY, "H")
        _KARABO_HELPER_MACRO(DEGREE_CELSIUS, "degC")
        _KARABO_HELPER_MACRO(LUMEN, "lm")
        _KARABO_HELPER_MACRO(LUX, "lx")
        _KARABO_HELPER_MACRO(BECQUEREL, "Bq")
        _KARABO_HELPER_MACRO(GRAY, "Gy")
        _KARABO_HELPER_MACRO(SIEVERT, "Sv")
        _KARABO_HELPER_MACRO(KATAL, "kat")
        _KARABO_HELPER_MACRO(MINUTE, "min")
        _KARABO_HELPER_MACRO(HOUR, "h")
        _KARABO_HELPER_MACRO(DAY, "d")
        _KARABO_HELPER_MACRO(YEAR, "a")
        _KARABO_HELPER_MACRO(BAR, "bar")
        _KARABO_HELPER_MACRO(PIXEL, "px")
        _KARABO_HELPER_MACRO(BYTE, "B")
        _KARABO_HELPER_MACRO(BIT, "bit")
        _KARABO_HELPER_MACRO(METER_PER_SECOND, "m/s")
        _KARABO_HELPER_MACRO(VOLT_PER_SECOND, "V/s")
        _KARABO_HELPER_MACRO(AMPERE_PER_SECOND, "A/s")
        _KARABO_HELPER_MACRO(PERCENT, "%")
        _KARABO_HELPER_MACRO(NOT_ASSIGNED, "N_A")
        _KARABO_HELPER_MACRO(REVOLUTIONS_PER_MINUTE, "rpm")

#undef _KARABO_HELPER_MACRO

        // Runtime conversion
        inline std::pair<std::string, std::string> getUnit(const Unit unit) {
#define _KARABO_HELPER_MACRO(enumerator) \
    case Unit::enumerator:               \
        return getUnit<Unit::enumerator>();
            switch (unit) {
                _KARABO_HELPER_MACRO(NUMBER)
                _KARABO_HELPER_MACRO(COUNT)
                _KARABO_HELPER_MACRO(METER)
                _KARABO_HELPER_MACRO(GRAM)
                _KARABO_HELPER_MACRO(SECOND)
                _KARABO_HELPER_MACRO(AMPERE)
                _KARABO_HELPER_MACRO(KELVIN)
                _KARABO_HELPER_MACRO(MOLE)
                _KARABO_HELPER_MACRO(CANDELA)
                _KARABO_HELPER_MACRO(HERTZ)
                _KARABO_HELPER_MACRO(RADIAN)
                _KARABO_HELPER_MACRO(DEGREE)
                _KARABO_HELPER_MACRO(STERADIAN)
                _KARABO_HELPER_MACRO(NEWTON)
                _KARABO_HELPER_MACRO(PASCAL)
                _KARABO_HELPER_MACRO(JOULE)
                _KARABO_HELPER_MACRO(ELECTRONVOLT)
                _KARABO_HELPER_MACRO(WATT)
                _KARABO_HELPER_MACRO(COULOMB)
                _KARABO_HELPER_MACRO(VOLT)
                _KARABO_HELPER_MACRO(FARAD)
                _KARABO_HELPER_MACRO(OHM)
                _KARABO_HELPER_MACRO(SIEMENS)
                _KARABO_HELPER_MACRO(WEBER)
                _KARABO_HELPER_MACRO(TESLA)
                _KARABO_HELPER_MACRO(HENRY)
                _KARABO_HELPER_MACRO(DEGREE_CELSIUS)
                _KARABO_HELPER_MACRO(LUMEN)
                _KARABO_HELPER_MACRO(LUX)
                _KARABO_HELPER_MACRO(BECQUEREL)
                _KARABO_HELPER_MACRO(GRAY)
                _KARABO_HELPER_MACRO(SIEVERT)
                _KARABO_HELPER_MACRO(KATAL)
                _KARABO_HELPER_MACRO(MINUTE)
                _KARABO_HELPER_MACRO(HOUR)
                _KARABO_HELPER_MACRO(DAY)
                _KARABO_HELPER_MACRO(YEAR)
                _KARABO_HELPER_MACRO(BAR)
                _KARABO_HELPER_MACRO(PIXEL)
                _KARABO_HELPER_MACRO(BYTE)
                _KARABO_HELPER_MACRO(BIT)
                _KARABO_HELPER_MACRO(METER_PER_SECOND)
                _KARABO_HELPER_MACRO(VOLT_PER_SECOND)
                _KARABO_HELPER_MACRO(AMPERE_PER_SECOND)
                _KARABO_HELPER_MACRO(PERCENT)
                _KARABO_HELPER_MACRO(NOT_ASSIGNED)
                _KARABO_HELPER_MACRO(REVOLUTIONS_PER_MINUTE)
                default:
                    throw KARABO_PARAMETER_EXCEPTION("No string translation registered for given unit");
            }
#undef _KARABO_HELPER_MACRO
        }
    } // namespace data
} // namespace karabo
#endif
