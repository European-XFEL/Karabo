#include <gtest/gtest.h>

#include <karabo/karabo.hpp>

using karabo::data::Hash;


namespace karabo {
    namespace util {
        // http://google.github.io/googletest/advanced.html#teaching-googletest-how-to-print-your-values
        void PrintTo(const Hash& h, std::ostream* os) {
            *os << "Hash {" << h << "}";
        }
    } // namespace util
} // namespace karabo

testing::AssertionResult isEqual(const Hash& h1, const Hash& h2) {
    if (h1.fullyEquals(h2)) {
        return testing::AssertionSuccess();
    } else {
        return testing::AssertionFailure() << "Hashes not equal: \n"
                                           << "Hash1: \n"
                                           << h1 << "\n Hash2: \n"
                                           << h2 << std::endl;
    }
}

// test parameterization
using JsonString = const char*;
using Expected = Hash;
class testJsonToHash : public ::testing::TestWithParam<std::pair<JsonString, Expected>> {};
class testJsonToHash_throw : public ::testing::TestWithParam<JsonString> {};

INSTANTIATE_TEST_SUITE_P(
      SupportedJson, testJsonToHash,
      ::testing::Values(                                                                                            //
            std::make_pair(R"({})", Hash()),                                                                        //
            std::make_pair(R"({"k1":"value"})", Hash{"k1", "value"}),                                               //
            std::make_pair(R"({"k1":""})", Hash{"k1", ""}),                                                         //
            std::make_pair(R"({"k1":true})", Hash{"k1", true}),                                                     //
            std::make_pair(R"({"k1":1})", Hash{"k1", 1ll}),                                                         //
            std::make_pair(R"({"k1":1.0})", Hash{"k1", 1.0}),                                                       //
            std::make_pair(R"({"k1":[1, 2]})", Hash{"k1", std::vector<int long long>{1, 2}}),                       //
            std::make_pair(R"({"k1":[1.0, 2.0]})", Hash{"k1", std::vector<double>{1.0, 2.0}}),                      //
            std::make_pair(R"({"k1":["1", "2"]})", Hash{"k1", std::vector<std::string>{"1", "2"}}),                 //
            std::make_pair(R"({"k1":[]})", Hash{"k1", std::vector<std::string>{}}),                                 //
            std::make_pair(R"({"k1":[true, false]})", Hash{"k1", std::vector<bool>{true, false}}),                  //
            std::make_pair(R"({"k1":{"k1_1": "val1_1"}})", Hash{"k1", Hash("k1_1", "val1_1")}),                     //
            std::make_pair(R"({"k1":{"k1_1": {"k_1_1": "val"}}})", Hash{"k1", Hash("k1_1", Hash("k_1_1", "val"))}), //
            std::make_pair(R"({"k1":[{"c1": "1.2.3.4:21", "c2": "v2"}, {"c1": "v1", "c2": "v2"}]})",
                           Hash{"k1", std::vector<Hash>{Hash("c1", "1.2.3.4:21", "c2", "v2"),
                                                        Hash("c1", "v1", "c2", "v2")}})));

INSTANTIATE_TEST_SUITE_P(UnsupportedJson, testJsonToHash_throw,
                         ::testing::Values(         //
                               R"({1})",            //
                               R"({k1: null})",     //
                               R"({k1: [null]})",   //
                               R"(1)",              //
                               R"(1.0)",            //
                               R"(true)",           //
                               R"("a")",            //
                               R"([1, "a", true])", //
                               R"({"k1": [1, "a", true]})"));

TEST_P(testJsonToHash, resultAsExpected) {
    auto& [jsonString, expectedHash] = GetParam();
    EXPECT_TRUE(isEqual(karabo::util::jsonToHash(jsonString), expectedHash));
}

TEST_P(testJsonToHash_throw, badJson) {
    auto json = GetParam();
    EXPECT_ANY_THROW(karabo::util::jsonToHash(json));
}

TEST(testInitToAutoStart, isConversionAsExpected) {
    auto initHash = Hash{"data_logger_manager_1",
                         Hash{"classId", "DataLoggerManager", //
                              "serverList", std::vector<std::string>{"karabo/dataLogger", "karabo/teststring"}},
                         "schema_printer_1", Hash{"classId", "SchemaPrinter"}};

    auto expectedAutoStartHash = Hash{
          "autoStart",
          std::vector<Hash>{Hash{"DataLoggerManager",
                                 Hash{"serverList", std::vector<std::string>{"karabo/dataLogger", "karabo/teststring"},
                                      "deviceId", "data_logger_manager_1"}},
                            Hash{"SchemaPrinter", Hash{"'deviceId'", "schema_printer_1"}}}};
    EXPECT_EQ(karabo::util::generateAutoStartHash(initHash), expectedAutoStartHash);
}
