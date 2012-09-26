/* 
 * File:   SignalWrap.hh
 * Author: irinak
 *
 * Created on March 19, 2012, 11:37 AM
 */

#ifndef KARABO_PYKARABO_SIGNALWRAP_HH
#define	KARABO_PYKARABO_SIGNALWRAP_HH

#include <boost/python.hpp>
#include <karabo/xms/Signal.hh>
#include <karabo/net/BrokerChannel.hh>
#include "HashWrap.hh"

namespace bp = boost::python;

namespace karabo {

    namespace pyexfel {
        
        class SignalWrap : public karabo::xms::Signal {
        public:
            SignalWrap(const karabo::net::BrokerChannel::Pointer& channel, const std::string& instanceId, const std::string& signalId) : 
            karabo::xms::Signal(channel, instanceId, signalId ){}
                       
            void emitPy1(const bp::object& a1) {
               karabo::util::Hash message;
               karabo::pyexfel::HashWrap::pythonSet(message, "a1", a1);
               send(message);
            }

            void emitPy2(const bp::object& a1, const bp::object& a2) {
              karabo::util::Hash message;      
              karabo::pyexfel::HashWrap::pythonSet(message, "a1", a1);
              karabo::pyexfel::HashWrap::pythonSet(message, "a2", a2);
              send(message);
            }

            void emitPy3(const bp::object& a1, const bp::object& a2, const bp::object& a3) {
               karabo::util::Hash message;      
               karabo::pyexfel::HashWrap::pythonSet(message, "a1", a1);
               karabo::pyexfel::HashWrap::pythonSet(message, "a2", a2);
               karabo::pyexfel::HashWrap::pythonSet(message, "a3", a3);
               send(message);
            }

            void emitPy4(const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
               karabo::util::Hash message;      
               karabo::pyexfel::HashWrap::pythonSet(message, "a1", a1);
               karabo::pyexfel::HashWrap::pythonSet(message, "a2", a2);
               karabo::pyexfel::HashWrap::pythonSet(message, "a3", a3);
               karabo::pyexfel::HashWrap::pythonSet(message, "a4", a4);
               send(message);
            }
        };
    }
}

#endif	/* KARABO_PYKARABO_SIGNALWRAP_HH */

