/* 
 * File:   SignalWrap.hh
 * Author: irinak
 *
 * Created on March 19, 2012, 11:37 AM
 */

#ifndef EXFEL_PYEXFEL_SIGNALWRAP_HH
#define	EXFEL_PYEXFEL_SIGNALWRAP_HH

#include <boost/python.hpp>
#include <exfel/xms/Signal.hh>
#include <exfel/net/BrokerChannel.hh>
#include "HashWrap.hh"

namespace bp = boost::python;

namespace exfel {

    namespace pyexfel {
        
        class SignalWrap : public exfel::xms::Signal {
        public:
            SignalWrap(const exfel::net::BrokerChannel::Pointer& channel, const std::string& instanceId, const std::string& signalId) : 
            exfel::xms::Signal(channel, instanceId, signalId ){}
                       
            void emitPy1(const bp::object& a1) {
               exfel::util::Hash message;
               exfel::pyexfel::HashWrap::pythonSet(message, "a1", a1);
               send(message);
            }

            void emitPy2(const bp::object& a1, const bp::object& a2) {
              exfel::util::Hash message;      
              exfel::pyexfel::HashWrap::pythonSet(message, "a1", a1);
              exfel::pyexfel::HashWrap::pythonSet(message, "a2", a2);
              send(message);
            }

            void emitPy3(const bp::object& a1, const bp::object& a2, const bp::object& a3) {
               exfel::util::Hash message;      
               exfel::pyexfel::HashWrap::pythonSet(message, "a1", a1);
               exfel::pyexfel::HashWrap::pythonSet(message, "a2", a2);
               exfel::pyexfel::HashWrap::pythonSet(message, "a3", a3);
               send(message);
            }

            void emitPy4(const bp::object& a1, const bp::object& a2, const bp::object& a3, const bp::object& a4) {
               exfel::util::Hash message;      
               exfel::pyexfel::HashWrap::pythonSet(message, "a1", a1);
               exfel::pyexfel::HashWrap::pythonSet(message, "a2", a2);
               exfel::pyexfel::HashWrap::pythonSet(message, "a3", a3);
               exfel::pyexfel::HashWrap::pythonSet(message, "a4", a4);
               send(message);
            }
        };
    }
}

#endif	/* EXFEL_PYEXFEL_SIGNALWRAP_HH */

