/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
        
#include <exfel/util/Hash.hh>
#include <exfel/util/Schema.hh>
#include <exfel/util/Types.hh>

using namespace exfel::util;
using namespace std;
namespace bp = boost::python;

void exportPyUtilHash() {//exposing exfel::util::Hash
    typedef vector<std::string> keysSet;
    typedef void ( ::exfel::util::Hash::*update_function_type)(::exfel::util::Hash const &);
    
    bp::implicitly_convertible< std::string const &, Hash >();
    bp::class_<Hash> h ("Hash");
    h.def(bp::init< std::string const & >((bp::arg("key"))));
    h.def(bp::init< std::string const &, Hash > ((bp::arg("key"), bp::arg("value"))));
    h.def(bp::init< std::string const &, float >((bp::arg("key"), bp::arg("value"))));
    h.def(bp::init< std::string const &, std::string const & >((bp::arg("key"), bp::arg("value"))));
    h.def(bp::init< std::string const &, double >((bp::arg("key"), bp::arg("value"))));
    h.def(bp::init< std::string const &, int >((bp::arg("key"), bp::arg("value"))));
    //h.def(bp::init< std::string const &, bool >((bp::arg("key"), bp::arg("value"))));
    h.def(bp::init< std::string const &, char const * >((bp::arg("key"), bp::arg("value"))));
    h.def(bp::init< std::string const &, vector<int> >((bp::arg("key"), bp::arg("value"))));
    h.def(bp::init< std::string const &, vector<string> >((bp::arg("key"), bp::arg("value"))));
    h.def("append"
        , (Hash const & (Hash::*)(Hash const &))(&Hash::append)
        , (bp::arg("hash"))
        , bp::return_value_policy< bp::copy_const_reference > ());
    h.def("getAsString"
        , (string(Hash::*)(string const &) const) (&Hash::getAsString)
        , (bp::arg("key")));
     //h.def("getAsString"
    //    , (string(Hash::*)(_Rb_tree_const_iterator< std::pair< string const, boost::any > > const &) const) (&Hash::getAsString)
    //    , (bp::arg("it")));
    h.def("getNumeric"
        , (double ( Hash::*)(string const &) const) (&Hash::getNumeric)
        , (bp::arg("key")));

    h.def("getTypeAsString"
        , (string(Hash::*)(string const &) const) (&Hash::getTypeAsString)
        , (bp::arg("key")));
    //h.def("getTypeAsString"
    //    , (string(Hash::*)(_Rb_tree_const_iterator< pair< string const, boost::any > > const &) const) (&Hash::getTypeAsString)
    //    , (bp::arg("it")));
    h.def("getTypeAsId"
        , (Types::Type(Hash::*)(string const &) const) (&Hash::getTypeAsId)
        , (bp::arg("key")));     
    //h.def("getTypeAsId"
    //    , (Types::Type(Hash::*)(_Rb_tree_const_iterator< pair< string const, boost::any > > const &) const) (&Hash::getTypeAsId)
    //    , (bp::arg("it")));
    h.def("convertFromString"
        , (void ( Hash::*)(string const &, Types::Type const &)) (&Hash::convertFromString)
        , (bp::arg("key"), bp::arg("type")));
    h.def("has"
        , (bool(Hash::*)(string const &) const) (&Hash::has)
        , (bp::arg("key")));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, Hash const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, Schema const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, float const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, string const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, double const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, int const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, char const *, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPathAsBool"
        , (void ( Hash::*)(string const &, bool const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, vector< string > const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, vector< int > const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, vector< unsigned int > const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, vector< long long > const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, vector< unsigned long long > const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, deque< bool > const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, vector< double > const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &, vector< float > const &, string))(&Hash::setFromPath)
        , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setFromPath"
        , (void ( Hash::*)(string const &))(&Hash::setFromPath)
        , (bp::arg("path")));   
    h.def(bp::self_ns::str(bp::self));
    h.def("clear", &Hash::clear);
    h.def("getKeys", (keysSet(Hash::*)() const) (&Hash::getKeysAsVector));
    h.def("getLeaves", (keysSet(Hash::*)(string const &) const) (&Hash::getLeavesAsVector)
        , (bp::arg("sep") = "."));
   
    h.def("getFromPathAsVecBOOL"
        , (deque<bool> const & (Hash::*) (string const &, string))(&Hash::getFromPath<deque<bool> const>)
        , (bp::arg("path"), bp::arg("sep") = ".")
        , bp::return_value_policy< bp::copy_const_reference > ());
    
    h.def("update"
        , update_function_type(&::exfel::util::Hash::update)
        , (bp::arg("")));
    
    h.def("getAsVecBOOL"
        , ( deque<bool> const &(Hash::*)(string const &) )(&Hash::get)
        , (bp::arg("key"))
        , bp::return_value_policy< bp::copy_const_reference >() );
     
    h.def("set"
        , (void ( Hash::*)(string const &, Hash const &))(&Hash::set));
    h.def("set"
        , (void (Hash::*)(string const &, Schema const &))(&Hash::set));
    h.def("set"
        , (void (Hash::*)(string const &, float const &))(&Hash::set));
    h.def("set"
        , (void (Hash::*)(string const &, string const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, double const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, int const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, char const * const &))(&Hash::set));
    h.def("setAsBool"
        , (void ( Hash::*)(string const &, bool const &))(&Hash::set));
    h.def("set"
        , (void (Hash::*)(string const &, vector<float> const &))(&Hash::set));
    h.def("set"
        , (void (Hash::*)(string const &, vector<string> const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, vector<double> const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, vector<int> const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, vector<unsigned int> const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, vector<long long> const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, vector<unsigned long long> const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, deque< bool > const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, wchar_t const * const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, wchar_t * const &))(&Hash::set));
    h.def("set"
        , (void ( Hash::*)(string const &, ::boost::any const &))(&Hash::set));        
    h.def("__iter__", bp::iterator<Hash>());
    
    typedef std::vector< exfel::util::Hash > vecHash;
    bp::class_<vecHash>("vecHash")
     .def("__len__", &std::vector<vecHash>::size)
     .def("clear", &std::vector<vecHash>::clear)
     .def("__iter__", bp::iterator<vecHash>())
    ;
}

