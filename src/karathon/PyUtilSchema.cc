/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <iostream>
#include <karabo/util/Factory.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/ListElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/OverwriteElement.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/ImageElement.hh>
#include <karabo/io/InputElement.hh>
#include <karabo/io/OutputElement.hh>
#include <karabo/util/Validator.hh>
#include <karabo/util/HashFilter.hh>

#include "PythonMacros.hh"
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace karabo::io;
using namespace std;


struct SchemaWrapper : Schema, bp::wrapper< Schema > {

    SchemaWrapper(Schema const & arg) : Schema(arg), bp::wrapper< Schema >() {
    }

    SchemaWrapper() : Schema(), bp::wrapper< Schema >() {
    }

    SchemaWrapper(std::string const & classId, Schema::AssemblyRules const & rules)
    : Schema(classId, boost::ref(rules)), bp::wrapper< Schema >() {
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


class ValidatorWrap {

public:

    static bp::object validate(Validator& self, const Schema& schema, const Hash& configuration, const Timestamp& stamp) {
        Hash::Pointer validated = Hash::Pointer(new Hash);
        pair<bool, string> result = self.validate(schema, configuration, *validated, stamp);
        if (result.first)
            return bp::object(validated);
        throw KARABO_PYTHON_EXCEPTION(result.second);
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
};


struct NodeElementWrap {

    static NodeElement & appendParametersOfConfigurableClass(NodeElement& self, const bp::object& baseobj, const std::string& classid) {

        if (!PyType_Check(baseobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'arg1' given in 'appendParametersOfConfigurableClass(arg1, arg2)' of NODE_ELEMENT must be a class in Python registered as base class in Configurator");
        }
        if (!PyObject_HasAttrString(baseobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class with classid = '" + classid + "' given in 'appendParametersOfConfigurableClass(base, classid)' of NODE_ELELEMT has no 'getSchema' method.");
        }
        std::string baseClassId;
        if (PyObject_HasAttrString(baseobj.ptr(), "__karabo_cpp_classid__")) {
            // baseobj is object of C++ base class
            baseClassId = bp::extract<std::string>(baseobj.attr("__karabo_cpp_classid__"));
        } else {
            baseClassId = bp::extract<std::string>(baseobj.attr("__classid__"));
        }
        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
        self.getNode().setAttribute(KARABO_SCHEMA_CLASS_ID, classid);
        self.getNode().setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, baseClassId);

        bp::object schemaObj = baseobj.attr("getSchema")(classid);

        const Schema schema = bp::extract<Schema> (schemaObj);
        const Hash h = schema.getParameterHash();
        self.getNode().setValue<Hash>(h);

        return self;
    }

    static NodeElement & appendParametersOf(NodeElement& self, const bp::object& obj) {

        if (!PyType_Check(obj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'arg' given in 'appendParametersOf(arg)' of NODE_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(obj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class given in 'appendParametersOf' of NODE_ELELEMT has no 'getSchema' method");
        }
        std::string classid;
        if (PyObject_HasAttrString(obj.ptr(), "__karabo_cpp_classid__")) {
            classid = bp::extract<std::string>(obj.attr("__karabo_cpp_classid__"));
        } else {
            classid = bp::extract<std::string>(obj.attr("__classid__"));
        }
        bp::object schemaObj = obj.attr("getSchema")(classid);
        const Schema schemaPy = bp::extract<Schema> (schemaObj);

        const Hash h = schemaPy.getParameterHash();
        self.getNode().setValue<Hash>(h);

        return self;
    }

};


struct ChoiceElementWrap {

    static ChoiceElement & appendNodesOfConfigurationBase(ChoiceElement& self, const bp::object& classobj) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'arg' given in 'appendNodesOfConfigurationBase(arg)' of CHOICE_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class given in 'appendNodesOfConfigurationBase' of CHOICE_ELEMENT must have 'getSchema' function");
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

        if (any.type() != typeid (std::vector<std::string>))
            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
        const vector<string>& nodeNames = boost::any_cast<vector<string> >(any);
        for (size_t i = 0; i < nodeNames.size(); i++) {
            std::string nodeName = nodeNames[i];
            bp::object schemaObj = classobj.attr("getSchema")(nodeName);
            const Schema& schema = bp::extract<const Schema&>(schemaObj);
            Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
            node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
            node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
            node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
            node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        }
        return self;
    }

    static ChoiceElement & appendAsNode(ChoiceElement& self, const bp::object& classobj, const std::string& nodeNameObj) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'classobj' given in 'appendAsNode(classobj, nodeName)' of CHOICE_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class given in 'appendAsNode(classobj, nodeName)' of CHOICE_ELEMENT has no 'getSchema' method");
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
        bp::object schemaObj = classobj.attr("getSchema")(nodeName);
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

    static ListElement & appendNodesOfConfigurationBase(ListElement& self, const bp::object& classobj) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'arg' given in 'appendNodesOfConfigurationBase(arg)' of LIST_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class given in 'appendNodesOfConfigurationBase' of LIST_ELEMENT has no 'getSchema' method");
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

        if (any.type() != typeid (vector<string>))
            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
        const vector<string>& nodeNames = boost::any_cast<vector<string> >(any);
        for (size_t i = 0; i < nodeNames.size(); i++) {
            std::string nodeName = nodeNames[i];
            bp::object schemaObj = classobj.attr("getSchema")(nodeName);
            const Schema& schema = bp::extract<const Schema&>(schemaObj);
            Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
            node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
            node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
            node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
            node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        }
        return self;
    }

    static ListElement & appendAsNode(ListElement& self, const bp::object& classobj, const std::string& name) {
        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'classobj' given in 'appendAsNode(classobj, nodeName)' of LIST_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class given in 'appendAsNode(classobj, nodeName)' of LIST_ELEMENT has no 'getSchema' method");
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
        bp::object schemaObj = classobj.attr("getSchema")(nodeName);
        const Schema& schema = bp::extract<const Schema&>(schemaObj);
        Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
        node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
        node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
        node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
        node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
        return self;
    }

    typedef DefaultValue<ListElement, vector<string> > DefListElement;

    static ListElement & defaultValueList(DefListElement& self, const bp::object& obj) {
        if (PyList_Check(obj.ptr())) {
            const bp::list& l = bp::extract<bp::list > (obj);
            bp::ssize_t size = bp::len(l);

            vector<string> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<string> (obj[i]);
            }
            return self.defaultValue(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the defaultValue of LIST_ELEMENT must be a list of strings");
        }
    }

};


struct InputElementWrap {

    static InputElement & setInputType(InputElement& self, const bp::object& classobj) {

        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'classobj' given in 'setInputType(classobj)' of INPUT_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class given in 'setInputType(classobj)' of INPUT_ELEMENT has no 'getSchema' method");
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

        if (any.type() != typeid (vector<string>))
            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
        const vector<string>& nodeNames = boost::any_cast<vector<string> >(any);
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

    static OutputElement & setOutputType(OutputElement& self, const bp::object& classobj) {

        if (!PyType_Check(classobj.ptr())) {
            throw KARABO_PYTHON_EXCEPTION("Argument 'classobj' given in 'setOutputType(classobj)' of OUTPUT_ELEMENT must be a class in Python");
        }
        if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
            throw KARABO_PYTHON_EXCEPTION("Class given in 'setOutputType(classobj)' of OUTPUT_ELEMENT has no 'getSchema' method");
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

        if (any.type() != typeid (vector<string>))
            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
        const vector<string>& nodeNames = boost::any_cast<vector<string> >(any);
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

    static OverwriteElement & setNewAlias(OverwriteElement& self, const bp::object& alias) {
        boost::any any;
        karathon::Wrapper::toAny(alias, any);
        return self.setNewAlias(any);
    }

    static OverwriteElement & setNewTag(OverwriteElement& self, const bp::object& tag) {
        boost::any any;
        karathon::Wrapper::toAny(tag, any);
        return self.setNewTag(any);
    }

    static OverwriteElement & setNewDefaultValue(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewDefaultValue(any);
    }

    static OverwriteElement & setNewMinInc(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewMinInc(any);
    }

    static OverwriteElement & setNewMaxInc(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewMaxInc(any);
    }

    static OverwriteElement & setNewMinExc(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewMinExc(any);
    }

    static OverwriteElement & setNewMaxExc(OverwriteElement& self, const bp::object& value) {
        boost::any any;
        karathon::Wrapper::toAny(value, any);
        return self.setNewMaxExc(any);
    }

};


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
                        v[i] = bp::extract<std::string > (aliasObj[i]);
                    }
                    self.setAlias(path, v);
                    return;
                }
                if (PyUnicode_Check(list0.ptr())) {
                    std::vector<std::string> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        bp::object str(bp::handle<>(PyUnicode_AsUTF8String(static_cast<bp::object>(aliasObj[i]).ptr())));
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
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument defining the key name in 'setAlias' should be a string");
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
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
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
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
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
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
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
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
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

    bp::object getMinIncAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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

    bp::object getMaxIncAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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

    bp::object getMinExcAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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

    bp::object getMaxExcAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
    }

    bp::object getWarnLow(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_WARN_LOW), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getWarnLow' must be a string");
    }

    void setWarnHigh(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setWarnHigh(path, any);
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
    }

    bp::object getWarnHigh(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_WARN_HIGH), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getWarnHigh' must be a string");
    }

    void setAlarmLow(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setAlarmLow(path, any);
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
    }

    bp::object getAlarmLow(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_ALARM_LOW), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAlarmLow' must be a string");
    }

