/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Modified by <burkhard.heisen@xfel.eu>
 *   Added getLogCategory()
 *   Extended KARABO_CLASSINFO macro to contain fully qualified class name and class version
 *
 * Modified by <krzysztof.wrona@xfel.eu>
 *   improved parsing to work with template specializations (proper namespace)
 *   Note: Still template parameters are not part of the class name
 *         Also for template specializations
 *   Consider further improvements for template parameters - to be discussed what is needed
 *   i.e.
 *    template< class T> class A
 *    template<> class A<int>
 *    template<> class A<std::string>
 *    template<class U, Class V> class B
 *    template<class U > class B<U, int>
 *    template<> class B<float, std::string>
 *    etc.
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
 */


#include "ClassInfo.hh"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <string>

#include "Exception.hh"

using namespace std;


namespace karabo {
    namespace data {


        ClassInfo::ClassInfo(const std::string& classId, const std::string& signature, const std::string& classVersion)
            : m_classId(classId), m_configVersion(classVersion) {
            initClassNameAndSpace(signature);
            initLogCategory();
        }


        const std::string& ClassInfo::getClassName() const {
            return m_className;
        }


        const std::string& ClassInfo::getNamespace() const {
            return m_namespace;
        }


        const std::string& ClassInfo::getClassId() const {
            return m_classId;
        }


        const std::string& ClassInfo::getLogCategory() const {
            return m_logCategory;
        }


        const std::string& ClassInfo::getVersion() const {
            return m_configVersion;
        }


        void ClassInfo::initClassNameAndSpace(const std::string& signature) {
            //            clog << "signature=|" << signature << "|" << endl;

            size_t found = signature.find("<");
            if (found == std::string::npos) {
                // no templates in the signature
#if defined(_WIN32)
                boost::regex re("class karabo::data::ClassInfo __cdecl\\s(.+::)*(.+)::classInfo");
#else
                boost::regex re("static karabo::data::ClassInfo\\s*(.+::)*(.+)::classInfo");
#endif
                boost::smatch what;
                bool result = boost::regex_search(signature, what, re);
                if (result && what.size() == 3) {
                    m_className = what.str(2);
                    m_namespace = what.str(1);
                    if (m_namespace.length() > 1) {
                        std::string::iterator it = m_namespace.end() - 1;
                        if (*it == ':') {
                            m_namespace.erase(it--);
                            m_namespace.erase(it);
                        }
                    }
                } else {
                    throw KARABO_LOGIC_EXCEPTION("Introspection error");
                }
            } else {
                string tmp = signature.substr(0, found);

#if defined(_WIN32)
                boost::regex re("class karabo::data::ClassInfo __cdecl\\s(.+::)*(.+)");
#else
                boost::regex re("static karabo::data::ClassInfo\\s*(.+::)*(.+)");
#endif

                boost::smatch what;

                bool result = boost::regex_search(tmp, what, re);
                if (result && what.size() == 3) {
                    m_className = what.str(2);
                    m_namespace = what.str(1);
                    if (m_namespace.length() > 1) {
                        std::string::iterator it = m_namespace.end() - 1;
                        if (*it == ':') {
                            m_namespace.erase(it--);
                            m_namespace.erase(it);
                        }
                    }
                } else {
                    throw KARABO_LOGIC_EXCEPTION("Introspection error");
                }
            }
        }


        void ClassInfo::initLogCategory() {
            std::vector<std::string> tokens;
            boost::split(tokens, m_namespace, boost::is_any_of("::"));
            for (size_t i = 0; i < tokens.size(); ++i) {
                if (!tokens[i].empty()) {
                    m_logCategory += tokens[i] + ".";
                }
            }
            m_logCategory += m_classId;
        }
    } // namespace data
} // namespace karabo
