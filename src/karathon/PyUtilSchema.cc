/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/util/Factory.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/ListElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/Validator.hh>

#include "PythonMacros.hh"
#include "DefaultValueVectorWrap.hh"
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

struct SchemaWrapper : Schema, bp::wrapper< Schema > {


    SchemaWrapper(Schema const & arg) : Schema(arg), bp::wrapper< Schema >() {
    }


    SchemaWrapper() : Schema(), bp::wrapper< Schema >() {
    }


    SchemaWrapper(std::string const & classId, Schema::AssemblyRules const & rules)
    : Schema(classId, boost::ref(rules)), bp::wrapper< karabo::util::Schema >() {
    }


    virtual ClassInfo getClassInfo() const {
        if (bp::override func_getClassInfo = this->get_override("getClassInfo"))
            return func_getClassInfo();
        else
            return this->Schema::getClassInfo();
    }


    ClassInfo default_getClassInfo() const {
        return Schema::getClassInfo();
    }
};

class ValidatorWrap : Validator {
public:
    ValidatorWrap() : Validator() {} 
    bp::object validate(const bp::object& schemaObj, const bp::object& confObj) {
        Hash validated;
        if (bp::extract<Schema>(schemaObj).check() && bp::extract<Hash>(confObj).check()) {
            const Schema& schema = bp::extract<Schema>(schemaObj);
            const Hash& configuration = bp::extract<Hash>(confObj);
            pair<bool, string> result = Validator::validate(schema, configuration, validated);
            if (result.first) 
                return bp::object(validated);
            throw KARABO_PYTHON_EXCEPTION(result.second);
        }
        throw KARABO_PYTHON_EXCEPTION("Python arguments are not supported types");
    }
};

struct NodeElementWrap {
    
