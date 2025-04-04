#include "JsonToHashParser.hh"

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

using karabo::data::Hash;

namespace karabo {
    namespace util {
        static void processJson(const nlohmann::json& j, Hash& result);
        static void processJsonObject(nlohmann::json::const_iterator& it, Hash& result);
        static void processJsonArray(nlohmann::json::const_iterator& it, Hash& result);
        static void processJsonValue(nlohmann::json::const_iterator& it, Hash& result);

        using JsonType = std::optional<nlohmann::json::value_t>;
        static JsonType getArrayType(const nlohmann::json& j);

        karabo::data::Hash jsonToHash(const std::string& j) {
            auto json = nlohmann::json::parse(j);
            auto result = Hash();
            processJson(json, result);
            return result;
        }

        Hash generateAutoStartHash(const Hash& initHash) {
            Hash autoStartHash;
            auto& autoStartEntries = autoStartHash.bindReference<std::vector<Hash>>("autoStart");

            for (const auto& node : initHash) {
                auto deviceId = node.getKey();
                auto details = node.getValue<Hash>(); // copy because we cannot
                                                      // modify initHash

                auto classId = details.get<std::string>("classId");
                details.erase("classId");

                details.set("deviceId", deviceId);
                autoStartEntries.emplace_back(classId, std::move(details));
            }

            return autoStartHash;
        }

        void processJson(const nlohmann::json& j, Hash& result) {
            if (j.is_object() == false) {
                auto message = std::string("Expecting a JSON object; instead found: \n") += j.dump();
                throw KARABO_PARAMETER_EXCEPTION(message);
            }
            for (auto it = j.begin(); it != j.end(); ++it) {
                // all iterators processed in this loop guaranteed to be a
                // valid k:v json pair (by the function precondition allowing
                // only objects as json strings)
                if (it.value().is_object()) {
                    processJsonObject(it, result);
                } else if (it.value().is_array()) {
                    processJsonArray(it, result);
                } else {
                    processJsonValue(it, result);
                }
            }
        }

        void processJsonObject(nlohmann::json::const_iterator& it, Hash& result) {
            auto& node = result.set(it.key(), Hash());
            processJson(it.value(), node.getValue<Hash>());
        }

        void processJsonArray(nlohmann::json::const_iterator& it, Hash& result) {
            auto type = getArrayType(it.value());
            if (type.has_value() == false) { // array is not homogenous}
                auto message = std::string("Only homogenous arrays are supported; offending json: ") +=
                      it.key() + ": " + it.value().dump();
                throw KARABO_PARAMETER_EXCEPTION(message);
            }
            switch (*type) {
                case nlohmann::json::value_t::string:
                    result.set<std::vector<std::string>>(it.key(), it.value());
                    break;
                case nlohmann::json::value_t::number_integer:
                case nlohmann::json::value_t::number_unsigned:
                    result.set<std::vector<long long>>(it.key(), it.value());
                    break;
                case nlohmann::json::value_t::number_float:
                    result.set<std::vector<double>>(it.key(), it.value());
                    break;
                case nlohmann::json::value_t::boolean:
                    result.set<std::vector<bool>>(it.key(), it.value());
                    break;
                case nlohmann::json::value_t::object: {
                    auto& node = result.set(it.key(), std::vector<Hash>(it.value().size(), Hash()));
                    auto& hashList = node.getValue<std::vector<Hash>>();
                    for (size_t i = 0; i < it.value().size(); ++i) {
                        processJson(it.value()[i], hashList[i]); // it.value()[i] is a json object
                                                                 // hashList.size()
                                                                 // ==
                                                                 // it.value().size()
                    }
                    break;
                }
                default:
                    auto message = std::string("UnsupportedJson type in array: \n") +=
                          it.key() + ": " + it.value().dump();
                    throw KARABO_PARAMETER_EXCEPTION(message);
                    break;
            }
        }

        void processJsonValue(nlohmann::json::const_iterator& it, Hash& result) {
            switch (it.value().type()) {
                case nlohmann::json::value_t::string:
                    result.set<std::string>(it.key(), it.value());
                    break;
                case nlohmann::json::value_t::number_integer:
                case nlohmann::json::value_t::number_unsigned:
                    result.set<long long>(it.key(), it.value());
                    break;
                case nlohmann::json::value_t::number_float:
                    result.set<double>(it.key(), it.value());
                    break;
                case nlohmann::json::value_t::boolean:
                    result.set<bool>(it.key(), it.value());
                    break;
                default:
                    auto message = std::string("UnsupportedJson type: \n") += it.key() + ": " + it.value().dump();
                    throw KARABO_PARAMETER_EXCEPTION(message);
                    break;
            }
        }

        JsonType getArrayType(const nlohmann::json& j) {
            if (j.empty()) {
                return {nlohmann::json::value_t::string};
            }

            auto firstType = j[0].type();
            for (size_t i = 0; i < j.size(); ++i) {
                if (firstType != j[i].type()) {
                    return {};
                }
            }
            return {firstType};
        }
    } // namespace util
} // namespace karabo
