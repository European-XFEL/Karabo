

/* 
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * 
 * Created on April 5, 2013, 1:36 PM
 */

#ifndef KARABO_IO_H5_IOTRACER_HH
#define	KARABO_IO_H5_IOTRACER_HH

//#define KARABO_ENABLE_TRACE_LOG

#include <karabo/log/Logger.hh>
#include <karabo/util/Hash.hh>

namespace karabo {

    namespace io {

        namespace h5 {

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

                void enable(const char* category) {                
                    std::string cat(category);
                    enable(cat);
                }
                
                void enable(std::string& category) {
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

                void disable(const char* category) {                
                    std::string cat(category);
                    disable(cat);
                }
                
                void disable(std::string& category) {
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
                    karabo::log::Logger::configure(m_conf);
                }


            };

        }
    }
}
#endif	/* IOTRACER_HH */

