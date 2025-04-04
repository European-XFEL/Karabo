#ifndef KARABO_UTIL_JSONTOHASHPARSER_HH
#define KARABO_UTIL_JSONTOHASHPARSER_HH

#include "karabo/data/types/Hash.hh"

namespace karabo {
    namespace util {

        /**
         * @brief Converts a JSON string representation into a Hash object.
         *
         * This function parses a JSON string and constructs a corresponding Hash object
         * representing the JSON structure. The JSON string is expected to follow the
         * JSON standard format.
         *
         * JSON Arrays of Mixed types are unsupported.
         *
         * JSON types are mapped to C++ types as below:
         * - JSON Integer -> long long
         * - JSON Decimal -> double
         * - JSON empty array -> empty std::vector<std::string>
         *
         * @param jsonString A string containing the JSON representation to be converted.
         *
         * @return A Hash object representing the parsed JSON structure.
         *
         * @throws KARABO_PARAMETER_EXCEPTION if the provided JSON string is invalid or cannot
         * be parsed into a Hash object.
         */
        karabo::data::Hash jsonToHash(const std::string& j);

        /**
         * @brief Generates an auto-start configuration Hash based on the provided initialization Hash.
         *
         * This function takes an initialization Hash representing the initial configuration
         * of components and constructs an auto-start configuration Hash based on it.
         *
         * The initialization Hash is expected to have been generated from the JSON for the init string used to
         * autostart devices on a karabo cpp server.
         *
         * An example conversion should look like this:
         *
         * initHash:
         *     'data_logger_manager_1' +
         *         'classId' => DataLoggerManager STRING
         *         'serverList' => karabo/dataLogger STRING
         *     'schema_printer1' +
         *         'classId' => SchemaPrinter STRING
         *
         * Above transforms to autoStart hash:
         *     'autoStart' @
         *     [0]
         *       'DataLoggerManager' +
         *         'deviceId' => data_logger_manager_1 STRING
         *         'serverList' => karabo/dataLogger STRING
         *     [1]
         *       'SchemaPrinter' +
         *         'deviceId' => schema_printer1 STRING
         *
         *
         * @param initHash The initialization Hash containing the initial configuration of components.
         *
         * @return A Hash object representing the auto-start configuration based on the provided initialization Hash.
         */
        karabo::data::Hash generateAutoStartHash(const karabo::data::Hash& initHash);

    } // namespace util
} // namespace karabo

#endif // KARABO_UTIL_JSONTOHASHPARSER_HH
