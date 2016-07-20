

/* 
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * 
 * Created on April 5, 2013, 1:36 PM
 */

#ifndef KARABO_LOG_TRACER_HH
#define	KARABO_LOG_TRACER_HH


#include <karabo/log/Logger.hh>
#include <karabo/util/Hash.hh>

namespace karabo {

    namespace log {

        class Tracer {


            karabo::util::Hash m_conf;
            int m_numCategories;

        public:

            Tracer() : m_numCategories(0) {
                m_conf.set("priority", "DEBUG");
                m_conf.set("appenders[0].Ostream.layout", "Pattern");
            }

            void enableAll() {
                m_conf.clear();
                m_conf.set("priority", "DEBUG");
                m_conf.set("appenders[0].Ostream.layout", "Pattern");
                m_numCategories = 0;
            }

            void disableAll() {
                m_conf.clear();
                m_conf.set("priority", "INFO");
                m_conf.set("appenders[0].Ostream.layout", "Pattern");
                m_numCategories = 0;
            }

            void enable(const std::string& category) {
                {
                    std::ostringstream oss;
                    oss << "categories[" << m_numCategories << "].Category.name";
                    m_conf.set(oss.str(), category);
                }
                {
                    std::ostringstream oss;
                    oss << "categories[" << m_numCategories << "].Category.priority";
                    m_conf.set(oss.str(), "DEBUG");
                }
                m_numCategories++;
            }

            void disable(const std::string& category) {
                {
                    std::ostringstream oss;
                    oss << "categories[" << m_numCategories << "].Category.name";
                    m_conf.set(oss.str(), category);
                }
                {
                    std::ostringstream oss;
                    oss << "categories[" << m_numCategories << "].Category.priority";
                    m_conf.set(oss.str(), "INFO");
                }
                m_numCategories++;
            }

            void reconfigure() {
                karabo::log::Logger::reset();
                karabo::log::Logger::configure(m_conf);
            }


        };

    }
}

#endif	/* TRACER_HH */

