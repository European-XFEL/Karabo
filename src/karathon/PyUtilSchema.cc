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

        s.def("getAllowedStates"
              , &Schema::getAllowedStates
              , bp::return_value_policy< bp::copy_const_reference >());

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
        
        //all other get-s....

        //********* has methods ****************

        s.def("keyHasAlias", &Schema::keyHasAlias);
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