    static karabo::util::NodeElement & appendParametersOfPy(karabo::util::NodeElement& self, const bp::object& obj) {

        if (!PyType_Check(obj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'arg' given in 'appendParametersOf(arg)' of NODE_ELEMENT must be a class in Python");
        }
        if (!obj.attr("expectedParameters")) {
            throw KARABO_PYTHON_EXCEPTION("Class given in 'appendParametersOf' of NODE_ELELEMT must have 'expectedParameters' function");
        }

        const karabo::util::Schema schemaPy = bp::extract<karabo::util::Schema> (obj.attr("getSchema")());

        const karabo::util::Hash h = schemaPy.getParameterHash();
        self.getNode().setValue<karabo::util::Hash>(h);

        return self;
    }

};

  
karabo::pyexfel::PyTypes::ReferenceType Wrap_Schema_getValueType(const Schema& schema, const bp::object& obj) {
   if (PyString_Check(obj.ptr())){
      string path = bp::extract<string>(obj);  
      Types::ReferenceType t = schema.getValueType(path);   
      return karabo::pyexfel::PyTypes::from(t);
   } 
   throw KARABO_PYTHON_EXCEPTION("Python argument in 'getValueType' must be a string"); 
}

//*********************************************************************
// Wrapper functions for : getMinInc, getMaxInc, getMinExc, getMaxExc *
//*********************************************************************
bp::object Wrap_Schema_getMinInc(const Schema& schema, const bp::object& obj){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        Types::ReferenceType reftype = schema.getValueType(path);

        switch (reftype) {
                case Types::INT32:
                     return bp::object(schema.getMinInc<int>(path));
                case Types::UINT32:
                     return bp::object(schema.getMinInc<unsigned int>(path));
                case Types::INT64:
                     return bp::object(schema.getMinInc<long long>(path));
                case Types::UINT64:
                     return bp::object(schema.getMinInc<unsigned long long>(path));
                case Types::STRING:
                     return bp::object(schema.getMinInc<string>(path));
                case Types::DOUBLE:
                     return bp::object(schema.getMinInc<double>(path));    
                default:
                     break;
        }
        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getMinInc' must be a string");
}


bp::object Wrap_Schema_getMaxInc(const Schema& schema, const bp::object& obj){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        Types::ReferenceType reftype = schema.getValueType(path);

        switch (reftype) {
                case Types::INT32:
                     return bp::object(schema.getMaxInc<int>(path));
                case Types::UINT32:
                     return bp::object(schema.getMaxInc<unsigned int>(path));
                case Types::INT64:
                     return bp::object(schema.getMaxInc<long long>(path));
                case Types::UINT64:
                     return bp::object(schema.getMaxInc<unsigned long long>(path));
                case Types::STRING:
                     return bp::object(schema.getMaxInc<string>(path));
                case Types::DOUBLE:
                     return bp::object(schema.getMaxInc<double>(path));    
                default:
                     break;
        }
        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getMaxInc' must be a string");
}


bp::object Wrap_Schema_getMinExc(const Schema& schema, const bp::object& obj){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        Types::ReferenceType reftype = schema.getValueType(path);

        switch (reftype) {
                case Types::INT32:
                     return bp::object(schema.getMinExc<int>(path));
                case Types::UINT32:
                     return bp::object(schema.getMinExc<unsigned int>(path));
                case Types::INT64:
                     return bp::object(schema.getMinExc<long long>(path));
                case Types::UINT64:
                     return bp::object(schema.getMinExc<unsigned long long>(path));
                case Types::STRING:
                     return bp::object(schema.getMinExc<string>(path));
                case Types::DOUBLE:
                     return bp::object(schema.getMinExc<double>(path));    
                default:
                     break;
        }
        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getMinExc' must be a string");
}


bp::object Wrap_Schema_getMaxExc(const Schema& schema, const bp::object& obj){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        Types::ReferenceType reftype = schema.getValueType(path);

        switch (reftype) {
                case Types::INT32:
                     return bp::object(schema.getMaxExc<int>(path));
                case Types::UINT32:
                     return bp::object(schema.getMaxExc<unsigned int>(path));
                case Types::INT64:
                     return bp::object(schema.getMaxExc<long long>(path));
                case Types::UINT64:
                     return bp::object(schema.getMaxExc<unsigned long long>(path));
                case Types::STRING:
                     return bp::object(schema.getMaxExc<string>(path));
                case Types::DOUBLE:
                     return bp::object(schema.getMaxExc<double>(path));    
                default:
                     break;
        }
        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getMinInc' must be a string");
}

//*****************************************************************************
// Wrapper functions for : getMinIncAs, getMaxIncAs, getMinExcAs, getMaxExcAs *
//*****************************************************************************

bp::object Wrap_Schema_getMinIncAs(const Schema& schema, const bp::object& obj, const karabo::pyexfel::PyTypes::ReferenceType& pytype){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        switch (pytype) {
                case karabo::pyexfel::PyTypes::INT32:
                     return bp::object(schema.getMinIncAs<int>(path));
                case karabo::pyexfel::PyTypes::UINT32:
                     return bp::object(schema.getMinIncAs<unsigned int>(path));
                case karabo::pyexfel::PyTypes::INT64:
                     return bp::object(schema.getMinIncAs<long long>(path));
                case karabo::pyexfel::PyTypes::UINT64:
                     return bp::object(schema.getMinIncAs<unsigned long long>(path));
                case karabo::pyexfel::PyTypes::STRING:
                     return bp::object(schema.getMinIncAs<string>(path));
                case karabo::pyexfel::PyTypes::DOUBLE:
                     return bp::object(schema.getMinIncAs<double>(path));    
                default:
                     break;
        }
        throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getMinIncAs' must be a string");
}


bp::object Wrap_Schema_getMaxIncAs(const Schema& schema, const bp::object& obj, const karabo::pyexfel::PyTypes::ReferenceType& pytype){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        switch (pytype) {
                case karabo::pyexfel::PyTypes::INT32:
                     return bp::object(schema.getMaxIncAs<int>(path));
                case karabo::pyexfel::PyTypes::UINT32:
                     return bp::object(schema.getMaxIncAs<unsigned int>(path));
                case karabo::pyexfel::PyTypes::INT64:
                     return bp::object(schema.getMaxIncAs<long long>(path));
                case karabo::pyexfel::PyTypes::UINT64:
                     return bp::object(schema.getMaxIncAs<unsigned long long>(path));
                case karabo::pyexfel::PyTypes::STRING:
                     return bp::object(schema.getMaxIncAs<string>(path));
                case karabo::pyexfel::PyTypes::DOUBLE:
                     return bp::object(schema.getMaxIncAs<double>(path));    
                default:
                     break;
        }
        throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getMaxIncAs' must be a string");
}

bp::object Wrap_Schema_getMinExcAs(const Schema& schema, const bp::object& obj, const karabo::pyexfel::PyTypes::ReferenceType& pytype){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        switch (pytype) {
                case karabo::pyexfel::PyTypes::INT32:
                     return bp::object(schema.getMinExcAs<int>(path));
                case karabo::pyexfel::PyTypes::UINT32:
                     return bp::object(schema.getMinExcAs<unsigned int>(path));
                case karabo::pyexfel::PyTypes::INT64:
                     return bp::object(schema.getMinExcAs<long long>(path));
                case karabo::pyexfel::PyTypes::UINT64:
                     return bp::object(schema.getMinExcAs<unsigned long long>(path));
                case karabo::pyexfel::PyTypes::STRING:
                     return bp::object(schema.getMinExcAs<string>(path));
                case karabo::pyexfel::PyTypes::DOUBLE:
                     return bp::object(schema.getMinExcAs<double>(path));    
                default:
                     break;
        }
        throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getMinExcAs' must be a string");
}

bp::object Wrap_Schema_getMaxExcAs(const Schema& schema, const bp::object& obj, const karabo::pyexfel::PyTypes::ReferenceType& pytype){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        switch (pytype) {
                case karabo::pyexfel::PyTypes::INT32:
                     return bp::object(schema.getMaxExcAs<int>(path));
                case karabo::pyexfel::PyTypes::UINT32:
                     return bp::object(schema.getMaxExcAs<unsigned int>(path));
                case karabo::pyexfel::PyTypes::INT64:
                     return bp::object(schema.getMaxExcAs<long long>(path));
                case karabo::pyexfel::PyTypes::UINT64:
                     return bp::object(schema.getMaxExcAs<unsigned long long>(path));
                case karabo::pyexfel::PyTypes::STRING:
                     return bp::object(schema.getMaxExcAs<string>(path));
                case karabo::pyexfel::PyTypes::DOUBLE:
                     return bp::object(schema.getMaxExcAs<double>(path));    
                default:
                     break;
        }
        throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getMaxExcAs' must be a string");
}


bp::list Wrap_Schema_getParameters(const Schema& schema, const bp::object& obj) {
    if (PyString_Check(obj.ptr())) {
        bp::list listParams;
        string path = bp::extract<string>(obj);
        std::vector<std::string> v = schema.getParameters(path);
        for (size_t i = 0; i < v.size(); i++) listParams.attr("append")(bp::object(v[i]));
        return listParams;
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getParameters' should be a string");
}
     
bp::object Wrap_Schema_getTags(const Schema& schema, const bp::object& obj) {
    if (PyString_Check(obj.ptr())) {
        string path = bp::extract<string>(obj);
        std::vector<std::string> v = schema.getTags(path);
        return karabo::pyexfel::Wrapper::fromStdVectorToPyArray<string>(v);
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getTags' should be a string");
}


bp::object Wrap_Schema_getOptions(const Schema& schema, const bp::object& obj) {
    if (PyString_Check(obj.ptr())) {
        string path = bp::extract<string>(obj);
        vector<string> v = schema.getOptions(path);
        return karabo::pyexfel::Wrapper::fromStdVectorToPyArray<string>(v);
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getOptions' should be a string");
}

bp::object Wrap_Schema_getAllowedStates(const Schema& schema, const bp::object& obj) {
    if (PyString_Check(obj.ptr())) {
        string path = bp::extract<string>(obj);
        vector<string> v = schema.getAllowedStates(path);
        return karabo::pyexfel::Wrapper::fromStdVectorToPyArray<string>(v);
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAllowedStates' should be a string");
}

bp::object Wrap_Schema_getDefaultValue(const Schema& schema, const bp::object& obj){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        Types::ReferenceType reftype = schema.getValueType(path);

        switch (reftype) {
                case Types::BOOL:
                     return bp::object(schema.getDefaultValue<bool>(path));    
                case Types::INT32:
                     return bp::object(schema.getDefaultValue<int>(path));
                case Types::UINT32:
                     return bp::object(schema.getDefaultValue<unsigned int>(path));
                case Types::INT64:
                     return bp::object(schema.getDefaultValue<long long>(path));
                case Types::UINT64:
                     return bp::object(schema.getDefaultValue<unsigned long long>(path));
                case Types::STRING:
                     return bp::object(schema.getDefaultValue<string>(path));
                case Types::DOUBLE:
                     return bp::object(schema.getDefaultValue<double>(path));    
                default:
                     break;
        }
        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python argument in 'getDefaultValue' should be a string");
}


bp::object Wrap_Schema_getDefaultValueAs(const Schema& schema, const bp::object& obj, const karabo::pyexfel::PyTypes::ReferenceType& pytype){
    if (PyString_Check(obj.ptr())){
        string path = bp::extract<string>(obj);
        
        switch (pytype) {
                case karabo::pyexfel::PyTypes::BOOL:
                     return bp::object(schema.getDefaultValueAs<bool>(path));    
                case karabo::pyexfel::PyTypes::INT32:
                     return bp::object(schema.getDefaultValueAs<int>(path));
                case karabo::pyexfel::PyTypes::UINT32:
                     return bp::object(schema.getDefaultValueAs<unsigned int>(path));
                case karabo::pyexfel::PyTypes::INT64:
                     return bp::object(schema.getDefaultValueAs<long long>(path));
                case karabo::pyexfel::PyTypes::UINT64:
                     return bp::object(schema.getDefaultValueAs<unsigned long long>(path));
                case karabo::pyexfel::PyTypes::STRING:
                     return bp::object(schema.getDefaultValueAs<string>(path));
                case karabo::pyexfel::PyTypes::DOUBLE:
                     return bp::object(schema.getDefaultValueAs<double>(path));    
                default:
                     break;
        }
        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not supported");
    }
    throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getDefaultValueAs' should be a string");
}


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
        s.def(bp::init<std::string const &, bp::optional<Schema::AssemblyRules const&> > ());

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
        bp::class_< Schema::AssemblyRules >("AssemblyRules", bp::init< bp::optional< AccessType const &, std::string const &, std::string const & > >((bp::arg("accessMode") = operator|(INIT, WRITE), bp::arg("state") = "", bp::arg("accessRole") = "")))
                .def_readwrite("m_accessMode", &Schema::AssemblyRules::m_accessMode)
                .def_readwrite("m_accessRole", &Schema::AssemblyRules::m_accessRole)
                .def_readwrite("m_state", &Schema::AssemblyRules::m_state);

        s.def(bp::self_ns::str(bp::self));

        s.def("getAccessMode", &Schema::getAccessMode);

        s.def("getAssemblyRules", &Schema::getAssemblyRules);

        s.def("getAssignment", &Schema::getAssignment);

        s.def("getDescription"
              , &Schema::getDescription
              , bp::return_value_policy< bp::copy_const_reference >());

        s.def("getDisplayType"
              , &Schema::getDisplayType
              , bp::return_value_policy< bp::copy_const_reference >());

        s.def("getDisplayedName"
              , &Schema::getDisplayedName
              , bp::return_value_policy< bp::copy_const_reference >());
        
        s.def("getUnit", &Schema::getUnit);
        
        s.def("getUnitName"
              , &Schema::getUnitName
              , bp::return_value_policy< bp::copy_const_reference >());
        
        s.def("getUnitSymbol"
              , &Schema::getUnitSymbol
              , bp::return_value_policy< bp::copy_const_reference >());
        
        s.def("getMetricPrefix", &Schema::getMetricPrefix);
        
        s.def("getMetricPrefixName"
               , &Schema::getMetricPrefixName
               , bp::return_value_policy< bp::copy_const_reference >());
        
        s.def("getMetricPrefixSymbol"
               , &Schema::getMetricPrefixSymbol
               , bp::return_value_policy< bp::copy_const_reference >());
        
        s.def("getRootName"
               , &Schema::getRootName
               , bp::return_value_policy< bp::copy_const_reference >());
        
        s.def("getValueType", &Wrap_Schema_getValueType, (bp::arg("path")));
        
        s.def("getNodeType", &Schema::getNodeType);
        
        s.def("getMinInc", &Wrap_Schema_getMinInc, (bp::arg("path")));

        s.def("getMaxInc", &Wrap_Schema_getMaxInc);
        
        s.def("getMinExc", &Wrap_Schema_getMinExc);
        
        s.def("getMaxExc", &Wrap_Schema_getMaxExc);

        s.def("getMinIncAs", &Wrap_Schema_getMinIncAs, (bp::arg("path"),bp::arg("pytype")));
        
        s.def("getMaxIncAs", &Wrap_Schema_getMaxIncAs, (bp::arg("path"),bp::arg("pytype")));
        
        s.def("getMinExcAs", &Wrap_Schema_getMinExcAs, (bp::arg("path"),bp::arg("pytype")));
        
        s.def("getMaxExcAs", &Wrap_Schema_getMaxExcAs, (bp::arg("path"),bp::arg("pytype")));
        
        s.def("getAliasAsString", &Schema::getAliasAsString); 
        
        s.def("getParameters", &Wrap_Schema_getParameters, (bp::arg("path") = "") );
        
        s.def("getOptions", &Wrap_Schema_getOptions);
        
        s.def("getTags", &Wrap_Schema_getTags);
        
        s.def("getAllowedStates", &Wrap_Schema_getAllowedStates);
        
        s.def("getDefaultValue", &Wrap_Schema_getDefaultValue);
        
        s.def("getDefaultValueAs", &Wrap_Schema_getDefaultValueAs);
        //all other get-s....

        //********* has methods ****************

        s.def("keyHasAlias", &Schema::keyHasAlias);
        
        s.def("hasAccessMode", &Schema::hasAccessMode);
        
        s.def("hasAssignment", &Schema::hasAssignment);
        
        s.def("hasAllowedStates", &Schema::hasAllowedStates);
        
        s.def("hasDefaultValue", &Schema::hasDefaultValue);
        
        s.def("hasOptions", &Schema::hasOptions);
        
        s.def("hasTags", &Schema::hasTags);
        
        s.def("hasUnit", &Schema::hasUnit);
        
        s.def("hasMetricPrefix", &Schema::hasMetricPrefix);
        
        s.def("hasMinInc", &Schema::hasMinInc);
        
        s.def("hasMaxInc", &Schema::hasMaxInc);
        
        s.def("hasMinExc", &Schema::hasMinExc);
        
        s.def("hasMaxExc", &Schema::hasMaxExc);
        
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
              , (ClassInfo(Schema::*)() const) (&Schema::getClassInfo)
              , (ClassInfo(SchemaWrapper::*)() const) (&SchemaWrapper::default_getClassInfo));

        s.def("classInfo"
              , (ClassInfo(*)())(&Schema::classInfo)).staticmethod("classInfo");
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


    //////////////////////////////////////////////////////////////////////
    //Binding karabo::util::SimpleElement< EType >, where EType:
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

    //////////////////////////////////////////////////////////////////////
    // Binding karabo::util::VectorElement< EType, std::vector >
    // In Python : VECTOR_INT32_ELEMENT, VECTOR_UINT32_ELEMENT, 
    // VECTOR_INT64_ELEMENT, VECTOR_UINT64_ELEMENT, VECTOR_DOUBLE_ELEMENT,
    // VECTOR_STRING_ELEMENT, VECTOR_BOOL_ELEMENT

    KARABO_PYTHON_VECTOR(int, INT32)
    KARABO_PYTHON_VECTOR(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR(long long, INT64)
    KARABO_PYTHON_VECTOR(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR(double, DOUBLE)
    KARABO_PYTHON_VECTOR(string, STRING)
    KARABO_PYTHON_VECTOR(bool, BOOL)

    //////////////////////////////////////////////////////////////////////
    // Binding karabo::util::NodeElement       
    // In Python : NODE_ELEMENT
    {
        bp::implicitly_convertible< Schema &, NodeElement >();
        bp::class_<NodeElement> ("NODE_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                KARABO_PYTHON_NODE_CHOICE_LIST(NodeElement)
                .def("appendParametersOf"
                     , &NodeElementWrap::appendParametersOfPy
                     , bp::return_internal_reference<> ())
                ;
    }

    //////////////////////////////////////////////////////////////////////
    // Binding karabo::util::ListElement
    // In Python : LIST_ELEMENT
    {
        bp::implicitly_convertible< Schema &, ListElement >();
        bp::class_<ListElement> ("LIST_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                KARABO_PYTHON_NODE_CHOICE_LIST(ListElement)
                .def("assignmentMandatory"
                     , &ListElement::assignmentMandatory
                     , bp::return_internal_reference<> ())
                .def("assignmentOptional"
                     , &ListElement::assignmentOptional
                     , bp::return_internal_reference<> ())
                .def("min"
                     , &ListElement::min
                     , bp::return_internal_reference<> ())
                .def("max"
                     , &ListElement::max
                     , bp::return_internal_reference<> ())
                ;
    }

    //////////////////////////////////////////////////////////////////////
    // Binding karabo::util::ChoiceElement       
    // In Python : CHOICE_ELEMENT
    {
        bp::implicitly_convertible< Schema &, ChoiceElement >();
        bp::class_<ChoiceElement> ("CHOICE_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                KARABO_PYTHON_NODE_CHOICE_LIST(ChoiceElement)
                .def("assignmentMandatory"
                     , &ChoiceElement::assignmentMandatory
                     , bp::return_internal_reference<> ())
                .def("assignmentOptional"
                     , &ChoiceElement::assignmentOptional
                     , bp::return_internal_reference<> ())
                ;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  karabo::util::DefaultValue<ChoiceElement> 
    {
        typedef DefaultValue<ChoiceElement, string> DefChoiceElement;
        bp::class_< DefChoiceElement, boost::noncopyable > ("DefaultValueChoiceElement", bp::no_init)
                .def("defaultValue"
                     , (ChoiceElement & (DefChoiceElement::*)(string const &))(&DefChoiceElement::defaultValue)
                     , (bp::arg("defValue"))
                     , bp::return_internal_reference<> ())
                .def("noDefaultValue"
                     , (ChoiceElement & (DefChoiceElement::*)())(&DefChoiceElement::noDefaultValue)
                     , bp::return_internal_reference<> ())
                ;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  karabo::util::DefaultValue<ListElement> 
    {
        typedef DefaultValue<ListElement, vector<string> > DefListElement;
        bp::class_< DefListElement, boost::noncopyable > ("DefaultValueListElement", bp::no_init)
                .def("defaultValue"
                     , (ListElement & (DefListElement::*)(string const &))(&DefListElement::defaultValueFromString)
                     , (bp::arg("defValue"))
                     , bp::return_internal_reference<> ())
                .def("noDefaultValue"
                     , (ListElement & (DefListElement::*)())(&DefListElement::noDefaultValue)
                     , bp::return_internal_reference<> ())
                ;
    }

    {
        bp::class_<ValidatorWrap>("Validator", bp::init<>())
                .def("validate", &ValidatorWrap::validate, (bp::arg("schema"), bp::arg("configuration")))
                ;
    }
    
    {        
           bp::class_<Units> ("Units");
           bp::enum_<Units::MetricPrefix>("MetricPrefix")
            .value("YOTTA", Units::YOTTA)
            .value("ZETTA", Units::ZETTA)
            .value("EXA", Units::EXA)
            .value("PETA", Units::PETA)
            .value("TERA", Units::TERA)
            .value("GIGA", Units::GIGA)
            .value("MEGA", Units::MEGA)
            .value("KILO", Units::KILO)
            .value("HECTO", Units::HECTO)
            .value("DECA", Units::DECA)
            .value("NONE", Units::NONE)
            .value("DECI", Units::DECI)
            .value("CENTI", Units::CENTI)
            .value("MILLI", Units::MILLI)
            .value("MICRO", Units::MICRO)
            .value("NANO", Units::NANO)
            .value("PICO", Units::PICO)
            .value("FEMTO", Units::FEMTO)
            .value("ATTO", Units::ATTO)
            .value("ZEPTO", Units::ZEPTO)
            .value("YOCTO", Units::YOCTO)
            .export_values()
            ;
        bp::enum_< karabo::util::Units::Unit>("Unit")
            .value("METER", Units::METER)
            .value("GRAM", Units::GRAM)
            .value("SECOND", Units::SECOND)
            .value("AMPERE", Units::AMPERE)
            .value("KELVIN", Units::KELVIN)
            .value("MOLE", Units::MOLE)
            .value("CANDELA", Units::CANDELA)
            .value("HERTZ", Units::HERTZ)
            .value("RADIAN", Units::RADIAN)
            .value("STERADIAN", Units::STERADIAN)
            .value("NEWTON", Units::NEWTON)
            .value("PASCAL", Units::PASCAL)
            .value("JOULE", Units::JOULE)
            .value("WATT", Units::WATT)
            .value("COULOMB", Units::COULOMB)
            .value("VOLT", Units::VOLT)
            .value("FARAD", Units::FARAD)
            .value("OHM", Units::OHM)
            .value("SIEMENS", Units::SIEMENS)
            .value("WEBER", Units::WEBER)
            .value("TESLA", Units::TESLA)
            .value("HENRY", Units::HENRY)
            .value("DEGREE_CELSIUS", Units::DEGREE_CELSIUS)
            .value("LUMEN", Units::LUMEN)
            .value("LUX", Units::LUX)
            .value("BECQUEREL", Units::BECQUEREL)
            .value("GRAY", Units::GRAY)
            .value("SIEVERT", Units::SIEVERT)
            .value("KATAL", Units::KATAL)
            .value("MINUTE", Units::MINUTE)
            .value("HOUR", Units::HOUR)
            .value("DAY", Units::DAY)
            .value("YEAR", Units::YEAR)
            .value("BAR", Units::BAR)
            .value("PIXEL", Units::PIXEL)
            .value("BYTE", Units::BYTE)
            .value("BIT", Units::BIT)
            .export_values()
            ;
    }
} //end  exportPyUtilSchema

