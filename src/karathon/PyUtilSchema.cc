/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <iostream>
#include <karabo/io/InputElement.hh>
#include <karabo/io/OutputElement.hh>
#include <karabo/util/AlarmConditions.hh>
#include <karabo/util/ByteArrayElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/Factory.hh>
#include <karabo/util/HashFilter.hh>
#include <karabo/util/ListElement.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/OverwriteElement.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/RollingWindowStatistics.hh>
#include <karabo/util/TableElement.hh>
#include <karabo/util/Validator.hh>

#include "PythonMacros.hh"
#include "Wrapper.hh"
#include "boost/python/raw_function.hpp"

namespace bp = boost::python;
using namespace karabo::util;
using namespace karabo::io;
using namespace std;


struct SchemaWrapper : Schema, bp::wrapper<Schema> {
    SchemaWrapper(Schema const& arg) : Schema(arg), bp::wrapper<Schema>() {}


    SchemaWrapper() : Schema(), bp::wrapper<Schema>() {}


    SchemaWrapper(std::string const& classId, Schema::AssemblyRules const& rules)
        : Schema(classId, boost::ref(rules)), bp::wrapper<Schema>() {}


    virtual ClassInfo getClassInfo() const {
        if (bp::override func_getClassInfo = this->get_override("getClassInfo")) return func_getClassInfo();
        else return this->Schema::getClassInfo();
    }


    ClassInfo default_getClassInfo() const {
        return Schema::getClassInfo();
    }
};


class ValidatorWrap {
   public:
    static bp::tuple validate(Validator& self, const Schema& schema, const Hash& configuration,
                              const bp::object& stamp) {
        Hash::Pointer validated = Hash::Pointer(new Hash);
        Timestamp tstamp;
        if (stamp.ptr() != Py_None && bp::extract<karabo::util::Timestamp>(stamp).check()) {
            tstamp = bp::extract<karabo::util::Timestamp>(stamp);
        }

        pair<bool, string> result = self.validate(schema, configuration, *validated, tstamp);
        bp::tuple t = bp::make_tuple(result.first, result.second, bp::object(validated));
        return t;
    }


    static void setValidationRules(Validator& self, const bp::object& obj) {
        if (bp::extract<Validator::ValidationRules>(obj).check()) {
            const Validator::ValidationRules& rules = bp::extract<Validator::ValidationRules>(obj);
            self.setValidationRules(rules);
        }
    }


    static bp::object getValidationRules(Validator& self) {
        return bp::object(self.getValidationRules());
    }


    static bp::object hasParametersInWarnOrAlarm(Validator& self) {
        return bp::object(self.hasParametersInWarnOrAlarm());
    }


    static bp::object getParametersInWarnOrAlarm(Validator& self) {
        return bp::object(self.getParametersInWarnOrAlarm());
    }


    static bp::object hasReconfigurableParameter(Validator& self) {
        return bp::object(self.hasReconfigurableParameter());
    }


    static const RollingWindowStatistics& getRollingStatistics(Validator& self, const std::string& path) {
        return *self.getRollingStatistics(path);
    }
};


struct NodeElementWrap {
    static NodeElement& appendParametersOfConfigurableClass(NodeElement& self, const bp::object& baseobj,
                                                            const std::string& classId) {
        if (!PyType_Check(baseobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Argument 'arg1' given in 'appendParametersOfConfigurableClass(arg1, arg2)' of NODE_ELEMENT must be "
                  "a class in Python registered as base class in Configurator");
        }
        if (!PyObject_HasAttrString(baseobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class with classid = '" + classId +
                                          "' given in 'appendParametersOfConfigurableClass(base, classid)' of "
                                          "NODE_ELEMENT has no 'getSchema' method.");
        }
        std::string baseClassId;
        if (PyObject_HasAttrString(baseobj.ptr(), "__karabo_cpp_classid__")) {
            // baseobj is object of C++ base class
            baseClassId = bp::extract<std::string>(baseobj.attr("__karabo_cpp_classid__"));
        } else {
            baseClassId = bp::extract<std::string>(baseobj.attr("__classid__"));
        }
        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
        self.getNode().setAttribute(KARABO_SCHEMA_CLASS_ID, classId);
        self.getNode().setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, baseClassId);

        bp::object schemaObj = baseobj.attr("getSchema")(classId);

        const Schema schema = bp::extract<Schema>(schemaObj);
        const Hash h = schema.getParameterHash();
        self.getNode().setValue<Hash>(h);

        return self;
    }


    static NodeElement& appendParametersOf(NodeElement& self, const bp::object& obj) {
        if (!PyType_Check(obj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Argument 'arg' given in 'appendParametersOf(arg)' of NODE_ELEMENT must be a class in Python");
        }

        std::string classId(bp::extract<std::string>(obj.attr("__name__")));

        Schema::Pointer schema(new Schema());
        obj.attr("expectedParameters")(schema);
        self.getNode().setValue<Hash>(schema->getParameterHash());
        self.getNode().setAttribute(KARABO_SCHEMA_CLASS_ID, classId);
        self.getNode().setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, classId);
        return self;
    }


    static NodeElement& appendSchema(NodeElement& self, const bp::object& schemaObj) {
        if (!bp::extract<Schema>(schemaObj).check()) throw KARABO_PYTHON_EXCEPTION("Argument is not a Schema object.");
        const Schema schemaPy = bp::extract<Schema>(schemaObj);
        const Hash h = schemaPy.getParameterHash();
        self.getNode().setValue<Hash>(h);
        return self;
    }

    static NodeElement& setDaqDataType(NodeElement& self, const bp::object& dataTypeObj) {
        bp::extract<DaqDataType> getDaqType(dataTypeObj);
        if (!getDaqType.check()) {
            throw KARABO_PYTHON_EXCEPTION("Argument is not a DaqDataType object.");
        }
        const DaqDataType dataType = getDaqType();
        return self.setDaqDataType(dataType);
    }

    static NodeElement& setSpecialDisplayType(NodeElement& self, const std::string& displayType) {
        self.setSpecialDisplayType(displayType);
        return self;
    }


    static NodeElement& setAllowedActions(NodeElement& self, const bp::object& actions) {
        // Accept any Python iterable that provides strings
        return self.setAllowedActions(karathon::Wrapper::fromPyIterableToCppContainer<std::string>(actions));
    }
};


struct ChoiceElementWrap {
    static ChoiceElement& appendNodesOfConfigurationBase(ChoiceElement& self, const bp::object& classobj) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Argument 'arg' given in 'appendNodesOfConfigurationBase(arg)' of CHOICE_ELEMENT must be a class in "
                  "Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Class given in 'appendNodesOfConfigurationBase' of CHOICE_ELEMENT must have 'getSchema' function");
        }
        // TODO This whole code block repeats over and over!! TERRIBLE!! Cleaning needed.
        std::string classid;
        if (PyObject_HasAttrString(classobj.ptr(), "__karabo_cpp_classid__")) {
            classid = bp::extract<std::string>(classobj.attr("__karabo_cpp_classid__"));
        } else {
            classid = bp::extract<std::string>(classobj.attr("__classid__"));
        }

        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
        Hash& choiceOfNodes = self.getNode().getValue<Hash>();

        bp::object nodeNameList = classobj.attr("getRegisteredClasses")();
        boost::any any;
        karathon::Wrapper::toAny(nodeNameList, any);

        if (any.type() != typeid(std::vector<std::string>))
            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
        const vector<string>& nodeNames = boost::any_cast<vector<string>>(any);
        for (size_t i = 0; i < nodeNames.size(); i++) {
            std::string nodeName = nodeNames[i];
            bp::object schemaObj = classobj.attr("getSchema")(nodeName, bp::object(Schema::AssemblyRules()));
            const Schema& schema = bp::extract<const Schema&>(schemaObj);
            Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
            node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
            node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
            node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
            node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        }
        return self;
    }


    static ChoiceElement& appendAsNode(ChoiceElement& self, const bp::object& classobj,
                                       const std::string& nodeNameObj) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Argument 'classobj' given in 'appendAsNode(classobj, nodeName)' of CHOICE_ELEMENT must be a class "
                  "in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Class given in 'appendAsNode(classobj, nodeName)' of CHOICE_ELEMENT has no 'getSchema' method");
        }
        std::string classid;
        if (PyObject_HasAttrString(classobj.ptr(), "__karabo_cpp_classid__")) {
            classid = bp::extract<std::string>(classobj.attr("__karabo_cpp_classid__"));
        } else {
            classid = bp::extract<std::string>(classobj.attr("__classid__"));
        }
        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
        Hash& choiceOfNodes = self.getNode().getValue<Hash>();
        string nodeName = nodeNameObj;
        if (nodeNameObj == "") nodeName = classid;
        bp::object schemaObj = classobj.attr("getSchema")(nodeName, bp::object(Schema::AssemblyRules()));
        const Schema& schema = bp::extract<const Schema&>(schemaObj);
        Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
        node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
        node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
        node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
        node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        return self;
    }
};


struct ListElementWrap {
    static ListElement& appendNodesOfConfigurationBase(ListElement& self, const bp::object& classobj) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Argument 'arg' given in 'appendNodesOfConfigurationBase(arg)' of LIST_ELEMENT must be a class in "
                  "Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Class given in 'appendNodesOfConfigurationBase' of LIST_ELEMENT has no 'getSchema' method");
        }
        std::string classid;
        if (PyObject_HasAttrString(classobj.ptr(), "__karabo_cpp_classid__")) {
            classid = bp::extract<std::string>(classobj.attr("__karabo_cpp_classid__"));
        } else {
            classid = bp::extract<std::string>(classobj.attr("__classid__"));
        }

        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
        Hash& choiceOfNodes = self.getNode().getValue<Hash>();

        bp::object nodeNameList = classobj.attr("getRegisteredClasses")();
        boost::any any;
        karathon::Wrapper::toAny(nodeNameList, any);

        if (any.type() != typeid(vector<string>))
            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
        const vector<string>& nodeNames = boost::any_cast<vector<string>>(any);
        for (size_t i = 0; i < nodeNames.size(); i++) {
            std::string nodeName = nodeNames[i];
            bp::object schemaObj = classobj.attr("getSchema")(nodeName, Schema::AssemblyRules());
            const Schema& schema = bp::extract<const Schema&>(schemaObj);
            Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
            node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
            node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
            node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
            node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        }
        return self;
    }


    static ListElement& appendAsNode(ListElement& self, const bp::object& classobj, const std::string& name) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Argument 'classobj' given in 'appendAsNode(classobj, nodeName)' of LIST_ELEMENT must be a class in "
                  "Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Class given in 'appendAsNode(classobj, nodeName)' of LIST_ELEMENT has no 'getSchema' method");
        }
        std::string classid;
        if (PyObject_HasAttrString(classobj.ptr(), "__karabo_cpp_classid__")) {
            classid = bp::extract<std::string>(classobj.attr("__karabo_cpp_classid__"));
        } else {
            classid = bp::extract<std::string>(classobj.attr("__classid__"));
        }
        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
        Hash& choiceOfNodes = self.getNode().getValue<Hash>();
        string nodeName = name;
        if (nodeName == "") nodeName = classid;
        bp::object schemaObj = classobj.attr("getSchema")(nodeName, Schema::AssemblyRules());
        const Schema& schema = bp::extract<const Schema&>(schemaObj);
        Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
        node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
        node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
        node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
        node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        return self;
    }

    typedef DefaultValue<ListElement, vector<string>> DefListElement;


    static ListElement& defaultValueList(DefListElement& self, const bp::object& obj) {
        if (PyList_Check(obj.ptr())) {
            vector<string> v = karathon::Wrapper::fromPyListToStdVector<string>(obj);
            return self.defaultValue(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the defaultValue of LIST_ELEMENT must be a list of strings");
        }
    }
};