    void setAlarmHigh(Schema& self, const bp::object& obj, const bp::object& value) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            boost::any any;
            karathon::Wrapper::toAny(value, any);
            self.setAlarmHigh(path, any);
        } else
            throw KARABO_PYTHON_EXCEPTION("Python argument in 'setMinInc' must be a string");
    }

    bp::object getAlarmHigh(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_ALARM_HIGH), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getAlarmHigh' must be a string");
    }


    //*************************************************************************************
    // Wrapper functions for : getWarnLowAs, getWarnHighAs, getAlarmLowAs, getAlarmHighAs *
    //*************************************************************************************

    bp::object getWarnLowAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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

    bp::object getWarnHighAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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

    bp::object getAlarmLowAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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

    bp::object getAlarmHighAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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
    // Wrapper functions for : getKeys, getPaths, getTags, getOptions, getAllowedStates *
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

    bp::object getOptions(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const vector<string>& v = schema.getOptions(path);
            return karathon::Wrapper::fromStdVectorToPyArray<string>(v);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument in 'getOptions' should be a string");
    }

    bp::object getAllowedStates(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const vector<string>& v = schema.getAllowedStates(path);
            return karathon::Wrapper::fromStdVectorToPyArray<string>(v);
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
            throw KARABO_PYTHON_EXCEPTION("Python argument defining the key name in 'setDefaultValue' should be a string");
    }

    bp::object getDefaultValue(const Schema& schema, const bp::object& obj) {
        if (PyUnicode_Check(obj.ptr())) {
            string path = bp::extract<string>(obj);
            const Hash& h = schema.getParameterHash();
            return karathon::Wrapper::toObject(h.getAttributeAsAny(path, KARABO_SCHEMA_DEFAULT_VALUE), false);
        }
        throw KARABO_PYTHON_EXCEPTION("Python argument defining the key name in 'getDefaultValue' should be a string");
    }

    bp::object getDefaultValueAs(const Schema& schema, const bp::object& obj, const karathon::PyTypes::ReferenceType& pytype) {
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
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<bool, vector > (path));
                case Types::VECTOR_INT32:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<int, vector >(path));
                case Types::VECTOR_UINT32:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<unsigned int, vector >(path));
                case Types::VECTOR_INT64:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<long long, vector >(path));
                case Types::VECTOR_UINT64:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<unsigned long long, vector >(path));
                case Types::VECTOR_STRING:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<string, vector >(path));
                case Types::VECTOR_FLOAT:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<float, vector >(path));
                case Types::VECTOR_DOUBLE:
                    return karathon::Wrapper::fromStdVectorToPyArray(schema.getDefaultValueAs<double, vector >(path));
                default:
                    break;
            }
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not supported");
        }
        throw KARABO_PYTHON_EXCEPTION("Python first argument in 'getDefaultValueAs' should be a string (defining a key name)");
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
                    v[i] = bp::extract<std::string > (obj[i]);
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
                    v[i] = bp::extract<std::string > (obj[i]);
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
}


