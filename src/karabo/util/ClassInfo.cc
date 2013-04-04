/*
 * $Id: ClassInfo.cc 4956 2012-01-11 08:28:15Z wegerk@DESY.DE $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Modified by <burkhard.heisen@xfel.eu>
 *   Added getLogCategory()
 *   Extended KARABO_CLASSINFO macro to contain fully qualified class name and class version
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "ClassInfo.hh"
#include <string>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include "Exception.hh"

using namespace std;


namespace karabo {
    namespace util {


        ClassInfo::ClassInfo(const std::string& classId, const std::string& signature, const std::string& classVersion) :
        m_classId(classId), m_configVersion(classVersion) {

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
            #if defined(_WIN32)
            boost::regex re("class karabo::util::ClassInfo __cdecl\\s(.+::)*(.+)::classInfo");
            #else            
            boost::regex re("static karabo::util::ClassInfo\\s*(.+::)*(.+)::classInfo");
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
    }
}