struct InputElementWrap {
    static InputElement& setInputType(InputElement& self, const bp::object& classobj) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Argument 'classobj' given in 'setInputType(classobj)' of INPUT_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Class given in 'setInputType(classobj)' of INPUT_ELEMENT has no 'getSchema' method");
        }
        std::string classid;
        if (PyObject_HasAttrString(classobj.ptr(), "__karabo_cpp_classid__")) {
            classid = bp::extract<std::string>(classobj.attr("__karabo_cpp_classid__"));
        } else {
            classid = bp::extract<std::string>(classobj.attr("__classid__"));
        }
        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
        Hash& choiceOfNodes = self.getNode().getValue<Hash>();

        bp::object nodeNameList = classobj.attr("getRegisteredClasses")();
        boost::any any;
        karathon::Wrapper::toAny(nodeNameList, any);

        if (any.type() != typeid(vector<string>))
            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
        const vector<string>& nodeNames = boost::any_cast<vector<string>>(any);
        for (size_t i = 0; i < nodeNames.size(); i++) {
            std::string nodeName = nodeNames[i];
            bp::object schemaObj = classobj.attr("getSchema")(nodeName);
            const Schema& schema = bp::extract<const Schema&>(schemaObj);
            Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
            node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
            node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Input-" + nodeName);
            node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
            node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        }
        return self;
    }
};


struct OutputElementWrap {
    static OutputElement& setOutputType(OutputElement& self, const bp::object& classobj) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Argument 'classobj' given in 'setOutputType(classobj)' of OUTPUT_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Class given in 'setOutputType(classobj)' of OUTPUT_ELEMENT has no 'getSchema' method");
        }
        std::string classid;
        if (PyObject_HasAttrString(classobj.ptr(), "__karabo_cpp_classid__")) {
            classid = bp::extract<std::string>(classobj.attr("__karabo_cpp_classid__"));
        } else {
            classid = bp::extract<std::string>(classobj.attr("__classid__"));
        }
        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
        // Retrieve reference for filling
        Hash& choiceOfNodes = self.getNode().getValue<Hash>();

        bp::object nodeNameList = classobj.attr("getRegisteredClasses")();
        boost::any any;
        karathon::Wrapper::toAny(nodeNameList, any);

        if (any.type() != typeid(vector<string>))
            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
        const vector<string>& nodeNames = boost::any_cast<vector<string>>(any);
        for (size_t i = 0; i < nodeNames.size(); i++) {
            std::string nodeName = nodeNames[i];
            bp::object schemaObj = classobj.attr("getSchema")(nodeName);
            const Schema& schema = bp::extract<const Schema&>(schemaObj);
            Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
            node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
            node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Output-" + nodeName);
            node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
            node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        }
        return self;
    }
};


struct OverwriteElementWrap {
    static OverwriteElement& setNewAlias(OverwriteElement& self, const bp::object& alias) {
        boost::any any;
        karathon::Wrapper::toAny(alias, any);
        return self.setNewAlias(any);
    }

    static OverwriteElement& setNewTags(OverwriteElement& self, const bp::object& tags) {
        if (PyUnicode_Check(tags.ptr())) {
            std::string tag = bp::extract<std::string>(tags);
            return self.setNewTags({tag});
        }
        try {
            return self.setNewTags(karathon::Wrapper::fromPyIterableToCppContainer<std::string>(tags));
        } catch (...) {
            throw KARABO_CAST_EXCEPTION("setNewTags expects new tags in a single str or a iterable of str");
            return self; // please compiler
        }
    }

    static OverwriteElement& setNewDefaultValue(OverwriteElement& self, const bp::object& value) {
        const std::string className = bp::extract<std::string>(value.attr("__class__").attr("__name__"));
        if (className == "State") {
            const std::string state = bp::extract<std::string>(value.attr("name"));
            return self.setNewDefaultValue(karabo::util::State::fromString(state));
        } else if (className == "AlarmCondition") {
            const std::string condition = bp::extract<std::string>(value.attr("value"));
            return self.setNewDefaultValue(karabo::util::AlarmCondition::fromString(condition));
        } else {
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            return self.setNewDefaultValue(any);
        }
    }


    static OverwriteElement& setNewMinInc(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewMinInc(any);
    }


    static OverwriteElement& setNewMaxInc(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewMaxInc(any);
    }


    static OverwriteElement& setNewMinExc(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewMinExc(any);
    }


    static OverwriteElement& setNewMaxExc(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewMaxExc(any);
    }


    static OverwriteElement& setNewMinSize(OverwriteElement& self, const bp::object& obj) {
        try {
            return self.setNewMinSize(karathon::Wrapper::toInteger<unsigned int>(obj));
        } catch (const karabo::util::CastException& e) {
            KARABO_RETHROW_AS(e);
            return self; // please compiler
        }
    }


    static OverwriteElement& setNewMaxSize(OverwriteElement& self, const bp::object& obj) {
        try {
            return self.setNewMaxSize(karathon::Wrapper::toInteger<unsigned int>(obj));
        } catch (const karabo::util::CastException& e) {
            KARABO_RETHROW_AS(e);
            return self; // please compiler
        }
    }


    static bp::object setNewAllowedStates(bp::tuple args, bp::dict kwargs) {
        OverwriteElement& self = bp::extract<OverwriteElement&>(args[0]);
        std::vector<karabo::util::State> states;
        for (unsigned int i = 1; i < bp::len(args); ++i) {
            const std::string state = bp::extract<std::string>(args[i].attr("name"));
            states.push_back(karabo::util::State::fromString(state));
        }
        self.setNewAllowedStates(states);
        return args[0];
    }


    static bp::object setNewOptions(bp::tuple args, bp::dict kwargs) {
        OverwriteElement& self = bp::extract<OverwriteElement&>(args[0]);
        // get type of first arg

        bp::extract<const std::string> first_arg(args[1]);
        if (first_arg.check()) {
            self.setNewOptions(first_arg(), ",;");
            return args[0];
        } else {
            // try states
            std::vector<karabo::util::State> states;
            for (unsigned int i = 1; i < bp::len(args); ++i) {
                const std::string className = bp::extract<std::string>(args[i].attr("__class__").attr("__name__"));
                if (className == "State") {
                    const std::string state = bp::extract<std::string>(args[i].attr("name"));
                    states.push_back(karabo::util::State::fromString(state));
                } else {
                    throw KARABO_PYTHON_EXCEPTION(
                          "setNewOptions expects either a string or an arbitrary number of arguments of type State.");
                }
            }
            self.setNewOptions(states);
            return args[0];
        }
    }
};


struct ReadOnlySpecificTableWrap {
    static ReadOnlySpecific<TableElement, std::vector<Hash>>& initialValueTable(
          ReadOnlySpecific<TableElement, std::vector<Hash>>& self, const bp::object& obj) {
        if (PyList_Check(obj.ptr())) {
            vector<Hash> v = karathon::Wrapper::fromPyListToStdVector<Hash>(obj);
            return self.initialValue(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of initialValue for a read-only table must be a list of Hashes");
        }
    }
};


namespace tableElementWrap {


    static bp::object allowedStatesPy(bp::tuple args, bp::dict kwargs) {
        TableElement& self = bp::extract<TableElement&>(args[0]);
        std::vector<karabo::util::State> states;
        for (unsigned int i = 1; i < bp::len(args); ++i) {
            const std::string state = bp::extract<std::string>(args[i].attr("name"));
            states.push_back(karabo::util::State::fromString(state));
        }
        self.allowedStates(states);
        return args[0];
    }
} // namespace tableElementWrap


namespace schemawrap {


