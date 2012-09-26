/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "HashWrap.hh"

namespace karabo {
    namespace util {

        template<>
        karabo::util::Hash::Hash(const std::string& key, const bp::object& value) {
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key, value);
        }

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2) {
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key1, value1);
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key2, value2);
        }

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2,
                const std::string& key3, const bp::object& value3) {
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key1, value1);
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key2, value2);
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key3, value3);
        }

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2,
                const std::string& key3, const bp::object& value3, const std::string& key4, const bp::object& value4) {
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key1, value1);
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key2, value2);
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key3, value3);
            karabo::pyexfel::HashWrap::pythonSetFromPath(*this, key4, value4);
        }
    }
}
