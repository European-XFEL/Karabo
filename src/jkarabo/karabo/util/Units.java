/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package karabo.util;

/**
 *
 * @author Sergey Esenov serguei.essenov at xfel.eu
 */
public class Units {

    public static class Entry<T0, T1> {

        public T0 name;
        public T1 symbol;

        public Entry(T0 name, T1 symbol) {
            this.name = name;
            this.symbol = symbol;
        }
    }

    public enum MetricPrefix {

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
        YOCTO;
    }

    public static Entry<String, String> getMetricPrefix(MetricPrefix metricPrefix) {
        switch (metricPrefix) {
            case YOTTA:
                return new Entry<>("yotta", "Y");
            case ZETTA:
                return new Entry<>("zetta", "Z");
            case EXA:
                return new Entry<>("exa", "E");
            case PETA:
                return new Entry<>("peta", "P");
            case TERA:
                return new Entry<>("tera", "T");
            case GIGA:
                return new Entry<>("giga", "G");
            case MEGA:
                return new Entry<>("mega", "M");
            case KILO:
                return new Entry<>("kilo", "k");
            case HECTO:
                return new Entry<>("hecto", "h");
            case DECA:
                return new Entry<>("deca", "da");
            case NONE:
                return new Entry<>("", "");
            case DECI:
                return new Entry<>("deci", "d");
            case CENTI:
                return new Entry<>("centi", "c");
            case MILLI:
                return new Entry<>("milli", "m");
            case MICRO:
                return new Entry<>("micro", "u");
            case NANO:
                return new Entry<>("nano", "n");
            case PICO:
                return new Entry<>("pico", "p");
            case FEMTO:
                return new Entry<>("femto", "f");
            case ATTO:
                return new Entry<>("atto", "a");
            case ZEPTO:
                return new Entry<>("zepto", "z");
            case YOCTO:
                return new Entry<>("yocto", "y");
            default:
                throw new RuntimeException("No string translation registered for given metricPrefix");
        }
    }

    public enum Unit {

        METER,
        GRAM,
        SECOND,
        AMPERE,
        KELVIN,
        MOLE,
        CANDELA,
        HERTZ,
        RADIAN,
        STERADIAN,
        NEWTON,
        PASCAL,
        JOULE,
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
        VOLT_PER_SECOND;
    }

    public static Entry<String, String> getUnit(Unit unit) {
        switch (unit) {
            case METER:
                return new Entry<>("meter", "m");
            case GRAM:
                return new Entry<>("gram", "g");
            case SECOND:
                return new Entry<>("second", "s");
            case AMPERE:
                return new Entry<>("ampere", "A");
            case KELVIN:
                return new Entry<>("kelvin", "K");
            case MOLE:
                return new Entry<>("mole", "mol");
            case CANDELA:
                return new Entry<>("candela", "cd");
            case HERTZ:
                return new Entry<>("hertz", "Hz");
            case RADIAN:
                return new Entry<>("radian", "rad");
            case STERADIAN:
                return new Entry<>("steradian", "sr");
            case NEWTON:
                return new Entry<>("newton", "N");
            case PASCAL:
                return new Entry<>("pascal", "Pa");
            case JOULE:
                return new Entry<>("joule", "J");
            case WATT:
                return new Entry<>("watt", "W");
            case COULOMB:
                return new Entry<>("coulomb", "C");
            case VOLT:
                return new Entry<>("volt", "V");
            case FARAD:
                return new Entry<>("farad", "F");
            case OHM:
                return new Entry<>("ohm", "Ω");
            case SIEMENS:
                return new Entry<>("siemens", "S");
            case WEBER:
                return new Entry<>("weber", "Wb");
            case TESLA:
                return new Entry<>("tesla", "T");
            case HENRY:
                return new Entry<>("henry", "H");
            case DEGREE_CELSIUS:
                return new Entry<>("degree_celsius", "°C");
            case LUMEN:
                return new Entry<>("lumen", "lm");
            case LUX:
                return new Entry<>("lux", "lx");
            case BECQUEREL:
                return new Entry<>("becquerel", "Bq");
            case GRAY:
                return new Entry<>("gray", "Gy");
            case SIEVERT:
                return new Entry<>("sievert", "Sv");
            case KATAL:
                return new Entry<>("katal", "kat");
            case MINUTE:
                return new Entry<>("minute", "min");
            case HOUR:
                return new Entry<>("hour", "h");
            case DAY:
                return new Entry<>("day", "d");
            case YEAR:
                return new Entry<>("year", "a");
            case BAR:
                return new Entry<>("bar", "bar");
            case PIXEL:
                return new Entry<>("pixel", "px");
            case BYTE:
                return new Entry<>("byte", "B");
            case BIT:
                return new Entry<>("bit", "B");
            case METER_PER_SECOND:
                return new Entry<>("meter_per_second", "m/s");
            case VOLT_PER_SECOND:
                return new Entry<>("volt_per_second", "V/s");
            default:
                throw new RuntimeException("No string translation registered for given unit");
        }
    }
}