    void setAlias(Schema& self, const bp::object& obj, const bp::object& aliasObj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            if (PyLong_Check(aliasObj.ptr())) {
                int alias = bp::extract<int>(aliasObj);
                self.setAlias(path, alias);
            } else if (PyUnicode_Check(aliasObj.ptr())) {
                std::string alias = bp::extract<std::string>(aliasObj);
                self.setAlias(path, alias);
            } else if (PyFloat_Check(aliasObj.ptr())) {
                double alias = bp::extract<double>(aliasObj);
                self.setAlias(path, alias);
            } else if (PyList_Check(aliasObj.ptr())) {
                bp::ssize_t size = bp::len(aliasObj);
                if (size == 0) {
                    std::vector<std::string> alias = std::vector<std::string>();
                    self.setAlias(path, alias);
                    return;
                }
                bp::object list0 = aliasObj[0];
                if (list0.ptr() == Py_None) {
                    std::vector<CppNone> v;
                    for (bp::ssize_t i = 0; i < size; ++i) v.push_back(CppNone());
                    self.setAlias(path, v);
                    return;
                }
                if (PyBool_Check(list0.ptr())) {
                    std::vector<bool> v(size); // Special case here
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<bool>(aliasObj[i]);
                    }
                    self.setAlias(path, v);
                    return;
                }
                if (PyLong_Check(list0.ptr())) {
                    std::vector<int> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<int>(aliasObj[i]);
                    }
                    self.setAlias(path, v);
                    return;
                }
                if (PyFloat_Check(list0.ptr())) {
                    std::vector<double> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<double>(aliasObj[i]);
                    }
                    self.setAlias(path, v);
                    return;
                }
                if (PyLong_Check(list0.ptr())) {
                    std::vector<long long> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<long>(aliasObj[i]);
                    }
                    self.setAlias(path, v);
                    return;
                }
                if (PyUnicode_Check(list0.ptr())) {
                    std::vector<std::string> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<std::string>(aliasObj[i]);
                    }
                    self.setAlias(path, v);
                    return;
                }
                if (PyUnicode_Check(list0.ptr())) {
                    std::vector<std::string> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        bp::object str(
                              bp::handle<>(PyUnicode_AsUTF8String(static_cast<bp::object>(aliasObj[i]).ptr())));
                        Py_ssize_t size;
                        const char* data = PyUnicode_AsUTF8AndSize(str.ptr(), &size);
                        v[i] = string(data, size);
                    }
                    self.setAlias(path, v);
                    return;
                }
            } else {
                throw KARABO_PYTHON_EXCEPTION("Unknown data type of the 'alias' argument");
            }
        } else throw KARABO_PYTHON_EXCEPTION("Python argument defining the key name in 'setAlias' should be a string");
    }


    bp::object getParameterHash(const Schema& schema) {
        return bp::object(schema.getParameterHash());
    }


    karathon::PyTypes::ReferenceType getValueType(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            Types::ReferenceType t = schema.getValueType(path);
            return karathon::PyTypes::from(t);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getValueType' must be a string");
    }

    //*********************************************************************
    // Wrapper functions for : getMinInc, getMaxInc, getMinExc, getMaxExc *
    //*********************************************************************


    void setMinInc(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setMinInc(path, any);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
    }


    bp::object getMinInc(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_MIN_INC), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getMinInc' must be a string");
    }


    void setMaxInc(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setMaxInc(path, any);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
    }


    bp::object getMaxInc(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_MAX_INC), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getMaxInc' must be a string");
    }


    void setMinExc(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setMinExc(path, any);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
    }


    bp::object getMinExc(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_MIN_EXC), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getMinExc' must be a string");
    }


    void setMaxExc(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setMaxExc(path, any);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
    }


    bp::object getMaxExc(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_MAX_EXC), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getMaxExc' must be a string");
    }

    //*****************************************************************************
    // Wrapper functions for : getMinIncAs, getMaxIncAs, getMinExcAs, getMaxExcAs *
    //*****************************************************************************


    bp::object getMinIncAs(const Schema& schema, const bp::object& obj,
                           const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            switch (pytype) {
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getMinIncAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getMinIncAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getMinIncAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getMinIncAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getMinIncAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getMinIncAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getMinIncAs<double>(path));
                default:
                    break;
            }
            throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getMinIncAs' must be a string");
    }


    bp::object getMaxIncAs(const Schema& schema, const bp::object& obj,
                           const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            switch (pytype) {
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getMaxIncAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getMaxIncAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getMaxIncAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getMaxIncAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getMaxIncAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getMaxIncAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getMaxIncAs<double>(path));
                default:
                    break;
            }
            throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getMaxIncAs' must be a string");
    }


    bp::object getMinExcAs(const Schema& schema, const bp::object& obj,
                           const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            switch (pytype) {
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getMinExcAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getMinExcAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getMinExcAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getMinExcAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getMinExcAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getMinExcAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getMinExcAs<double>(path));
                default:
                    break;
            }
            throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getMinExcAs' must be a string");
    }


    bp::object getMaxExcAs(const Schema& schema, const bp::object& obj,
                           const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            switch (pytype) {
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getMaxExcAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getMaxExcAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getMaxExcAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getMaxExcAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getMaxExcAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getMaxExcAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getMaxExcAs<double>(path));
                default:
                    break;
            }
            throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getMaxExcAs' must be a string");
    }


    //*****************************************************************************
    // Wrapper functions for : getWarnLow, getWarnHigh, getAlarmLow, getAlarmHigh *
    //*****************************************************************************


    void setWarnLow(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setWarnLow(path, any);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setWarnLow' must be a string");
    }


    bp::object getWarnLow(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, AlarmCondition::WARN_LOW.asString()), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getWarnLow' must be a string");
    }


    void setWarnHigh(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setWarnHigh(path, any);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setWarnHigh' must be a string");
    }


    bp::object getWarnHigh(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, AlarmCondition::WARN_HIGH.asString()), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getWarnHigh' must be a string");
    }


    void setAlarmLow(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setAlarmLow(path, any);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setAlarmLow' must be a string");
    }


    bp::object getAlarmLow(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, AlarmCondition::ALARM_LOW.asString()), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAlarmLow' must be a string");
    }


    void setAlarmHigh(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setAlarmHigh(path, any);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setAlarmHigh' must be a string");
    }


    bp::object getAlarmHigh(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, AlarmCondition::ALARM_HIGH.asString()), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAlarmHigh' must be a string");
    }


    void setWarnVarianceLow(Schema& self, const bp::object& obj, const double value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            self.setWarnVarianceLow(path, value);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setWarnVarianceLow' must be a string");
    }


    bp::object getWarnVarianceLow(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, AlarmCondition::WARN_VARIANCE_LOW.asString()),
                                               false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getWarnVarianceLow' must be a string");
    }


    void setWarnVarianceHigh(Schema& self, const bp::object& obj, const double value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            self.setWarnVarianceHigh(path, value);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setWarnVarianceHigh' must be a string");
    }


    bp::object getWarnVarianceHigh(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, AlarmCondition::WARN_VARIANCE_HIGH.asString()),
                                               false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getWarnVarianceHigh' must be a string");
    }


    void setAlarmVarianceLow(Schema& self, const bp::object& obj, const double value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            self.setAlarmVarianceLow(path, value);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setAlarmVarianceLow' must be a string");
    }


    bp::object getAlarmVarianceLow(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, AlarmCondition::ALARM_VARIANCE_LOW.asString()),
                                               false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAlarmVarianceLow' must be a string");
    }


    void setAlarmVarianceHigh(Schema& self, const bp::object& obj, const double value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            self.setAlarmVarianceHigh(path, value);
        } else throw KARABO_PYTHON_EXCEPTION("Python argument in 'setAlarmVarianceHigh' must be a string");
    }


    bp::object getAlarmVarianceHigh(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(
                  h.getAttributeAsAny(path, AlarmCondition::ALARM_VARIANCE_HIGH.asString()), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAlarmVarianceHigh' must be a string");
    }


    //*************************************************************************************
    // Wrapper functions for : getWarnLowAs, getWarnHighAs, getAlarmLowAs, getAlarmHighAs *
    //*************************************************************************************


    bp::object getWarnLowAs(const Schema& schema, const bp::object& obj,
                            const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            switch (pytype) {
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getWarnLowAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getWarnLowAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getWarnLowAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getWarnLowAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getWarnLowAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getWarnLowAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getWarnLowAs<double>(path));
                default:
                    break;
            }
            throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getWarnLowAs' must be a string");
    }


    bp::object getWarnHighAs(const Schema& schema, const bp::object& obj,
                             const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            switch (pytype) {
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getWarnHighAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getWarnHighAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getWarnHighAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getWarnHighAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getWarnHighAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getWarnHighAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getWarnHighAs<double>(path));
                default:
                    break;
            }
            throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getWarnHighAs' must be a string");
    }


    bp::object getAlarmLowAs(const Schema& schema, const bp::object& obj,
                             const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            switch (pytype) {
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getAlarmLowAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getAlarmLowAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getAlarmLowAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getAlarmLowAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getAlarmLowAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getAlarmLowAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getAlarmLowAs<double>(path));
                default:
                    break;
            }
            throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getAlarmLowAs' must be a string");
    }


    bp::object getAlarmHighAs(const Schema& schema, const bp::object& obj,
                              const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            switch (pytype) {
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getAlarmHighAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getAlarmHighAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getAlarmHighAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getAlarmHighAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getAlarmHighAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getAlarmHighAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getAlarmHighAs<double>(path));
                default:
                    break;
            }
            throw KARABO_PYTHON_EXCEPTION("Python Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getAlarmHighAs' must be a string");
    }


    //***********************************************************************************
    // Wrapper functions for : getKeys, getPaths, getTags, getOptions,                  *
    // getAllowedStates                                                                 *
    //***********************************************************************************


    bp::object getKeys(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            bp::list listParams;
            string path = bp::extract<string>(obj);
            const vector<string>& v = schema.getKeys(path);
            for (size_t i = 0; i < v.size(); i++) listParams.attr("append")(bp::object(v[i]));
            return listParams;
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getKeys' should be a string");
    }


    bp::object getPaths(const Schema& schema) {
        bp::list listParams;
        const vector<string>& v = schema.getPaths();
        for (size_t i = 0; i < v.size(); i++) listParams.attr("append")(bp::object(v[i]));
        return listParams;
    }


    bp::object getTags(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const std::vector<std::string>& v = schema.getTags(path);
            return karathon::Wrapper::fromStdVectorToPyArray<string>(v);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getTags' should be a string");
    }

    struct ConvertOptions {
        inline ConvertOptions(const string& path, const Schema& schema) : m_path(path), m_schema(schema) {}

        template <class T>
        inline void operator()(T*) {
            result = karathon::Wrapper::fromStdVectorToPyArray<T>(m_schema.getOptions<T>(m_path));
        }

        const string& m_path;
        const Schema& m_schema;
        bp::object result;
    };

    bp::object getOptions(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            ConvertOptions convertOptions(path, schema);
            templatize(schema.getValueType(path), convertOptions);
            return convertOptions.result;
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getOptions' should be a string");
    }


    bp::object getAllowedStates(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const std::string s = karabo::util::toString(schema.getAllowedStates(path));
            const vector<string> v = karabo::util::fromString<std::string, std::vector>(s);
            // now construct python states
            bp::list states;
            bp::object sModule = bp::import("karabo.common.states");
            for (vector<string>::const_iterator it = v.begin(); it != v.end(); ++it) {
                states.append(sModule.attr("State")(*it));
            }

            return states;
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAllowedStates' should be a string");
    }

    //*************************************************************
    // Wrapper functions for : setDefaultValue, getDefaultValue, getDefaultValueAs *
    //*************************************************************


    void setDefaultValue(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setDefaultValue(path, any);
        } else
            throw KARABO_PYTHON_EXCEPTION(
                  "Python argument defining the key name in 'setDefaultValue' should be a string");
    }


    bp::object getDefaultValue(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_DEFAULT_VALUE), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument defining the key name in 'getDefaultValue' should be a string");
    }


    bp::object getDefaultValueAs(const Schema& schema, const bp::object& obj,
                                 const karathon::PyTypes::ReferenceType& pytype) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);

            switch (pytype) {
                case karathon::PyTypes::BOOL:
                    return bp::object(schema.getDefaultValueAs<bool>(path));
                case karathon::PyTypes::INT32:
                    return bp::object(schema.getDefaultValueAs<int>(path));
                case karathon::PyTypes::UINT32:
                    return bp::object(schema.getDefaultValueAs<unsigned int>(path));
                case karathon::PyTypes::INT64:
                    return bp::object(schema.getDefaultValueAs<long long>(path));
                case karathon::PyTypes::UINT64:
                    return bp::object(schema.getDefaultValueAs<unsigned long long>(path));
                case karathon::PyTypes::STRING:
                    return bp::object(schema.getDefaultValueAs<string>(path));
                case karathon::PyTypes::FLOAT:
                    return bp::object(schema.getDefaultValueAs<float>(path));
                case karathon::PyTypes::DOUBLE:
                    return bp::object(schema.getDefaultValueAs<double>(path));
                case Types::VECTOR_BOOL:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<bool, vector>(path));
                case Types::VECTOR_INT32:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<int, vector>(path));
                case Types::VECTOR_UINT32:
                    return karathon::Wrapper::fromStdVectorToPyArray(
                          schema.getDefaultValueAs<unsigned int, vector>(path));
                case Types::VECTOR_INT64:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<long long, vector>(path));
                case Types::VECTOR_UINT64:
                    return karathon::Wrapper::fromStdVectorToPyArray(
                          schema.getDefaultValueAs<unsigned long long, vector>(path));
                case Types::VECTOR_STRING:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<string, vector>(path));
                case Types::VECTOR_FLOAT:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<float, vector>(path));
                case Types::VECTOR_DOUBLE:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<double, vector>(path));
                default:
                    break;
            }
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION(
              "Python first argument in 'getDefaultValueAs' should be a string (defining a key name)");
    }

    //************************************************************************
    // Wrapper functions for : aliasHasKey, getAliasFromKey, getKeyFromAlias *
    //************************************************************************


    bool aliasHasKey(const Schema& schema, const bp::object& obj) {
        if (PyLong_Check(obj.ptr())) {
            int param = bp::extract<int>(obj);
            return schema.aliasHasKey(param);
        } else if (PyUnicode_Check(obj.ptr())) {
            std::string param = bp::extract<std::string>(obj);
            return schema.aliasHasKey(param);
        } else if (PyFloat_Check(obj.ptr())) {
            double param = bp::extract<double>(obj);
            return schema.aliasHasKey(param);
        } else if (PyList_Check(obj.ptr())) {
            bp::ssize_t size = bp::len(obj);
            if (size == 0) {
                std::vector<std::string> params = std::vector<std::string>();
                return schema.aliasHasKey(params);
            }
            bp::object list0 = obj[0];
            if (list0.ptr() == Py_None) {
                std::vector<CppNone> v;
                for (bp::ssize_t i = 0; i < size; ++i) v.push_back(CppNone());
                return schema.aliasHasKey(v);
            }
            if (PyBool_Check(list0.ptr())) {
                std::vector<bool> v(size); // Special case here
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<bool>(obj[i]);
                }
                return schema.aliasHasKey(v);
            }
            if (PyLong_Check(list0.ptr())) {
                std::vector<int> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<int>(obj[i]);
                }
                return schema.aliasHasKey(v);
            }
            if (PyFloat_Check(list0.ptr())) {
                std::vector<double> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<double>(obj[i]);
                }
                return schema.aliasHasKey(v);
            }
            if (PyLong_Check(list0.ptr())) {
                std::vector<long long> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<long>(obj[i]);
                }
                return schema.aliasHasKey(v);
            }
            if (PyUnicode_Check(list0.ptr())) {
                std::vector<std::string> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<std::string>(obj[i]);
                }
                return schema.aliasHasKey(v);
            }
            if (PyUnicode_Check(list0.ptr())) {
                std::vector<std::string> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    bp::object str(bp::handle<>(PyUnicode_AsUTF8String(static_cast<bp::object>(obj[i]).ptr())));
                    Py_ssize_t size;
                    const char* data = PyUnicode_AsUTF8AndSize(str.ptr(), &size);
                    v[i] = string(data, size);
                }
                return schema.aliasHasKey(v);
            }
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'aliasHasKey': type is not supported");
    }


    bp::object getAliasFromKey(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_ALIAS), false);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAliasFromKey' should be a string");
        }
    }


    string getKeyFromAlias(const Schema& schema, const bp::object& obj) {
        if (PyLong_Check(obj.ptr())) {
            int param = bp::extract<int>(obj);
            return schema.getKeyFromAlias(param);
        } else if (PyUnicode_Check(obj.ptr())) {
            std::string param = bp::extract<std::string>(obj);
            return schema.getKeyFromAlias(param);
        } else if (PyFloat_Check(obj.ptr())) {
            double param = bp::extract<double>(obj);
            return schema.getKeyFromAlias(param);
        } else if (PyList_Check(obj.ptr())) {
            bp::ssize_t size = bp::len(obj);
            if (size == 0) {
                std::vector<std::string> params = std::vector<std::string>();
                return schema.getKeyFromAlias(params);
            }
            bp::object list0 = obj[0];
            if (list0.ptr() == Py_None) {
                std::vector<CppNone> v;
                for (bp::ssize_t i = 0; i < size; ++i) v.push_back(CppNone());
                return schema.getKeyFromAlias(v);
            }
            if (PyBool_Check(list0.ptr())) {
                std::vector<bool> v(size); // Special case here
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<bool>(obj[i]);
                }
                return schema.getKeyFromAlias(v);
            }
            if (PyLong_Check(list0.ptr())) {
                std::vector<int> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<int>(obj[i]);
                }
                return schema.getKeyFromAlias(v);
            }
            if (PyFloat_Check(list0.ptr())) {
                std::vector<double> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<double>(obj[i]);
                }
                return schema.getKeyFromAlias(v);
            }
            if (PyLong_Check(list0.ptr())) {
                std::vector<long long> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<long>(obj[i]);
                }
                return schema.getKeyFromAlias(v);
            }
            if (PyUnicode_Check(list0.ptr())) {
                std::vector<std::string> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<std::string>(obj[i]);
                }
                return schema.getKeyFromAlias(v);
            }
            if (PyUnicode_Check(list0.ptr())) {
                std::vector<std::string> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    bp::object str(bp::handle<>(PyUnicode_AsUTF8String(static_cast<bp::object>(obj[i]).ptr())));
                    Py_ssize_t size;
                    const char* data = PyUnicode_AsUTF8AndSize(str.ptr(), &size);
                    v[i] = string(data, size);
                }
                return schema.getKeyFromAlias(v);
            }
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getKeyFromAlias': type is not supported");
    }


    void help(Schema& schema, const std::string& classId = "") {
        schema.help(classId);
    }


    bp::object merge(Schema& schema, const Schema& schema2) {
        schema.merge(schema2);
        return bp::object(schema);
    }


    bp::object copy(Schema& schema, const Schema& schema2) {
        schema = schema2;
        return bp::object(schema);
    }


    void updateAliasMap(Schema& schema) {
        schema.updateAliasMap();
    }


    const std::string getInfoForAlarm(Schema& schema, const std::string& path, const bp::object condition) {
        const std::string className = bp::extract<std::string>(condition.attr("__class__").attr("__name__"));
        if (className != "AlarmCondition") {
            throw KARABO_PYTHON_EXCEPTION("Python argument for condition needs to be of type AlarmCondition and not " +
                                          className);
        }
        const std::string conditionName = bp::extract<std::string>(condition.attr("value"));
        return schema.getInfoForAlarm(path, karabo::util::AlarmCondition::fromString(conditionName));
    }

    const bool doesAlarmNeedAcknowledging(Schema& schema, const std::string& path, const bp::object condition) {
        const std::string className = bp::extract<std::string>(condition.attr("__class__").attr("__name__"));
        if (className != "AlarmCondition") {
            throw KARABO_PYTHON_EXCEPTION("Python argument for condition needs to be of type AlarmCondition and not " +
                                          className);
        }
        const std::string conditionName = bp::extract<std::string>(condition.attr("value"));
        return schema.doesAlarmNeedAcknowledging(path, karabo::util::AlarmCondition::fromString(conditionName));
    }


    void setAllowedStates(Schema& self, const std::string& path, PyObject* rargs) {
        bp::tuple args = bp::extract<bp::tuple>(rargs);
        std::vector<karabo::util::State> states;
        for (unsigned int i = 0; i < bp::len(args); ++i) {
            const std::string state = bp::extract<std::string>(args[i].attr("name"));
            states.push_back(karabo::util::State::fromString(state));
        }
        return self.setAllowedStates(path, states);
    }


    bp::object getAllowedActions(const Schema& self, const std::string& path) {
        const std::vector<string>& actions = self.getAllowedActions(path);
        return karathon::Wrapper::fromStdVectorToPyList(actions);
    }


    void setAllowedActions(Schema& self, const std::string& path, const bp::object& actions) {
        // Accept any Python iterable that provides strings
        self.setAllowedActions(path, karathon::Wrapper::fromPyIterableToCppContainer<std::string>(actions));
    }
} // namespace schemawrap