struct HashFilterWrap {

    static boost::shared_ptr<Hash> byTag(const Schema& schema, const Hash& config, const std::string& tags, const std::string& sep = ",") {
        boost::shared_ptr<Hash> result(new Hash);
        HashFilter::byTag(schema, config, *result, tags, sep);
        return result;
    }
};

void exportPyUtilSchema() {

    bp::enum_< AccessType>("AccessType")
            .value("INIT", INIT)
            .value("READ", READ)
            .value("WRITE", WRITE)
            .export_values()
            ;

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
                .export_values()
                ;
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
                .export_values()
                ;
    }

    {//  Exposing std::vector<std::string>
        typedef bp::class_<std::vector<std::string> > VectorStringExposer;
        VectorStringExposer exposerOfVectorString = VectorStringExposer("VectorString");
        bp::scope scopeOfVectorString(exposerOfVectorString);
        exposerOfVectorString.def(bp::vector_indexing_suite<std::vector<std::string>, true>());
    }
    
    {//exposing ::Schema

        bp::class_< Schema > s("Schema");
        s.def(bp::init< >());
        s.def(bp::init<std::string const &, bp::optional<Schema::AssemblyRules const&> > ());

        bp::enum_< Schema::AssignmentType > ("AssignmentType")
                .value("OPTIONAL", Schema::OPTIONAL_PARAM)
                .value("MANDATORY", Schema::MANDATORY_PARAM)
                .value("INTERNAL", Schema::INTERNAL_PARAM)
                .export_values()
                ;
        bp::enum_< Schema::AccessLevel > ("AccessLevel")
                .value("OBSERVER", Schema::OBSERVER)
                .value("USER", Schema::USER)
                .value("OPERATOR", Schema::OPERATOR)
                .value("EXPERT", Schema::EXPERT)
                .value("ADMIN", Schema::ADMIN)
                .export_values()
                ;
        bp::enum_< Schema::LeafType>("LeafType")
                .value("PROPERTY", Schema::PROPERTY)
                .value("COMMAND", Schema::COMMAND)
                .export_values()
                ;
        bp::enum_< Schema::NodeType>("NodeType")
                .value("LEAF", Schema::LEAF)
                .value("NODE", Schema::NODE)
                .value("CHOICE_OF_NODES", Schema::CHOICE_OF_NODES)
                .value("LIST_OF_NODES", Schema::LIST_OF_NODES)
                .export_values()
                ;
        bp::enum_< Schema::ArchivePolicy > ("ArchivePolicy")
                .value("EVERY_EVENT", Schema::EVERY_EVENT)
                .value("EVERY_100MS", Schema::EVERY_100MS)
                .value("EVERY_1S", Schema::EVERY_1S)
                .value("EVERY_5S", Schema::EVERY_5S)
                .value("EVERY_10S", Schema::EVERY_10S)
                .value("EVERY_1MIN", Schema::EVERY_1MIN)
                .value("EVERY_10MIN", Schema::EVERY_10MIN)
                .value("NO_ARCHIVING", Schema::NO_ARCHIVING)
                .export_values()
                ;
        bp::class_< Schema::AssemblyRules >("AssemblyRules", bp::init< bp::optional< AccessType const &, std::string const &, const int > >((bp::arg("accessMode") = operator|(INIT, WRITE), bp::arg("state") = "", bp::arg("accessLevel") = -1)))
                .def_readwrite("m_accessMode", &Schema::AssemblyRules::m_accessMode)
                .def_readwrite("m_accessLevel", &Schema::AssemblyRules::m_accessLevel)
                .def_readwrite("m_state", &Schema::AssemblyRules::m_state);

        s.def(bp::self_ns::str(bp::self));


        //********* General functions on Schema *******

        s.def("has"
              , (bool (Schema::*)(string const &) const) (&Schema::has)
              , bp::arg("path"));

        s.def("__contains__"
              , (bool (Schema::*)(string const &) const) (&Schema::has)
              , bp::arg("path"));

        s.def("empty"
              , (bool (Schema::*)()const) (&Schema::empty));

        s.def("merge"
              , (void (Schema::*)(Schema const &))(&Schema::merge)
              , bp::arg("schema"));

        s.def("__iadd__", &schemawrap::merge, bp::arg("schema"));

        s.def("copy", &schemawrap::copy, bp::arg("schema"));

        //********* 'get'-methods *********************
        s.def("getRequiredAccessLevel", &Schema::getRequiredAccessLevel);

        s.def("getParameterHash", &schemawrap::getParameterHash);

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

        s.def("getUnit", (const int (Schema::*)(const std::string & path) const) &Schema::getUnit, (bp::arg("path")));

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

        s.def("getMin", &Schema::getMin
              , bp::return_value_policy< bp::copy_const_reference >());

        s.def("getMax", &Schema::getMax
              , bp::return_value_policy< bp::copy_const_reference >());

        s.def("getMinSize"
              , (const unsigned int& (Schema::*)(const string &) const) (&Schema::getMinSize)
              , bp::return_value_policy< bp::copy_const_reference >());

        s.def("getMaxSize"
              , (const unsigned int& (Schema::*)(const string &) const) (&Schema::getMaxSize)
              , bp::return_value_policy< bp::copy_const_reference >());

        s.def("getArchivePolicy", &Schema::getArchivePolicy
              , bp::return_value_policy< bp::copy_const_reference >());

        s.def("getWarnLow", &schemawrap::getWarnLow);

        s.def("getWarnHigh", &schemawrap::getWarnHigh);

        s.def("getAlarmLow", &schemawrap::getAlarmLow);

        s.def("getAlarmHigh", &schemawrap::getAlarmHigh);

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

        s.def("getClassInfo"
              , (ClassInfo(Schema::*)() const) (&Schema::getClassInfo)
              , (ClassInfo(SchemaWrapper::*)() const) (&SchemaWrapper::default_getClassInfo));

        s.def("classInfo"
              , (ClassInfo(*)())(&Schema::classInfo)).staticmethod("classInfo");

        s.def("setAssemblyRules", (void (Schema::*)(const Schema::AssemblyRules&)) & Schema::setAssemblyRules, (bp::arg("rules")));
        s.def("setAccessMode", &Schema::setAccessMode, (bp::arg("path"), bp::arg("value")));
        s.def("setDisplayedName", &Schema::setDisplayedName, (bp::arg("path"), bp::arg("value")));
        s.def("setDescription", &Schema::setDescription, (bp::arg("path"), bp::arg("value")));
        s.def("setTags", &Schema::setTags, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = ",;"));
        s.def("setDisplayType", &Schema::setDisplayType, (bp::arg("path"), bp::arg("value")));
        s.def("setAssignment", &Schema::setAssignment, (bp::arg("path"), bp::arg("value")));
        s.def("setOptions", &Schema::setOptions, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = ",;"));
        s.def("setAllowedStates", &Schema::setAllowedStates, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = ",;"));
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
        s.def("setArchivePolicy", &Schema::setArchivePolicy, (bp::arg("path"), bp::arg("value")));
        s.def("setMin", &Schema::setMin, (bp::arg("path"), bp::arg("value")));
        s.def("setMax", &Schema::setMax, (bp::arg("path"), bp::arg("value")));
        s.def("setRequiredAccessLevel", &Schema::setRequiredAccessLevel, (bp::arg("path"), bp::arg("value")));
        //s.def("", &Schema::, ());     // overwrite<>(default) not implemented
        s.def("updateAliasMap", &schemawrap::updateAliasMap);
    }// end Schema

    /////////////////////////////////////////////////////////////
    //DefaultValue<SimpleElement< EType >, EType >, where EType:
    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    //and DefaultValue<PathElement, std::string >

    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<int>, int, INT32)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<unsigned int>, unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<long long>, long long, INT64)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<unsigned long long>, unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<float>, float, FLOAT)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<double>, double, DOUBLE)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<string>, string, STRING)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<bool>, bool, BOOL)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(PathElement, string, PATH)

    ///////////////////////////////////////////////////////////////
    //ReadOnlySpecific<SimpleElement< EType >, EType >, where EType:
    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    // and ReadOnlySpecific<PathElement, std::string >

    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<int>, int, INT32)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<unsigned int>, unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<long long>, long long, INT64)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<float>, float, FLOAT)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<double>, double, DOUBLE)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<std::string>, std::string, STRING)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<bool>, bool, BOOL)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(PathElement, std::string, PATH)

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
    //DefaultValue<VectorElement< EType, std::vector >, std::vector< EType > > where EType:
    //BOOL, INT32, UINT32, INT64, UINT64, DOUBLE, STRING 

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
    //ReadOnlySpecific<VectorElement< EType >, EType >, where EType:
    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL

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
    //Binding SimpleElement< EType >, where EType:
    //int, long long, double, string, bool
    //In Python: INT32_ELEMENT, UINT32_ELEMENT, INT64_ELEMENT, UINT64_ELEMENT, DOUBLE_ELEMENT,
    //STRING_ELEMENT, BOOL_ELEMENT

    KARABO_PYTHON_SIMPLE(int, INT32)
    KARABO_PYTHON_SIMPLE(unsigned int, UINT32)
    KARABO_PYTHON_SIMPLE(long long, INT64)
    KARABO_PYTHON_SIMPLE(unsigned long long, UINT64)
    KARABO_PYTHON_SIMPLE(float, FLOAT)
    KARABO_PYTHON_SIMPLE(double, DOUBLE)
    KARABO_PYTHON_SIMPLE(string, STRING)
    KARABO_PYTHON_SIMPLE(bool, BOOL)

    //////////////////////////////////////////////////////////////////////
    // Binding PathElement       
    // In Python : PATH_ELEMENT
    {
        bp::implicitly_convertible< Schema &, PathElement >();
        bp::class_<PathElement> ("PATH_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                KARABO_PYTHON_COMMON_ATTRIBUTES(PathElement)
                KARABO_PYTHON_OPTIONS_NONVECTOR(PathElement)
                .def("isInputFile"
                     , &PathElement::isInputFile
                     , bp::return_internal_reference<> ())
                .def("isOutputFile"
                     , &PathElement::isOutputFile
                     , bp::return_internal_reference<> ())
                .def("isDirectory"
                     , &PathElement::isDirectory
                     , bp::return_internal_reference<> ())
                ;
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
            // Binding NodeElement       
            // In Python : NODE_ELEMENT
    {
        bp::implicitly_convertible< Schema &, NodeElement >();
        bp::class_<NodeElement> ("NODE_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                KARABO_PYTHON_NODE_CHOICE_LIST(NodeElement)
                .def("appendParametersOf"
                     , &NodeElementWrap::appendParametersOf, (bp::arg("python_class"))
                     , bp::return_internal_reference<> ())
                .def("appendParametersOfConfigurableClass"
                     , &NodeElementWrap::appendParametersOfConfigurableClass, (bp::arg("python_base_class"), bp::arg("classid"))
                     , bp::return_internal_reference<> ())
                ;
    }

    //////////////////////////////////////////////////////////////////////
    // Binding ListElement
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
                .def("appendNodesOfConfigurationBase"
                     , &ListElementWrap::appendNodesOfConfigurationBase, (bp::arg("python_base_class"))
                     , bp::return_internal_reference<> ())
                .def("appendAsNode"
                     , &ListElementWrap::appendAsNode, (bp::arg("python_class"), bp::arg("nodeName") = "")
                     , bp::return_internal_reference<> ())
                ;
    }

    //////////////////////////////////////////////////////////////////////
    // Binding ChoiceElement       
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
                .def("appendNodesOfConfigurationBase"
                     , &ChoiceElementWrap::appendNodesOfConfigurationBase, (bp::arg("python_base_class"))
                     , bp::return_internal_reference<> ())
                .def("appendAsNode"
                     , &ChoiceElementWrap::appendAsNode, (bp::arg("python_class"), bp::arg("nodeName") = "")
                     , bp::return_internal_reference<> ())
                .def("reconfigurable"
                     , &ChoiceElement::reconfigurable
                     , bp::return_internal_reference<> ())
                .def("init"
                     , &ChoiceElement::init
                     , bp::return_internal_reference<> ())
                ;
    }

    //////////////////////////////////////////////////////////////////////
    // Binding ImageElement       
    // In Python : IMAGE_ELEMENT

    {
        bp::implicitly_convertible< Schema &, ImageElement >();
        bp::class_<ImageElement>("IMAGE_ELEMENT", bp::init<Schema &>((bp::arg("expected"))))
            KARABO_PYTHON_IMAGE_ELEMENT(ImageElement)
        ;
     }
    
    //////////////////////////////////////////////////////////////////////
    // Binding InputElement       
    // In Python : INPUT_ELEMENT
    {
        bp::implicitly_convertible< Schema &, InputElement >();
        bp::class_<InputElement> ("INPUT_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                KARABO_PYTHON_NODE_CHOICE_LIST(InputElement)
                .def("setInputType"
                     , &InputElementWrap::setInputType, (bp::arg("python_base_class"))
                     , bp::return_internal_reference<> ())
                ;
    }

    //////////////////////////////////////////////////////////////////////
    // Binding OutputElement       
    // In Python : OUTPUT_ELEMENT
    {
        bp::implicitly_convertible< Schema &, OutputElement >();
        bp::class_<OutputElement> ("OUTPUT_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                KARABO_PYTHON_NODE_CHOICE_LIST(OutputElement)
                .def("setOutputType"
                     , &OutputElementWrap::setOutputType, (bp::arg("python_base_class"))
                     , bp::return_internal_reference<> ())
                ;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  DefaultValue<ChoiceElement> 
    {
        typedef DefaultValue<ChoiceElement, string> DefChoiceElement;
        bp::class_< DefChoiceElement, boost::noncopyable > ("DefaultValueChoiceElement", bp::no_init)
                .def("defaultValue"
                     , (ChoiceElement & (DefChoiceElement::*)(string const &))(&DefChoiceElement::defaultValue)
                     , (bp::arg("defValue"))
                     , bp::return_internal_reference<> ())
                .def("defaultValueFromString"
                     , (ChoiceElement & (DefChoiceElement::*)(string const &))(&DefChoiceElement::defaultValueFromString)
                     , (bp::arg("defValue"))
                     , bp::return_internal_reference<> ())
                .def("noDefaultValue"
                     , (ChoiceElement & (DefChoiceElement::*)())(&DefChoiceElement::noDefaultValue)
                     , bp::return_internal_reference<> ())
                ;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  DefaultValue<ListElement> 
    {
        typedef DefaultValue<ListElement, vector<string> > DefListElement;
        bp::class_< DefListElement, boost::noncopyable > ("DefaultValueListElement", bp::no_init)
                .def("defaultValue"
                     , &ListElementWrap::defaultValueList
                     , (bp::arg("self"), bp::arg("pyList"))
                     , bp::return_internal_reference<> ())
                .def("defaultValueFromString"
                     , (ListElement & (DefListElement::*)(string const &))(&DefListElement::defaultValueFromString)
                     , (bp::arg("defValue"))
                     , bp::return_internal_reference<> ())
                .def("noDefaultValue"
                     , (ListElement & (DefListElement::*)())(&DefListElement::noDefaultValue)
                     , bp::return_internal_reference<> ())
                ;
    }

    {
        bp::class_<Validator::ValidationRules>("ValidatorValidationRules", bp::init<>())
                .def_readwrite("injectDefaults", &Validator::ValidationRules::injectDefaults)
                .def_readwrite("allowUnrootedConfiguration", &Validator::ValidationRules::allowUnrootedConfiguration)
                .def_readwrite("allowAdditionalKeys", &Validator::ValidationRules::allowAdditionalKeys)
                .def_readwrite("allowMissingKeys", &Validator::ValidationRules::allowMissingKeys)
                .def_readwrite("injectTimestamps", &Validator::ValidationRules::injectTimestamps)
                ;

        bp::class_<Validator>("Validator", bp::init<>())
                .def(bp::init<const Validator::ValidationRules>())
                .def("validate", &ValidatorWrap::validate, (bp::arg("schema"), bp::arg("configuration"), bp::arg("timestamp") = Timestamp()))
                .def("setValidationRules", &ValidatorWrap::setValidationRules, (bp::arg("rules")))
                .def("getValidationRules", &ValidatorWrap::getValidationRules)
                .def("hasParametersInWarnOrAlarm", &ValidatorWrap::hasParametersInWarnOrAlarm)
                .def("getParametersInWarnOrAlarm", &ValidatorWrap::getParametersInWarnOrAlarm)
                ;
    }

    {
        bp::implicitly_convertible< Schema &, OverwriteElement >();
        bp::class_<OverwriteElement> ("OVERWRITE_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                .def("key"
                     , (OverwriteElement & (OverwriteElement::*)(string const &))(&OverwriteElement::key)
                     , (bp::arg("key"))
                     , bp::return_internal_reference<> ())
                .def("setNewDisplayedName"
                     , (OverwriteElement & (OverwriteElement::*)(string const &))(&OverwriteElement::setNewDisplayedName)
                     , (bp::arg("name"))
                     , bp::return_internal_reference<> ())
                .def("setNewDescription"
                     , (OverwriteElement & (OverwriteElement::*)(string const &))(&OverwriteElement::setNewDescription)
                     , (bp::arg("description"))
                     , bp::return_internal_reference<> ())
                .def("setNewAlias"
                     , &OverwriteElementWrap().setNewAlias
                     , (bp::arg("alias"))
                     , bp::return_internal_reference<> ())
                .def("setNewTag"
                     , &OverwriteElementWrap().setNewTag
                     , (bp::arg("tag"))
                     , bp::return_internal_reference<> ())
                .def("setNewAssignmentMandatory"
                     , (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNewAssignmentMandatory)
                     , bp::return_internal_reference<> ())
                .def("setNewAssignmentOptional"
                     , (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNewAssignmentOptional)
                     , bp::return_internal_reference<> ())
                .def("setNewAssignmentInternal"
                     , (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNewAssignmentInternal)
                     , bp::return_internal_reference<> ())
                .def("setNowInit"
                     , (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowInit)
                     , bp::return_internal_reference<> ())
                .def("setNowReconfigurable"
                     , (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowReconfigurable)
                     , bp::return_internal_reference<> ())
                .def("setNowReadOnly"
                     , (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::setNowReadOnly)
                     , bp::return_internal_reference<> ())
                .def("setNewDefaultValue"
                     , &OverwriteElementWrap().setNewDefaultValue
                     , (bp::arg("value"))
                     , bp::return_internal_reference<> ())
                .def("setNewMinInc"
                     , &OverwriteElementWrap().setNewMinInc
                     , (bp::arg("value"))
                     , bp::return_internal_reference<> ())
                .def("setNewMaxInc"
                     , &OverwriteElementWrap().setNewMaxInc
                     , (bp::arg("value"))
                     , bp::return_internal_reference<> ())
                .def("setNewMinExc"
                     , &OverwriteElementWrap().setNewMinExc
                     , (bp::arg("value"))
                     , bp::return_internal_reference<> ())
                .def("setNewMaxExc"
                     , &OverwriteElementWrap().setNewMaxExc
                     , (bp::arg("value"))
                     , bp::return_internal_reference<> ())
                .def("setNewOptions"
                     , (OverwriteElement & (OverwriteElement::*)(const std::string&, const std::string&))(&OverwriteElement::setNewOptions)
                     , (bp::arg("value"), bp::arg("sep") = ",;")
                     , bp::return_internal_reference<> ())
                .def("setNewAllowedState"
                     , (OverwriteElement& (OverwriteElement::*)(const std::string&, const std::string&))(&OverwriteElement::setNewAllowedState)
                     , (bp::arg("states"), bp::arg("sep") = " ,;")
                     , bp::return_internal_reference<> ())
                .def("setNowObserverAccess"
                     , (OverwriteElement& (OverwriteElement::*)())(&OverwriteElement::setNowObserverAccess)
                     , bp::return_internal_reference<> ())
                .def("setNowUserAccess"
                     , (OverwriteElement& (OverwriteElement::*)())(&OverwriteElement::setNowUserAccess)
                     , bp::return_internal_reference<> ())
                .def("setNowOperatorAccess"
                     , (OverwriteElement& (OverwriteElement::*)())(&OverwriteElement::setNowOperatorAccess)
                     , bp::return_internal_reference<> ())
                .def("setNowExpertAccess"
                     , (OverwriteElement& (OverwriteElement::*)())(&OverwriteElement::setNowExpertAccess)
                     , bp::return_internal_reference<> ())
                .def("setNowAdminAccess"
                     , (OverwriteElement& (OverwriteElement::*)())(&OverwriteElement::setNowAdminAccess)
                     , bp::return_internal_reference<> ())
                .def("setNewUnit"
                     , (OverwriteElement & (OverwriteElement::*)(const UnitType&))(&OverwriteElement::setNewUnit)
                     , (bp::arg("value"))
                     , bp::return_internal_reference<> ())
                .def("setNewMetricPrefix"
                     , (OverwriteElement & (OverwriteElement::*)(const MetricPrefixType&))(&OverwriteElement::setNewMetricPrefix)
                     , (bp::arg("value"))
                     , bp::return_internal_reference<> ())
                .def("commit"
                     , (void (OverwriteElement::*)())(&OverwriteElement::commit))
                ;
    }

    {
        bp::class_<HashFilterWrap, boost::noncopyable>("HashFilter", bp::no_init)
                .def("byTag", HashFilterWrap::byTag, (bp::arg("schema"), bp::arg("config"), bp::arg("tags"), bp::arg("sep") = ","))
                .staticmethod("byTag")
                ;
    }
} //end  exportPyUtilSchema

