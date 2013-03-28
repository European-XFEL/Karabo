/* 
 * $Id$
 * 
 * Author: <irina.kozlova@xfel.eu>
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef PYTHONFACTORYMACROS_HH
#define	PYTHONFACTORYMACROS_HH

#include <boost/python.hpp>

namespace bp = boost::python;

#define KARABO_PYTHON_FACTORY_CONFIGURATOR_WRAPPER(baseClass) \
struct ConfiguratorWrapper : karabo::util::Configurator<baseClass>, bp::wrapper< karabo::util::Configurator<baseClass> > {\
virtual karabo::util::ClassInfo getClassInfo( ) const  {\
if( bp::override func_getClassInfo = this->get_override("getClassInfo"))\
{return func_getClassInfo( );}\
else\
{return this->karabo::util::Configurator<baseClass>::getClassInfo();}\
}\
karabo::util::ClassInfo default_getClassInfo(  ) const  {\
return karabo::util::Configurator<baseClass>::getClassInfo( );\
}\
};

#define KARABO_PYTHON_FACTORY_CONFIGURATOR(baseClass) \
.def("classInfo"\
, (karabo::util::ClassInfo(*)())(&karabo::util::Configurator<baseClass>::classInfo)).staticmethod("classInfo")\
.def("create"\
, (boost::shared_ptr<baseClass>(*)(karabo::util::Hash const &, bool const))(&karabo::util::Configurator<baseClass>::create)\
, (bp::arg("configuration"), bp::arg("validate")=(bool const)(true)))\
.def("create"\
, (boost::shared_ptr< baseClass >(*)(std::string const &, karabo::util::Hash const &, bool const))( &karabo::util::Configurator< baseClass >::create)\
, (bp::arg("classId"), bp::arg("configuration")=karabo::util::Hash(), bp::arg("validate")=(bool const)(true) )).staticmethod("create")\
.def("createChoice"\
, (boost::shared_ptr<baseClass> (*)(std::string const &, karabo::util::Hash const &,bool const ))( &karabo::util::Configurator<baseClass>::createChoice)\
, (bp::arg("choiceName"), bp::arg("input"), bp::arg("validate")=(bool const)(true))).staticmethod("createChoice")\
.def("createList"\
, (std::vector< boost::shared_ptr<baseClass> > (*)(std::string const &,karabo::util::Hash const &,bool const ))( &karabo::util::Configurator<baseClass>::createList)\
, (bp::arg("listName"), bp::arg("input"), bp::arg("validate")=(bool const)(true))).staticmethod("createList")\
.def("createNode"\
, (boost::shared_ptr<baseClass> (*)(std::string const &,std::string const &,karabo::util::Hash const &,bool const ))(&karabo::util::Configurator<baseClass>::createNode)\
, (bp::arg("nodeName"), bp::arg("classId"), bp::arg("input"), bp::arg("validate")=(bool const)(true))).staticmethod("createNode")\
.def("getClassInfo"\
, (karabo::util::ClassInfo(karabo::util::Configurator<baseClass>::* )() const)(&karabo::util::Configurator<baseClass>::getClassInfo))\
.def("getRegisteredClasses"\
, (std::vector< std::string > (*)( ))( &karabo::util::Configurator<baseClass>::getRegisteredClasses)).staticmethod("getRegisteredClasses")\
.def("getSchema"\
, (karabo::util::Schema (*)( std::string const &,karabo::util::Schema::AssemblyRules const & ))( &karabo::util::Configurator<baseClass>::getSchema)\
, (bp::arg("classId"), bp::arg("rules")=karabo::util::Schema::AssemblyRules() ) ).staticmethod( "getSchema" )
        
#endif	/* PYTHONFACTORYMACROS_HH */