namespace ndarrayelementwrap {


    NDArrayElement& setAllowedActions(NDArrayElement& self, const bp::object& actions) {
        // Accept any Python iterable that provides strings
        return self.setAllowedActions(karathon::Wrapper::fromPyIterableToCppContainer<std::string>(actions));
    }
} // namespace ndarrayelementwrap


struct HashFilterWrap {
    static boost::shared_ptr<Hash> byTag(const Schema& schema, const Hash& config, const std::string& tags,
                                         const std::string& sep = ",") {
        boost::shared_ptr<Hash> result(new Hash);
        HashFilter::byTag(schema, config, *result, tags, sep);
        return result;
    }


    static boost::shared_ptr<Hash> byAccessMode(const Schema& schema, const Hash& config, const AccessType& value) {
        boost::shared_ptr<Hash> result(new Hash);
        HashFilter::byAccessMode(schema, config, *result, value);
        return result;
    }
};


void exportPyUtilSchema() {
    bp::enum_<AccessType>("AccessType").value("INIT", INIT).value("READ", READ).value("WRITE", WRITE).export_values();

    bp::enum_<DaqDataType>("DaqDataType")
          .value("PULSE", DaqDataType::PULSE)
          .value("TRAIN", DaqDataType::TRAIN)
          .value("PULSEMASTER", DaqDataType::PULSEMASTER)
          .value("TRAINMASTER", DaqDataType::TRAINMASTER);

    bp::enum_<DAQPolicy>("DAQPolicy")
          .value("UNSPECIFIED", DAQPolicy::UNSPECIFIED)
          .value("OMIT", DAQPolicy::OMIT)
          .value("SAVE", DAQPolicy::SAVE);

    {
        bp::enum_<MetricPrefixType>("MetricPrefix")
              .value("YOTTA", MetricPrefix::YOTTA)
              .value("ZETTA", MetricPrefix::ZETTA)
              .value("EXA", MetricPrefix::EXA)
              .value("PETA", MetricPrefix::PETA)
              .value("TERA", MetricPrefix::TERA)
              .value("GIGA", MetricPrefix::GIGA)
              .value("MEGA", MetricPrefix::MEGA)
              .value("KILO", MetricPrefix::KILO)
              .value("HECTO", MetricPrefix::HECTO)
              .value("DECA", MetricPrefix::DECA)
              .value("NONE", MetricPrefix::NONE)
              .value("DECI", MetricPrefix::DECI)
              .value("CENTI", MetricPrefix::CENTI)
              .value("MILLI", MetricPrefix::MILLI)
              .value("MICRO", MetricPrefix::MICRO)
              .value("NANO", MetricPrefix::NANO)
              .value("PICO", MetricPrefix::PICO)
              .value("FEMTO", MetricPrefix::FEMTO)
              .value("ATTO", MetricPrefix::ATTO)
              .value("ZEPTO", MetricPrefix::ZEPTO)
              .value("YOCTO", MetricPrefix::YOCTO)
              .export_values();
        bp::enum_<UnitType>("Unit")
              .value("NUMBER", Unit::NUMBER)
              .value("COUNT", Unit::COUNT)
              .value("METER", Unit::METER)
              .value("GRAM", Unit::GRAM)
              .value("SECOND", Unit::SECOND)
              .value("AMPERE", Unit::AMPERE)
              .value("KELVIN", Unit::KELVIN)
              .value("MOLE", Unit::MOLE)
              .value("CANDELA", Unit::CANDELA)
              .value("HERTZ", Unit::HERTZ)
              .value("RADIAN", Unit::RADIAN)
              .value("DEGREE", Unit::DEGREE)
              .value("STERADIAN", Unit::STERADIAN)
              .value("NEWTON", Unit::NEWTON)
              .value("PASCAL", Unit::PASCAL)
              .value("JOULE", Unit::JOULE)
              .value("ELECTRONVOLT", Unit::ELECTRONVOLT)
              .value("WATT", Unit::WATT)
              .value("COULOMB", Unit::COULOMB)
              .value("VOLT", Unit::VOLT)
              .value("FARAD", Unit::FARAD)
              .value("OHM", Unit::OHM)
              .value("SIEMENS", Unit::SIEMENS)
              .value("WEBER", Unit::WEBER)
              .value("TESLA", Unit::TESLA)
              .value("HENRY", Unit::HENRY)
              .value("DEGREE_CELSIUS", Unit::DEGREE_CELSIUS)
              .value("LUMEN", Unit::LUMEN)
              .value("LUX", Unit::LUX)
              .value("BECQUEREL", Unit::BECQUEREL)
              .value("GRAY", Unit::GRAY)
              .value("SIEVERT", Unit::SIEVERT)
              .value("KATAL", Unit::KATAL)
              .value("MINUTE", Unit::MINUTE)
              .value("HOUR", Unit::HOUR)
              .value("DAY", Unit::DAY)
              .value("YEAR", Unit::YEAR)
              .value("BAR", Unit::BAR)
              .value("PIXEL", Unit::PIXEL)
              .value("BYTE", Unit::BYTE)
              .value("BIT", Unit::BIT)
              .value("METER_PER_SECOND", Unit::METER_PER_SECOND)
              .value("VOLT_PER_SECOND", Unit::VOLT_PER_SECOND)
              .value("AMPERE_PER_SECOND", Unit::AMPERE_PER_SECOND)
              .value("PERCENT", Unit::PERCENT)
              .value("NOT_ASSIGNED", Unit::NOT_ASSIGNED)
              .export_values();
    }

    { //  Exposing std::vector<std::string>
        typedef bp::class_<std::vector<std::string>> VectorStringExposer;
        VectorStringExposer exposerOfVectorString = VectorStringExposer("VectorString");
        bp::scope scopeOfVectorString(exposerOfVectorString);
        exposerOfVectorString.def(bp::vector_indexing_suite<std::vector<std::string>, true>());
    }

    { // exposing ::Schema

        bp::class_<Schema, boost::shared_ptr<Schema>> s("Schema");
        s.def(bp::init<>());
        s.def(bp::init<std::string const&, bp::optional<Schema::AssemblyRules const&>>());

        bp::enum_<Schema::AssignmentType>("AssignmentType")
              .value("OPTIONAL", Schema::OPTIONAL_PARAM)
              .value("MANDATORY", Schema::MANDATORY_PARAM)
              .value("INTERNAL", Schema::INTERNAL_PARAM)
              .export_values();
        bp::enum_<Schema::AccessLevel>("AccessLevel")
              .value("OBSERVER", Schema::OBSERVER)
              .value("USER", Schema::USER)
              .value("OPERATOR", Schema::OPERATOR)
              .value("EXPERT", Schema::EXPERT)
              .value("ADMIN", Schema::ADMIN)
              .export_values();
        bp::enum_<Schema::LeafType>("LeafType")
              .value("PROPERTY", Schema::PROPERTY)
              .value("COMMAND", Schema::COMMAND)
              .value("STATE", Schema::STATE)
              .value("ALARM_CONDITION", Schema::ALARM_CONDITION)
              .export_values();
        bp::enum_<Schema::NodeType>("NodeType")
              .value("LEAF", Schema::LEAF)
              .value("NODE", Schema::NODE)
              .value("CHOICE_OF_NODES", Schema::CHOICE_OF_NODES)
              .value("LIST_OF_NODES", Schema::LIST_OF_NODES)
              .export_values();
        bp::enum_<Schema::ArchivePolicy>("ArchivePolicy")
              .value("EVERY_EVENT", Schema::EVERY_EVENT)
              .value("EVERY_100MS", Schema::EVERY_100MS)
              .value("EVERY_1S", Schema::EVERY_1S)
              .value("EVERY_5S", Schema::EVERY_5S)
              .value("EVERY_10S", Schema::EVERY_10S)
              .value("EVERY_1MIN", Schema::EVERY_1MIN)
              .value("EVERY_10MIN", Schema::EVERY_10MIN)
              .value("NO_ARCHIVING", Schema::NO_ARCHIVING)
              .export_values();
        bp::class_<Schema::AssemblyRules>("AssemblyRules",
                                          bp::init<bp::optional<AccessType const&, std::string const&, const int>>(
                                                (bp::arg("accessMode") = operator|(INIT, WRITE), bp::arg("state") = "",
                                                 bp::arg("accessLevel") = -1)))
              .def_readwrite("m_accessMode", &Schema::AssemblyRules::m_accessMode)
              .def_readwrite("m_accessLevel", &Schema::AssemblyRules::m_accessLevel)
              .def_readwrite("m_state", &Schema::AssemblyRules::m_state);

        s.def(bp::self_ns::str(bp::self));


        //********* General functions on Schema *******

        s.def("has", (bool(Schema::*)(string const&) const)(&Schema::has), bp::arg("path"));

        s.def("__contains__", (bool(Schema::*)(string const&) const)(&Schema::has), bp::arg("path"));

        s.def("empty", (bool(Schema::*)() const)(&Schema::empty));

        s.def("merge", (void(Schema::*)(Schema const&))(&Schema::merge), bp::arg("schema"));

        s.def("__iadd__", &schemawrap::merge, bp::arg("schema"));

        s.def("copy", &schemawrap::copy, bp::arg("schema"));

        //********* 'get'-methods *********************
        s.def("getRequiredAccessLevel", &Schema::getRequiredAccessLevel);

        s.def("getParameterHash", &schemawrap::getParameterHash);

        s.def("getAccessMode", &Schema::getAccessMode);

        s.def("getAssemblyRules", &Schema::getAssemblyRules);

        s.def("getAssignment", &Schema::getAssignment);

        s.def("getSkipValidation", &Schema::getSkipValidation);

        s.def("getDescription", &Schema::getDescription, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getDisplayType", &Schema::getDisplayType, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getDisplayedName", &Schema::getDisplayedName, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getUnit", (const int (Schema::*)(const std::string& path) const) & Schema::getUnit, (bp::arg("path")));

        s.def("getUnitName", &Schema::getUnitName, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getUnitSymbol", &Schema::getUnitSymbol, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getMetricPrefix", &Schema::getMetricPrefix);

        s.def("getMetricPrefixName", &Schema::getMetricPrefixName, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getMetricPrefixSymbol", &Schema::getMetricPrefixSymbol,
              bp::return_value_policy<bp::copy_const_reference>());

        s.def("getRootName", &Schema::getRootName, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getValueType", &schemawrap::getValueType, (bp::arg("path")));

        s.def("getNodeType", &Schema::getNodeType);

        s.def("getMinInc", &schemawrap::getMinInc, (bp::arg("path")));

        s.def("getMaxInc", &schemawrap::getMaxInc);

        s.def("getMinExc", &schemawrap::getMinExc);

        s.def("getMaxExc", &schemawrap::getMaxExc);

        s.def("getMinIncAs", &schemawrap::getMinIncAs, (bp::arg("path"), bp::arg("pytype")));

        s.def("getMaxIncAs", &schemawrap::getMaxIncAs, (bp::arg("path"), bp::arg("pytype")));

        s.def("getMinExcAs", &schemawrap::getMinExcAs, (bp::arg("path"), bp::arg("pytype")));

        s.def("getMaxExcAs", &schemawrap::getMaxExcAs, (bp::arg("path"), bp::arg("pytype")));

        s.def("getAliasAsString", &Schema::getAliasAsString);

        s.def("getKeys", &schemawrap::getKeys, (bp::arg("path") = ""));

        s.def("getPaths", &schemawrap::getPaths);

        s.def("getOptions", &schemawrap::getOptions);

        s.def("getTags", &schemawrap::getTags);

        s.def("getAllowedStates", &schemawrap::getAllowedStates);

        s.def("getDefaultValue", &schemawrap::getDefaultValue);

        s.def("getDefaultValueAs", &schemawrap::getDefaultValueAs);

        s.def("getMin", &Schema::getMin, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getMax", &Schema::getMax, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getMinSize", (const unsigned int& (Schema::*)(const string&) const)(&Schema::getMinSize),
              bp::return_value_policy<bp::copy_const_reference>());

        s.def("getMaxSize", (const unsigned int& (Schema::*)(const string&) const)(&Schema::getMaxSize),
              bp::return_value_policy<bp::copy_const_reference>());

        s.def("getArchivePolicy", &Schema::getArchivePolicy, bp::return_value_policy<bp::copy_const_reference>());

        s.def("getWarnLow", &schemawrap::getWarnLow);

        s.def("getWarnHigh", &schemawrap::getWarnHigh);

        s.def("getAlarmLow", &schemawrap::getAlarmLow);

        s.def("getAlarmHigh", &schemawrap::getAlarmHigh);

        s.def("getWarnVarianceLow", &Schema::getWarnVarianceLow);

        s.def("getWarnVarianceHigh", &Schema::getWarnVarianceHigh);

        s.def("getAlarmVarianceLow", &Schema::getAlarmVarianceLow);

        s.def("getAlarmVarianceHigh", &Schema::getAlarmVarianceHigh);

        s.def("getWarnLowAs", &schemawrap::getWarnLowAs, (bp::arg("path"), bp::arg("pytype")));

        s.def("getWarnHighAs", &schemawrap::getWarnHighAs, (bp::arg("path"), bp::arg("pytype")));

        s.def("getAlarmLowAs", &schemawrap::getAlarmLowAs, (bp::arg("path"), bp::arg("pytype")));

        s.def("getAlarmHighAs", &schemawrap::getAlarmHighAs, (bp::arg("path"), bp::arg("pytype")));


        //********* 'has'-methods ****************

        s.def("keyHasAlias", &Schema::keyHasAlias, (bp::arg("key")));

        s.def("aliasHasKey", &schemawrap::aliasHasKey, (bp::arg("alias")));

        s.def("getAliasFromKey", &schemawrap::getAliasFromKey, (bp::arg("key")));

        s.def("getKeyFromAlias", &schemawrap::getKeyFromAlias, (bp::arg("alias")));

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

        s.def("hasWarnLow", &Schema::hasWarnLow);

        s.def("hasWarnHigh", &Schema::hasWarnHigh);

        s.def("hasAlarmLow", &Schema::hasAlarmLow);

        s.def("hasAlarmHigh", &Schema::hasAlarmHigh);

        s.def("hasWarnVarianceLow", &Schema::hasWarnVarianceLow);

        s.def("hasWarnVarianceHigh", &Schema::hasWarnVarianceHigh);

        s.def("hasAlarmVarianceLow", &Schema::hasAlarmVarianceLow);

        s.def("hasAlarmVarianceHigh", &Schema::hasAlarmVarianceHigh);

        s.def("hasArchivePolicy", &Schema::hasArchivePolicy);

        s.def("hasDisplayedName", &Schema::hasDisplayedName);

        s.def("hasDisplayType", &Schema::hasDisplayType);

        s.def("hasDescription", &Schema::hasDescription);

        s.def("hasMin", &Schema::hasMin);

        s.def("hasMax", &Schema::hasMax);

        s.def("hasMinSize", &Schema::hasMinSize);

        s.def("hasMaxSize", &Schema::hasMaxSize);

        //********* 'is'-methods ****************

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
        s.def("isCommand", &Schema::isCommand);
        s.def("isProperty", &Schema::isProperty);

        //********* Help function to show all parameters *******
        s.def("help", &schemawrap::help, (bp::arg("classId") = ""));

        s.def("applyRuntimeUpdates", &Schema::applyRuntimeUpdates, bp::arg("updates"));

        s.def("getClassInfo", (ClassInfo(Schema::*)() const)(&Schema::getClassInfo),
              (ClassInfo(SchemaWrapper::*)() const)(&SchemaWrapper::default_getClassInfo));

        s.def("classInfo", (ClassInfo(*)())(&Schema::classInfo)).staticmethod("classInfo");

        s.def("setAssemblyRules", (void(Schema::*)(const Schema::AssemblyRules&)) & Schema::setAssemblyRules,
              (bp::arg("rules")));
        s.def("setAccessMode", &Schema::setAccessMode, (bp::arg("path"), bp::arg("value")));
        s.def("setDisplayedName", &Schema::setDisplayedName, (bp::arg("path"), bp::arg("value")));
        s.def("setDescription", &Schema::setDescription, (bp::arg("path"), bp::arg("value")));
        s.def("setTags", &Schema::setTags, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = ",;"));
        s.def("setDisplayType", &Schema::setDisplayType, (bp::arg("path"), bp::arg("value")));
        s.def("setAssignment", &Schema::setAssignment, (bp::arg("path"), bp::arg("value")));
        s.def("setSkipValidation", &Schema::setSkipValidation, (bp::arg("path"), bp::arg("value")));
        s.def("setOptions", &Schema::setOptions, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = ",;"));
        s.def("setAllowedStates", &schemawrap::setAllowedStates, (bp::arg("path"), bp::arg("value")));
        s.def("setDefaultValue", &schemawrap::setDefaultValue, (bp::arg("path"), bp::arg("value")));
        s.def("setAlias", &schemawrap::setAlias, (bp::arg("path"), bp::arg("value"))); // setAlias<type>
        s.def("setUnit", &Schema::setUnit, (bp::arg("path"), bp::arg("value")));
        s.def("setMetricPrefix", &Schema::setMetricPrefix, (bp::arg("path"), bp::arg("value")));
        s.def("setMinInc", &schemawrap::setMinInc, (bp::arg("path"), bp::arg("value")));
        s.def("setMaxInc", &schemawrap::setMaxInc, (bp::arg("path"), bp::arg("value")));
        s.def("setMinExc", &schemawrap::setMinExc, (bp::arg("path"), bp::arg("value")));
        s.def("setMaxExc", &schemawrap::setMaxExc, (bp::arg("path"), bp::arg("value")));
        s.def("setMinSize", &Schema::setMinSize, (bp::arg("path"), bp::arg("value")));
        s.def("setMaxSize", &Schema::setMaxSize, (bp::arg("path"), bp::arg("value")));
        s.def("setWarnLow", &schemawrap::setWarnLow, (bp::arg("path"), bp::arg("value")));
        s.def("setWarnHigh", &schemawrap::setWarnHigh, (bp::arg("path"), bp::arg("value")));
        s.def("setAlarmLow", &schemawrap::setAlarmLow, (bp::arg("path"), bp::arg("value")));
        s.def("setAlarmHigh", &schemawrap::setAlarmHigh, (bp::arg("path"), bp::arg("value")));
        s.def("setWarnVarianceLow", &Schema::setWarnVarianceLow, (bp::arg("path"), bp::arg("value")));
        s.def("setWarnVarianceHigh", &Schema::setWarnVarianceHigh, (bp::arg("path"), bp::arg("value")));
        s.def("setAlarmVarianceLow", &Schema::setAlarmVarianceLow, (bp::arg("path"), bp::arg("value")));
        s.def("setAlarmVarianceHigh", &Schema::setAlarmVarianceHigh, (bp::arg("path"), bp::arg("value")));
        s.def("getRollingStatsEvalInterval", &Schema::getRollingStatsEvalInterval, (bp::arg("path"), bp::arg("value")));
        s.def("getInfoForAlarm", &schemawrap::getInfoForAlarm, (bp::arg("path"), bp::arg("condition")));
        s.def("doesAlarmNeedAcknowledging", &schemawrap::doesAlarmNeedAcknowledging,
              (bp::arg("path"), bp::arg("condition")));
        s.def("setArchivePolicy", &Schema::setArchivePolicy, (bp::arg("path"), bp::arg("value")));
        s.def("setMin", &Schema::setMin, (bp::arg("path"), bp::arg("value")));
        s.def("setMax", &Schema::setMax, (bp::arg("path"), bp::arg("value")));
        s.def("setRequiredAccessLevel", &Schema::setRequiredAccessLevel, (bp::arg("path"), bp::arg("value")));
        // s.def("", &Schema::, ());     // overwrite<>(default) not implemented
        s.def("updateAliasMap", &schemawrap::updateAliasMap);
        s.def("hasRollingStatistics", &Schema::hasRollingStatistics);
        s.def("subSchema", &Schema::subSchema, (bp::arg("subNodePath"), bp::arg("filterTags") = ""));
        s.def("subSchemaByRules", &Schema::subSchemaByRules, (bp::arg("assemblyRules")));
        s.def("setDaqDataType", &Schema::setDaqDataType, (bp::arg("path"), bp::arg("dataType")));
        s.def("getDaqDataType", &Schema::getDaqDataType, (bp::arg("path")));
        s.def("hasDaqDataType", &Schema::hasDaqDataType, (bp::arg("path")));
        s.def("setDAQPolicy", &Schema::setDAQPolicy, (bp::arg("path"), bp::arg("policy")));
        s.def("getDAQPolicy", &Schema::getDAQPolicy, (bp::arg("path")));
        s.def("hasDAQPolicy", &Schema::hasDAQPolicy, (bp::arg("path")));
        s.def("setDefaultDAQPolicy", &Schema::setDefaultDAQPolicy, (bp::arg("policy")));
        s.def("isCustomNode", &Schema::isCustomNode, (bp::arg("path")));
        s.def("getCustomNodeClass", &Schema::getCustomNodeClass, (bp::arg("path")),
              bp::return_value_policy<bp::copy_const_reference>());
        s.def("hasAllowedActions", &Schema::hasAllowedActions, (bp::arg("path")),
              "Check if element given by argument has allowed actions.");
        s.def("getAllowedActions", &schemawrap::getAllowedActions, (bp::arg("path")),
              "Return allowed actions of element given by argument.");
        s.def("setAllowedActions", &schemawrap::setAllowedActions, (bp::arg("path"), bp::arg("actions")),
              "Specify one or more actions that are allowed on the element.\n"
              "If a Karabo device specifies allowed actions, that means that it offers\n"
              "a specific slot interface to operate on this element.\n"
              "Which allowed actions require which interface is defined elsewhere.");
    } // end Schema

    /////////////////////////////////////////////////////////////
    // DefaultValue<SimpleElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    // and DefaultValue<PathElement, std::string >
    // and DefaultValue<ByteArrayElement, ByteArray >


    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<int>, int, INT32)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<unsigned int>, unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<long long>, long long, INT64)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<unsigned long long>, unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<float>, float, FLOAT)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<double>, double, DOUBLE)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<string>, string, STRING)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<bool>, bool, BOOL)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(PathElement, string, PATH)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(ByteArrayElement, ByteArray, BYTEARRAY)

    ///////////////////////////////////////////////////////////////
    // AlarmSpecific<SimpleElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL

    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<int>, int, ReadOnlySpecific, INT32)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<unsigned int>, unsigned int, ReadOnlySpecific, UINT32)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<long long>, long long, ReadOnlySpecific, INT64)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, ReadOnlySpecific, UINT64)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<float>, float, ReadOnlySpecific, FLOAT)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<double>, double, ReadOnlySpecific, DOUBLE)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<std::string>, std::string, ReadOnlySpecific, STRING)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<bool>, bool, ReadOnlySpecific, BOOL)

    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<int>, int, RollingStatsSpecific, INT32)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<unsigned int>, unsigned int, RollingStatsSpecific, UINT32)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<long long>, long long, RollingStatsSpecific, INT64)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, RollingStatsSpecific,
                                        UINT64)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<float>, float, RollingStatsSpecific, FLOAT)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<double>, double, RollingStatsSpecific, DOUBLE)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<std::string>, std::string, RollingStatsSpecific, STRING)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<bool>, bool, RollingStatsSpecific, BOOL)

    ///////////////////////////////////////////////////////////////
    // RollingStatSpecific<SimpleElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL

    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<int>, int, INT32)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<unsigned int>, unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<long long>, long long, INT64)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<float>, float, FLOAT)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<double>, double, DOUBLE)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<bool>, bool, BOOL)

    ///////////////////////////////////////////////////////////////
    // ReadOnlySpecific<SimpleElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    //  and ReadOnlySpecific<PathElement, std::string >
    //  and ReadOnlySpecific<ByteArrayElement, ByteArray >


    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<int>, int, INT32)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<unsigned int>, unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<long long>, long long, INT64)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<float>, float, FLOAT)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<double>, double, DOUBLE)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<std::string>, std::string, STRING)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<bool>, bool, BOOL)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(PathElement, std::string, PATH)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(ByteArrayElement, ByteArray, BYTEARRAY)


    //    /////////////////////////////////////////////////////////////
    //    //DefaultValue<BitsetElement< EType >, EType >, where EType:
    //    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    //    //and DefaultValue<PathElement, std::string >
    //
    //    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(BitsetElement<unsigned char>,      unsigned char,      BITSET8)
    //    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(BitsetElement<unsigned short>,     unsigned short,     BITSET16)
    //    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(BitsetElement<unsigned int>,       unsigned int,       BITSET32)
    //    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(BitsetElement<unsigned long long>, unsigned long long, BITSET64)
    //
    //    ///////////////////////////////////////////////////////////////
    //    //ReadOnlySpecific<SimpleElement< EType >, EType >, where EType:
    //    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    //    // and ReadOnlySpecific<PathElement, std::string >
    //
    //    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(BitsetElement<unsigned char>,   unsigned char,      BITSET8)
    //    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(BitsetElement<unsigned short>,  unsigned short,     BITSET16)
    //    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(BitsetElement<unsigned int>,    unsigned int,       BITSET32)
    //    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(BitsetElement<unsigned long long>, unsigned long long, BITSET64)

    ///////////////////////////////////////////////////////////
    // DefaultValue<VectorElement< EType, 1, std::vector >, std::vector< EType > > where EType:
    // BOOL, INT32, UINT32, INT64, UINT64, DOUBLE, STRING

    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(int, INT32)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(long long, INT64)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(float, FLOAT)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(double, DOUBLE)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(std::string, STRING)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(bool, BOOL)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(char, CHAR)

    ///////////////////////////////////////////////////////////////
    // ReadOnlySpecific<VectorElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL

    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(int, INT32)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(long long, INT64)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(float, FLOAT)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(double, DOUBLE)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(std::string, STRING)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(bool, BOOL)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(char, CHAR)


    //////////////////////////////////////////////////////////////////////
    // Binding SimpleElement< EType >, where EType:
    // int, long long, double, string, bool
    // In Python: INT32_ELEMENT, UINT32_ELEMENT, INT64_ELEMENT, UINT64_ELEMENT, DOUBLE_ELEMENT,
    // STRING_ELEMENT, BOOL_ELEMENT

    KARABO_PYTHON_SIMPLE(int, INT32)
    KARABO_PYTHON_SIMPLE(unsigned int, UINT32)
    KARABO_PYTHON_SIMPLE(long long, INT64)
    KARABO_PYTHON_SIMPLE(unsigned long long, UINT64)
    KARABO_PYTHON_SIMPLE(float, FLOAT)
    KARABO_PYTHON_SIMPLE(double, DOUBLE)
    KARABO_PYTHON_SIMPLE(string, STRING)
    KARABO_PYTHON_SIMPLE(bool, BOOL)

    //////////////////////////////////////////////////////////////////////
    // Binding ByteArrayElement
    // In Python : BYTE_ARRAY
    {
        bp::implicitly_convertible<Schema&, ByteArrayElement>();
        bp::class_<ByteArrayElement>("BYTEARRAY_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              KARABO_PYTHON_COMMON_ATTRIBUTES(ByteArrayElement);
    }

    //////////////////////////////////////////////////////////////////////
    // Binding PathElement
    // In Python : PATH_ELEMENT
    {
        bp::implicitly_convertible<Schema&, PathElement>();
        typedef std::string EType;
        bp::class_<PathElement>("PATH_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              KARABO_PYTHON_COMMON_ATTRIBUTES(PathElement) KARABO_PYTHON_OPTIONS_NONVECTOR(PathElement)
                    .def("isInputFile", &PathElement::isInputFile, bp::return_internal_reference<>())
                    .def("isOutputFile", &PathElement::isOutputFile, bp::return_internal_reference<>())
                    .def("isDirectory", &PathElement::isDirectory, bp::return_internal_reference<>());
    }

    //////////////////////////////////////////////////////////////////////
    // Binding VectorElement< EType, std::vector >
    // In Python : VECTOR_INT32_ELEMENT, VECTOR_UINT32_ELEMENT,
    // VECTOR_INT64_ELEMENT, VECTOR_UINT64_ELEMENT, VECTOR_DOUBLE_ELEMENT,
    // VECTOR_STRING_ELEMENT, VECTOR_BOOL_ELEMENT, VECTOR_CHAR_ELEMENT


    KARABO_PYTHON_VECTOR(int, INT32)
    KARABO_PYTHON_VECTOR(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR(long long, INT64)
    KARABO_PYTHON_VECTOR(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR(float, FLOAT)
    KARABO_PYTHON_VECTOR(double, DOUBLE)
    KARABO_PYTHON_VECTOR(string, STRING)
    KARABO_PYTHON_VECTOR(bool, BOOL)
    KARABO_PYTHON_VECTOR(char, CHAR)

    //////////////////////////////////////////////////////////////////////
    // Binding NDArrayElement
    // In Python : NDARRAY_ELEMENT
    {
        bp::implicitly_convertible<Schema&, NDArrayElement>();
        bp::class_<NDArrayElement>("NDARRAY_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              .def("dtype", &NDArrayElementWrap::dtype, bp::arg("type"), bp::return_internal_reference<>())
              .def("shape", &NDArrayElementWrap::shape, bp::arg("shape"), bp::return_internal_reference<>())
              .def("unit", &NDArrayElement::unit, bp::return_internal_reference<>())
              .def("metricPrefix", &NDArrayElement::metricPrefix, bp::return_internal_reference<>())
              .def("observerAccess", &NDArrayElement::observerAccess, bp::return_internal_reference<>())
              .def("userAccess", &NDArrayElement::userAccess, bp::return_internal_reference<>())
              .def("operatorAccess", &NDArrayElement::operatorAccess, bp::return_internal_reference<>())
              .def("expertAccess", &NDArrayElement::expertAccess, bp::return_internal_reference<>())
              .def("adminAccess", &NDArrayElement::adminAccess, bp::return_internal_reference<>())
              .def("description", &NDArrayElement::description, bp::return_internal_reference<>())
              .def("displayedName", &NDArrayElement::displayedName, bp::return_internal_reference<>())
              .def("init", &NDArrayElement::init, bp::return_internal_reference<>())
              .def("key", &NDArrayElement::key, bp::return_internal_reference<>())
              .def("readOnly", &NDArrayElement::readOnly, bp::return_internal_reference<>())
              .def("reconfigurable", &NDArrayElement::reconfigurable, bp::return_internal_reference<>())
              .def("setAllowedActions", &ndarrayelementwrap::setAllowedActions, bp::arg("actions"),
                   "Specify one or more actions that are allowed on this node.\n"
                   "If a Karabo device specifies allowed actions for a node,\n"
                   "that means that it offers a specific slot interface to operate\n"
                   "on this node. Which allowed actions require which interface\n"
                   "is defined elsewhere",
                   bp::return_internal_reference<>())
              .def("skipValidation", &NDArrayElement::skipValidation, bp::return_internal_reference<>())
              .def("commit", &NDArrayElement::commit, bp::return_internal_reference<>())
              .def("commit", (NDArrayElement & (NDArrayElement::*)(karabo::util::Schema&))(&NDArrayElement::commit),
                   bp::arg("expected"), bp::return_internal_reference<>());
    }

    //////////////////////////////////////////////////////////////////////
    // Binding NodeElement
    // In Python : NODE_ELEMENT
    {
        bp::implicitly_convertible<Schema&, NodeElement>();
        bp::class_<NodeElement>("NODE_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              KARABO_PYTHON_NODE_CHOICE_LIST(NodeElement)
                    .def("appendParametersOf", &NodeElementWrap::appendParametersOf, (bp::arg("python_class")),
                         bp::return_internal_reference<>())
                    .def("appendParametersOfConfigurableClass", &NodeElementWrap::appendParametersOfConfigurableClass,
                         (bp::arg("python_base_class"), bp::arg("classid")), bp::return_internal_reference<>())
                    .def("appendSchema", &NodeElementWrap::appendSchema, (bp::arg("schema")),
                         bp::return_internal_reference<>())
                    .def("setDaqDataType", &NodeElementWrap::setDaqDataType, (bp::arg("dataType")),
                         bp::return_internal_reference<>())
                    .def("setSpecialDisplayType", &NodeElementWrap::setSpecialDisplayType, (bp::arg("displayType")),
                         bp::return_internal_reference<>())
                    .def("setAllowedActions", &NodeElementWrap::setAllowedActions, (bp::arg("actions")),
                         bp::return_internal_reference<>(),
                         "Specify one or more actions that are allowed on this node.\n"
                         "If a Karabo device specifies allowed actions for a node,\n"
                         "that means that it offers a specific slot interface to operate\n"
                         "on this node. Which allowed actions require which interface\n"
                         "is defined elsewhere.");
    }

    //////////////////////////////////////////////////////////////////////
    // Binding ListElement
    // In Python : LIST_ELEMENT
    {
        bp::implicitly_convertible<Schema&, ListElement>();
        bp::class_<ListElement>("LIST_ELEMENT",
                                bp::init<Schema&>((bp::arg("expected")))) KARABO_PYTHON_NODE_CHOICE_LIST(ListElement)
              .def("assignmentMandatory", &ListElement::assignmentMandatory, bp::return_internal_reference<>())
              .def("assignmentOptional", &ListElement::assignmentOptional, bp::return_internal_reference<>())
              .def("min", &ListElement::min, bp::return_internal_reference<>())
              .def("max", &ListElement::max, bp::return_internal_reference<>())
              .def("appendNodesOfConfigurationBase", &ListElementWrap::appendNodesOfConfigurationBase,
                   (bp::arg("python_base_class")), bp::return_internal_reference<>())
              .def("appendAsNode", &ListElementWrap::appendAsNode, (bp::arg("python_class"), bp::arg("nodeName") = ""),
                   bp::return_internal_reference<>())
              .def("init", &ListElement::init, bp::return_internal_reference<>())
              .def("reconfigurable", &ListElement::reconfigurable, bp::return_internal_reference<>())
              .def("setSpecialDisplayType", &ListElement::setSpecialDisplayType, bp::return_internal_reference<>());
    }

    {
        typedef ReadOnlySpecific<TableElement, std::vector<Hash>> ReadOnlySpec;
        bp::class_<ReadOnlySpec, boost::noncopyable>("ReadOnlySpecificTABLE", bp::no_init)
              .def("initialValue", &ReadOnlySpecificTableWrap::initialValueTable, (bp::arg("self"), bp::arg("pyList")),
                   bp::return_internal_reference<>())
              .def("defaultValue", &ReadOnlySpecificTableWrap::initialValueTable, (bp::arg("self"), bp::arg("pyList")),
                   bp::return_internal_reference<>())
              .def("archivePolicy",
                   (ReadOnlySpec &
                    (ReadOnlySpec::*)(karabo::util::Schema::ArchivePolicy const&))(&ReadOnlySpec::archivePolicy),
                   bp::return_internal_reference<>())
              .def("commit", (void(ReadOnlySpec::*)())(&ReadOnlySpec::commit));
    }


    //////////////////////////////////////////////////////////////////////
    // Binding TableElement
    // In Python : TABLE_ELEMENT
    {
        bp::implicitly_convertible<Schema&, TableElement>();
        bp::class_<TableElement>("TABLE_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              .def("observerAccess", &TableElement::observerAccess, bp::return_internal_reference<>())
              .def("userAccess", &TableElement::userAccess, bp::return_internal_reference<>())
              .def("operatorAccess", &TableElement::operatorAccess, bp::return_internal_reference<>())
              .def("expertAccess", &TableElement::expertAccess, bp::return_internal_reference<>())
              .def("adminAccess", &TableElement::adminAccess, bp::return_internal_reference<>())
              .def("allowedStates", bp::raw_function(&tableElementWrap::allowedStatesPy, 2))
              .def("assignmentInternal", &TableElement::assignmentInternal, bp::return_internal_reference<>())
              .def("assignmentMandatory", &TableElement::assignmentMandatory, bp::return_internal_reference<>())
              .def("assignmentOptional", &TableElement::assignmentOptional, bp::return_internal_reference<>())
              .def("alias", &AliasAttributeWrap<TableElement>::aliasPy, bp::return_internal_reference<>())
              .def("commit", &TableElement::commit, bp::return_internal_reference<>())
              .def("commit", (TableElement & (TableElement::*)(karabo::util::Schema&))(&TableElement::commit),
                   bp::arg("expected"), bp::return_internal_reference<>())
              .def("description", &TableElement::description, bp::return_internal_reference<>())
              .def("displayedName", &TableElement::displayedName, bp::return_internal_reference<>())
              .def("init", &TableElement::init, bp::return_internal_reference<>())
              .def("key", &TableElement::key, bp::return_internal_reference<>())
              .def("reconfigurable", &TableElement::reconfigurable, bp::return_internal_reference<>())
              .def("tags",
                   (TableElement & (TableElement::*)(std::string const&, std::string const&))(&TableElement::tags),
                   (bp::arg("tags"), bp::arg("sep") = " ,;"), bp::return_internal_reference<>())
              .def("tags", (TableElement & (TableElement::*)(std::vector<std::string> const&))(&TableElement::tags),
                   (bp::arg("tags")), bp::return_internal_reference<>())
              .def("defaultValue", &DefaultValueTableWrap::defaultValue, (bp::arg("self"), bp::arg("pyList")),
                   bp::return_internal_reference<>())
              .def("noDefaultValue",
                   (TableElement & (karabo::util::TableDefaultValue<TableElement>::*)())(
                         &karabo::util::TableDefaultValue<TableElement>::noDefaultValue),
                   bp::return_internal_reference<>())
              .def("maxSize", (TableElement & (TableElement::*)(int const&))(&TableElement::maxSize),
                   bp::return_internal_reference<>())
              .def("minSize", (TableElement & (TableElement::*)(int const&))(&TableElement::minSize),
                   bp::return_internal_reference<>())
              .def("setNodeSchema", &TableElement::setColumns, (bp::arg("nodeSchema")),
                   bp::return_internal_reference<>(), "DEPRECATED - use 'setColumns' instead")
              .def("setColumns", &TableElement::setColumns, (bp::arg("schema")), bp::return_internal_reference<>(),
                   "Set Schema describing the columns")
              .def("readOnly", &TableElement::readOnly, bp::return_internal_reference<>())
              /*.def("addRow"
                   , (TableElement & (TableElement::*)(const Hash&))&TableElement::addRow, (bp::arg("nodeHash") =
              Hash()) , bp::return_internal_reference<> ()) .def("addRow" , (TableElement & (TableElement::*)(const
              Schema&, const Hash&))&TableElement::addRow, (bp::arg("nodeSchema"), bp::arg("nodeHash") = Hash()) ,
              bp::return_internal_reference<> ())*/
              ;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  TableDefaultValue<TableElement>
    {
        typedef TableDefaultValue<TableElement> DefTableElement;
        bp::class_<DefTableElement, boost::noncopyable>("DefaultValueTableElement", bp::no_init)
              .def("defaultValue", &DefaultValueTableWrap::defaultValue, (bp::arg("self"), bp::arg("pyList")),
                   bp::return_internal_reference<>())
              .def("noDefaultValue", (TableElement & (DefTableElement::*)())(&DefTableElement::noDefaultValue),
                   bp::return_internal_reference<>());
    }

    //////////////////////////////////////////////////////////////////////
    // Binding ChoiceElement
    // In Python : CHOICE_ELEMENT
    {
        bp::implicitly_convertible<Schema&, ChoiceElement>();
        bp::class_<ChoiceElement>("CHOICE_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              KARABO_PYTHON_NODE_CHOICE_LIST(ChoiceElement)
                    .def("assignmentMandatory", &ChoiceElement::assignmentMandatory, bp::return_internal_reference<>())
                    .def("assignmentOptional", &ChoiceElement::assignmentOptional, bp::return_internal_reference<>())
                    .def("appendNodesOfConfigurationBase", &ChoiceElementWrap::appendNodesOfConfigurationBase,
                         (bp::arg("python_base_class")), bp::return_internal_reference<>())
                    .def("appendAsNode", &ChoiceElementWrap::appendAsNode,
                         (bp::arg("python_class"), bp::arg("nodeName") = ""), bp::return_internal_reference<>())
                    .def("reconfigurable", &ChoiceElement::reconfigurable, bp::return_internal_reference<>())
                    .def("init", &ChoiceElement::init, bp::return_internal_reference<>());
    }

    //////////////////////////////////////////////////////////////////////
    // Binding InputElement
    // In Python : INPUT_ELEMENT
    {
        bp::implicitly_convertible<Schema&, InputElement>();
        bp::class_<InputElement>("INPUT_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              KARABO_PYTHON_NODE_CHOICE_LIST(InputElement)
                    .def("setInputType", &InputElementWrap::setInputType, (bp::arg("python_base_class")),
                         bp::return_internal_reference<>());
    }

    //////////////////////////////////////////////////////////////////////
    // Binding OutputElement
    // In Python : OUTPUT_ELEMENT
    {
        bp::implicitly_convertible<Schema&, OutputElement>();
        bp::class_<OutputElement>("OUTPUT_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              KARABO_PYTHON_NODE_CHOICE_LIST(OutputElement)
                    .def("setOutputType", &OutputElementWrap::setOutputType, (bp::arg("python_base_class")),
                         bp::return_internal_reference<>());
    }

    ///////////////////////////////////////////////////////////////////////////
    //  DefaultValue<ChoiceElement>
    {
        typedef DefaultValue<ChoiceElement, string> DefChoiceElement;
        bp::class_<DefChoiceElement, boost::noncopyable>("DefaultValueChoiceElement", bp::no_init)
              .def("defaultValue",
                   (ChoiceElement & (DefChoiceElement::*)(string const&))(&DefChoiceElement::defaultValue),
                   (bp::arg("defValue")), bp::return_internal_reference<>())
              .def("defaultValueFromString",
                   (ChoiceElement & (DefChoiceElement::*)(string const&))(&DefChoiceElement::defaultValueFromString),
                   (bp::arg("defValue")), bp::return_internal_reference<>())
              .def("noDefaultValue", (ChoiceElement & (DefChoiceElement::*)())(&DefChoiceElement::noDefaultValue),
                   bp::return_internal_reference<>());
    }

    ///////////////////////////////////////////////////////////////////////////
    //  DefaultValue<ListElement>
    {
        typedef DefaultValue<ListElement, vector<string>> DefListElement;
        bp::class_<DefListElement, boost::noncopyable>("DefaultValueListElement", bp::no_init)
              .def("defaultValue", &ListElementWrap::defaultValueList, (bp::arg("self"), bp::arg("pyList")),
                   bp::return_internal_reference<>())
              .def("defaultValueFromString",
                   (ListElement & (DefListElement::*)(string const&))(&DefListElement::defaultValueFromString),
                   (bp::arg("defValue")), bp::return_internal_reference<>())
              .def("noDefaultValue", (ListElement & (DefListElement::*)())(&DefListElement::noDefaultValue),
                   bp::return_internal_reference<>());
    }

    {
        bp::class_<Validator::ValidationRules>("ValidatorValidationRules", bp::init<>())
              .def_readwrite("injectDefaults", &Validator::ValidationRules::injectDefaults)
              .def_readwrite("allowUnrootedConfiguration", &Validator::ValidationRules::allowUnrootedConfiguration)
              .def_readwrite("allowAdditionalKeys", &Validator::ValidationRules::allowAdditionalKeys)
              .def_readwrite("allowMissingKeys", &Validator::ValidationRules::allowMissingKeys)
              .def_readwrite("injectTimestamps", &Validator::ValidationRules::injectTimestamps)
              .def_readwrite("forceInjectedTimestamp", &Validator::ValidationRules::forceInjectedTimestamp);

        bp::class_<Validator>("Validator", bp::init<>())
              .def(bp::init<const Validator::ValidationRules>())
              .def("validate", &ValidatorWrap::validate,
                   (bp::arg("schema"), bp::arg("configuration"), bp::arg("timestamp") = bp::object()))
              .def("setValidationRules", &ValidatorWrap::setValidationRules, (bp::arg("rules")))
              .def("getValidationRules", &ValidatorWrap::getValidationRules)
              .def("hasParametersInWarnOrAlarm", &ValidatorWrap::hasParametersInWarnOrAlarm)
              .def("getParametersInWarnOrAlarm", &ValidatorWrap::getParametersInWarnOrAlarm)
              .def("hasReconfigurableParameter", &ValidatorWrap::hasReconfigurableParameter)
              .def("getRollingStatistics", &ValidatorWrap::getRollingStatistics, bp::arg("key"),
                   bp::return_internal_reference<>())
              .def_readonly("kAlarmParamPathSeparator", &Validator::kAlarmParamPathSeparator);
    }

    {
        bp::implicitly_convertible<Schema&, OverwriteElement>();
        bp::class_<OverwriteElement>("OVERWRITE_ELEMENT", bp::init<Schema&>((bp::arg("expected"))))
              .def("key", (OverwriteElement & (OverwriteElement::*)(string const&))(&OverwriteElement::key),
                   (bp::arg("key")), bp::return_internal_reference<>())
              .def("setNewDisplayedName",
                   (OverwriteElement & (OverwriteElement::*)(string const&))(&OverwriteElement::setNewDisplayedName),
                   (bp::arg("name")), bp::return_internal_reference<>())
              .def("setNewDescription",
                   (OverwriteElement & (OverwriteElement::*)(string const&))(&OverwriteElement::setNewDescription),
                   (bp::arg("description")), bp::return_internal_reference<>())
              .def("setNewAlias", &OverwriteElementWrap().setNewAlias, (bp::arg("alias")),
                   bp::return_internal_reference<>())
              .def("setNewTags", &OverwriteElementWrap().setNewTags, (bp::arg("tags")),
                   bp::return_internal_reference<>(), "Overwrite tags, 'tags' can be a str or an iterable of str.")
              .def("setNewAssignmentMandatory",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNewAssignmentMandatory),
                   bp::return_internal_reference<>())
              .def("setNewAssignmentOptional",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNewAssignmentOptional),
                   bp::return_internal_reference<>())
              .def("setNewAssignmentInternal",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNewAssignmentInternal),
                   bp::return_internal_reference<>())
              .def("setNowInit", (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowInit),
                   bp::return_internal_reference<>())
              .def("setNowReconfigurable",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowReconfigurable),
                   bp::return_internal_reference<>())
              .def("setNowReadOnly", (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowReadOnly),
                   bp::return_internal_reference<>())
              .def("setNowValidate", (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowValidate),
                   bp::return_internal_reference<>())
              .def("setNowSkipValidation",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowSkipValidation),
                   bp::return_internal_reference<>())
              .def("setNewDefaultValue", &OverwriteElementWrap().setNewDefaultValue, (bp::arg("value")),
                   bp::return_internal_reference<>())
              .def("setNewMinInc", &OverwriteElementWrap().setNewMinInc, (bp::arg("value")),
                   bp::return_internal_reference<>())
              .def("setNewMaxInc", &OverwriteElementWrap().setNewMaxInc, (bp::arg("value")),
                   bp::return_internal_reference<>())
              .def("setNewMinExc", &OverwriteElementWrap().setNewMinExc, (bp::arg("value")),
                   bp::return_internal_reference<>())
              .def("setNewMaxExc", &OverwriteElementWrap().setNewMaxExc, (bp::arg("value")),
                   bp::return_internal_reference<>())
              .def("setNewMinSize", &OverwriteElementWrap().setNewMinSize, (bp::arg("value")),
                   bp::return_internal_reference<>())
              .def("setNewMaxSize", &OverwriteElementWrap().setNewMaxSize, (bp::arg("value")),
                   bp::return_internal_reference<>())
              .def("setNewOptions", bp::raw_function(&OverwriteElementWrap::setNewOptions, 2))
              .def("setNewAllowedStates", bp::raw_function(&OverwriteElementWrap::setNewAllowedStates, 2))
              .def("setNowObserverAccess",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowObserverAccess),
                   bp::return_internal_reference<>())
              .def("setNowUserAccess",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowUserAccess),
                   bp::return_internal_reference<>())
              .def("setNowOperatorAccess",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowOperatorAccess),
                   bp::return_internal_reference<>())
              .def("setNowExpertAccess",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowExpertAccess),
                   bp::return_internal_reference<>())
              .def("setNowAdminAccess",
                   (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowAdminAccess),
                   bp::return_internal_reference<>())
              .def("setNewUnit",
                   (OverwriteElement & (OverwriteElement::*)(const UnitType&))(&OverwriteElement::setNewUnit),
                   (bp::arg("value")), bp::return_internal_reference<>())
              .def("setNewMetricPrefix",
                   (OverwriteElement &
                    (OverwriteElement::*)(const MetricPrefixType&))(&OverwriteElement::setNewMetricPrefix),
                   (bp::arg("value")), bp::return_internal_reference<>())
              .def("commit", (void(OverwriteElement::*)())(&OverwriteElement::commit));
    }

    {
        bp::class_<HashFilterWrap, boost::noncopyable>("HashFilter", bp::no_init)
              .def("byTag", HashFilterWrap::byTag,
                   (bp::arg("schema"), bp::arg("config"), bp::arg("tags"), bp::arg("sep") = ","))
              .staticmethod("byTag")
              .def("byAccessMode", HashFilterWrap::byAccessMode,
                   (bp::arg("schema"), bp::arg("config"), bp::arg("accessMode")))
              .staticmethod("byAccessMode");
    }
} // end  exportPyUtilSchema
