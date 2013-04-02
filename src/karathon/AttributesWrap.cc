#include "AttributesWrap.hh"

namespace karabo {
    namespace util {

        template<>
        karabo::util::Hash::Attributes::OrderedMap(const std::string& key, const bp::object& value) {
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key, value);
        }

        template<>
        karabo::util::Hash::Attributes::OrderedMap(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2) {
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key1, value1);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key2, value2);
        }

        template<>
        karabo::util::Hash::Attributes::OrderedMap(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3) {
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key1, value1);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key2, value2);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key3, value3);
        }

        template<>
        karabo::util::Hash::Attributes::OrderedMap(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4) {
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key1, value1);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key2, value2);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key3, value3);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key4, value4);
        }

        template<>
        karabo::util::Hash::Attributes::OrderedMap(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4,
                                 const std::string& key5, const bp::object& value5) {
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key1, value1);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key2, value2);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key3, value3);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key4, value4);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key5, value5);
        }

        template<>
        karabo::util::Hash::Attributes::OrderedMap(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4,
                                 const std::string& key5, const bp::object& value5,
                                 const std::string& key6, const bp::object& value6) {
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key1, value1);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key2, value2);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key3, value3);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key4, value4);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key5, value5);
            karabo::pyexfel::AttributesWrap::pythonSet(*this, key6, value6);
        }
    }
}

