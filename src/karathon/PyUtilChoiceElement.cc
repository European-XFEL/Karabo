/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <karabo/net/Connection.hh>
#include <karabo/net/BrokerConnection.hh>

#include "PythonMacros.hh"
#include <karabo/util/ChoiceElement.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace karabo::net;
using namespace std;

struct ChoiceElementWrap : CHOICE_ELEMENT< Schema >, bp::wrapper< CHOICE_ELEMENT< Schema > > {

    ChoiceElementWrap(Schema & expected, Schema const & pythonExpected )
    : CHOICE_ELEMENT<Schema>( boost::ref(expected), boost::ref(pythonExpected) )
      , bp::wrapper< CHOICE_ELEMENT< Schema > >(){
    }

    virtual void build(  ) {
        if( bp::override func_build = this->get_override( "build" ) )
            func_build(  );
        else
            this->CHOICE_ELEMENT< Schema >::build(  );
    }
  
    void default_build(  ) {
        CHOICE_ELEMENT< Schema >::build( );
    }

};

void exportChoiceElement() {

    bp::class_< ChoiceElementWrap, boost::noncopyable >( "CHOICE_ELEMENT", bp::init< Schema &, Schema const & >(( bp::arg("expected"), bp::arg("pythonExpected") )) )    
        .def( 
            "build"
            , (void ( CHOICE_ELEMENT< Schema>::* )(  ) )(&CHOICE_ELEMENT< Schema >::build)
            , (void ( ChoiceElementWrap::* )(  ) )(&ChoiceElementWrap::default_build) )
            ;


  //C++: CHOICE_ELEMENT<Connection>    Python: CHOICE_ELEMENT_CONNECTION
  {
    //::karabo::util::GenericElement< karabo::util::CHOICE_ELEMENT< karabo::net::Connection >, std::string >
    typedef CHOICE_ELEMENT< Connection > U;
    typedef std::string EType;
    KARABO_PYTHON_GENERIC_SIMPLE_TYPES( U, EType );
    typedef DefaultValue< U, EType > DefValue;
    
    bp::class_< DefValue, boost::noncopyable > ("DefaultValueCHOICE_ELEMENTConnection", bp::no_init)
            KARABO_PYTHON_DEFAULT_VALUE
    ;

    bp::class_< GenericElement< U, string >, boost::noncopyable > ("GenericElementCHOICE_ELEMENTConnection", bp::init< Schema & >((bp::arg("expected"))))
            KARABO_PYTHON_GENERIC_ELEMENT_DEFS
    ;

    //::karabo::util::CHOICE_ELEMENT< karabo::net::Connection >   "CHOICE_ELEMENT_CONNECTION"
    typedef bp::class_< U, bp::bases< GenericElement< U, string > >, boost::noncopyable > CHOICE_ELEMENT_CONNECTION_exposer_t;
    bp::implicitly_convertible< Schema &, U > ();
    CHOICE_ELEMENT_CONNECTION_exposer_t("CHOICE_ELEMENT_CONNECTION", bp::init< Schema & >((bp::arg("expected"))));
  }
            
  //C++: CHOICE_ELEMENT<karabo::net::BrokerConnection>    Python: CHOICE_ELEMENT_BROKERCONNECTION
  {
    //karabo::util::GenericElement< karabo::util::CHOICE_ELEMENT< karabo::net::BrokerConnection >, std::string >
    typedef CHOICE_ELEMENT< BrokerConnection > U;
    typedef std::string EType;
    KARABO_PYTHON_GENERIC_SIMPLE_TYPES( U, EType );
    typedef DefaultValue< U, EType > DefValue;
    
    bp::class_< DefValue, boost::noncopyable > ("DefaultValueCHOICE_ELEMENTBrokerConnection", bp::no_init)
            KARABO_PYTHON_DEFAULT_VALUE
    ;

    bp::class_< GenericElement< U, string >, boost::noncopyable > ("GenericElementCHOICE_ELEMENTBrokerConnection", bp::init< Schema & >((bp::arg("expected"))))
            KARABO_PYTHON_GENERIC_ELEMENT_DEFS
    ;

    bp::class_< U, bp::bases< GenericElement< U, string > >, boost::noncopyable > ("CHOICE_ELEMENT_BROKERCONNECTION", bp::init< Schema & >(bp::arg("expected")));
    bp::implicitly_convertible< Schema &, U > ();
  }          
            
}//exportChoiceElement()
