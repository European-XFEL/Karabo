/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/util/Factory.hh>
#include "PythonMacros.hh"
#include "DefaultValueVectorWrap.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

struct SchemaWrapper : Schema, bp::wrapper< Schema > {

    SchemaWrapper(Schema const & arg ) : Schema( arg ), bp::wrapper< Schema >(){
    }

    SchemaWrapper( ) : Schema( ), bp::wrapper< Schema >(){
    }

    SchemaWrapper(std::string const & classId, Schema::AssemblyRules const & rules)
    : Schema( classId, boost::ref(rules) ), bp::wrapper< karabo::util::Schema >(){
    }

    virtual ClassInfo getClassInfo() const  {
        if( bp::override func_getClassInfo = this->get_override( "getClassInfo" ) )
            return func_getClassInfo(  );
        else
            return this->Schema::getClassInfo(  );
    }
       
    ClassInfo default_getClassInfo(  ) const  {
        return Schema::getClassInfo( );
    }
};

void exportPyUtilSchema() {
     
      bp::enum_< karabo::util::AccessType>("AccessType")
        .value("INIT", karabo::util::INIT)
        .value("READ", karabo::util::READ)
        .value("WRITE", karabo::util::WRITE)
        .export_values()
        ; 
    
    {//exposing ::karabo::util::Schema

        bp::class_< Schema > s("Schema");
        s.def(bp::init< >());
        s.def(bp::init<std::string const & , bp::optional<Schema::AssemblyRules const&> > ());
        
        bp::enum_< Schema::AssignmentType > ("AssignmentType")
                .value("OPTIONAL", Schema::OPTIONAL_PARAM)
                .value("MANDATORY", Schema::MANDATORY_PARAM)
                .value("INTERNAL", Schema::INTERNAL_PARAM)
                .export_values()
                ;
        bp::enum_< Schema::ExpertLevelType > ("ExpertLevelType")
                .value("SIMPLE", Schema::SIMPLE)
                .value("MEDIUM", Schema::MEDIUM)
                .value("ADVANCED", Schema::ADVANCED)
                .export_values()
                ;
        
        bp::enum_< Schema::LeafType>("LeafType")
            .value("PROPERTY", karabo::util::Schema::PROPERTY)
            .value("COMMAND", karabo::util::Schema::COMMAND)
            .export_values()
            ;
        bp::enum_< Schema::NodeType>("NodeType")
            .value("LEAF", Schema::LEAF)
            .value("NODE", Schema::NODE)
            .value("CHOICE_OF_NODES", Schema::CHOICE_OF_NODES)
            .value("LIST_OF_NODES", Schema::LIST_OF_NODES)
            .export_values()
            ;
        bp::class_< Schema::AssemblyRules >( "AssemblyRules", bp::init< bp::optional< AccessType const &, std::string const &, std::string const & > >(( bp::arg("accessMode")=operator|(INIT, WRITE), bp::arg("state")="", bp::arg("accessRole")="" )) )    
            .def_readwrite( "m_accessMode", &Schema::AssemblyRules::m_accessMode )    
            .def_readwrite( "m_accessRole", &Schema::AssemblyRules::m_accessRole )    
            .def_readwrite( "m_state", &Schema::AssemblyRules::m_state );
        
        s.def(bp::self_ns::str(bp::self));    
         
        s.def("getAccessMode", &Schema::getAccessMode);
        
        s.def("getAssemblyRules", &Schema::getAssemblyRules);
        
        s.def("getAllowedStates"
            , &Schema::getAllowedStates
            , bp::return_value_policy< bp::copy_const_reference >() );
        
        s.def("getAssignment", &Schema::getAssignment);
        
        s.def("getDescription"
            , &Schema::getDescription
            , bp::return_value_policy< bp::copy_const_reference >() );
        
        s.def("getDisplayType"
            , &Schema::getDisplayType
            , bp::return_value_policy< bp::copy_const_reference >() );
        
        s.def("getDisplayedName"
            , &Schema::getDisplayedName
            , bp::return_value_policy< bp::copy_const_reference >() );
        //all other get-s....
        
        //********* has methods ****************

        s.def("hasAlias", &Schema::hasAlias);
        s.def("hasAccessMode", &Schema::hasAccessMode);
        //all other has .....
        
        //********* is methods ****************
        
        s.def("isAccessInitOnly", &Schema::isAccessInitOnly);
        s.def("isAccessReadOnly", &Schema::isAccessReadOnly);
        s.def("isAccessReconfigurable", &Schema::isAccessReconfigurable);
        
        s.def("isAssignmentInternal", &Schema::isAssignmentInternal);
        s.def("isAssignmentMandatory", &Schema::isAssignmentMandatory);
        s.def("isAssignmentOptional", &Schema::isAssignmentOptional);
        
        s.def("isChoiceOfNodes", &Schema::isChoiceOfNodes);
        s.def("isListOfNodes", &Schema::isListOfNodes);
        s.def("isLeaf", &Schema::isLeaf);
        s.def("isNode", &Schema::isNode);
        
        s.def("help", &Schema::help, (bp::arg("classId") = ""));
            
        s.def("getClassInfo"
           , (ClassInfo (Schema::*)() const)(&Schema::getClassInfo)
           , (ClassInfo (SchemaWrapper::*)() const)(&SchemaWrapper::default_getClassInfo));
            
        s.def("classInfo"
           , (ClassInfo (*)() )(&Schema::classInfo) ).staticmethod("classInfo");
    }// end Schema

    /////////////////////////////////////////////////////////////
    //DefaultValue<SimpleElement< EType >, EType >, where EType:
    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL

    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(int, INT32)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(long long, INT64)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(double, DOUBLE)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(std::string, STRING)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(bool, BOOL)
    
    ///////////////////////////////////////////////////////////////
    //ReadOnlySpecific<SimpleElement< EType >, EType >, where EType:
    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL

    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(int, INT32)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(long long, INT64)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(double, DOUBLE)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(std::string, STRING)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(bool, BOOL)
    

    ///////////////////////////////////////////////////////////
    //DefaultValue<VectorElement< EType, std::vector >, std::vector< EType > > where EType:
    //BOOL, INT32, UINT32, INT64, UINT64, DOUBLE, STRING 

    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(int, INT32)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(long long, INT64)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(double, DOUBLE)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(std::string, STRING)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(bool, BOOL)
            
    ///////////////////////////////////////////////////////////////
    //ReadOnlySpecific<VectorElement< EType >, EType >, where EType:
    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
     
     KARABO_PYTHON_VECTOR_READONLYSPECIFIC(int, INT32)
     KARABO_PYTHON_VECTOR_READONLYSPECIFIC(unsigned int, UINT32)
     KARABO_PYTHON_VECTOR_READONLYSPECIFIC(long long, INT64)
     KARABO_PYTHON_VECTOR_READONLYSPECIFIC(unsigned long long, UINT64)
     KARABO_PYTHON_VECTOR_READONLYSPECIFIC(double, DOUBLE)  
     KARABO_PYTHON_VECTOR_READONLYSPECIFIC(std::string, STRING)
     KARABO_PYTHON_VECTOR_READONLYSPECIFIC(bool, BOOL)
            

    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    //SimpleElement< EType >, where EType:
    //int, long long, double, string, bool
    //In Python: INT32_ELEMENT, UINT32_ELEMENT, INT64_ELEMENT, UINT64_ELEMENT, DOUBLE_ELEMENT,
    //STRING_ELEMENT, BOOL_ELEMENT

    KARABO_PYTHON_SIMPLE(int, INT32)
    KARABO_PYTHON_SIMPLE(unsigned int, UINT32)
    KARABO_PYTHON_SIMPLE(long long, INT64)
    KARABO_PYTHON_SIMPLE(unsigned long long, UINT64)
    KARABO_PYTHON_SIMPLE(double, DOUBLE)
    KARABO_PYTHON_SIMPLE(string, STRING)
    KARABO_PYTHON_SIMPLE(bool, BOOL)
  
    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    // Binding ::karabo::util::VectorElement< EType, std::vector >
    // In Python : VECTOR_INT32_ELEMENT, VECTOR_UINT32_ELEMENT, 
    // VECTOR_INT64_ELEMENT, VECTOR_UINT64_ELEMENT, VECTOR_DOUBLE_ELEMENT,
    // VECTOR_STRING_ELEMENT

    KARABO_PYTHON_VECTOR(int, INT32)
    KARABO_PYTHON_VECTOR(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR(long long, INT64)
    KARABO_PYTHON_VECTOR(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR(double, DOUBLE)
    KARABO_PYTHON_VECTOR(string, STRING)   
    KARABO_PYTHON_VECTOR(bool, BOOL)
 
}//end  exportPyUtilSchema

