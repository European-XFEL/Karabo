/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include "exfel/util/Factory.hh"
#include "PythonLoader.hh"

namespace bp = boost::python;
using namespace exfel::util;

void exportPyUtilConfig() {
    { //::std::vector< std::string >
        typedef bp::class_< std::vector< std::string > > vectorstring_exposer_t;
        vectorstring_exposer_t vectorstring_exposer = vectorstring_exposer_t( "vecString" );
        bp::scope vectorstring_scope( vectorstring_exposer );
        vectorstring_exposer.def( bp::vector_indexing_suite< ::std::vector< std::string >, true >() );
    }

    bp::class_< std::vector< int > >("vecInt32")
        .def( bp::vector_indexing_suite< ::std::vector< int >, true >() );
    
    bp::class_< std::vector< unsigned int > >("vecUInt32")
        .def( bp::vector_indexing_suite< ::std::vector< unsigned int >, true >() );
    
    bp::class_< std::vector< long long > >("vecInt64")
        .def( bp::vector_indexing_suite< ::std::vector< long long >, true >() );
   
    bp::class_< std::vector< unsigned long long > >("vecUInt64")
        .def( bp::vector_indexing_suite< ::std::vector< unsigned long long >, true >() );
    
    bp::class_< std::deque< bool > >("vecBool")
        .def( bp::vector_indexing_suite< ::std::deque< bool >, true >() );
    
    bp::class_< std::vector< double > >("vecDouble")
        .def( bp::vector_indexing_suite< ::std::vector< double >, true >() );
    
    bp::class_< std::vector< float > >("vecFloat")
        .def( bp::vector_indexing_suite< ::std::vector< float >, true >() );
    
  {//exposing ::exfel::util::Schema
  typedef bp::class_< exfel::util::Schema, bp::bases< exfel::util::Hash > > Config_exposer_t;
  Config_exposer_t Config_exposer = Config_exposer_t("Schema", bp::init< >());
  bp::scope Config_scope(Config_exposer);
  bp::enum_< exfel::util::Schema::AssignmentType > ("AssignmentType")
          .value("OPTIONAL", exfel::util::Schema::OPTIONAL)
          .value("MANDATORY", exfel::util::Schema::MANDATORY)
          .value("INTERNAL", exfel::util::Schema::INTERNAL)
          .export_values()
          ;
  bp::enum_< exfel::util::Schema::ExpertLevelType > ("ExpertLevelType")
          .value("SIMPLE", exfel::util::Schema::SIMPLE)
          .value("MEDIUM", exfel::util::Schema::MEDIUM)
          .value("ADVANCED", exfel::util::Schema::ADVANCED)
          .export_values()
          ;
  bp::enum_< exfel::util::Schema::OccuranceType > ("OccuranceType")
          .value("EXACTLY_ONCE", exfel::util::Schema::EXACTLY_ONCE)
          .value("ONE_OR_MORE", exfel::util::Schema::ONE_OR_MORE)
          .value("ZERO_OR_ONE", exfel::util::Schema::ZERO_OR_ONE)
          .value("ZERO_OR_MORE", exfel::util::Schema::ZERO_OR_MORE)
          .value("EITHER_OR", exfel::util::Schema::EITHER_OR)
          .export_values()
          ;

  Config_exposer.def( bp::init< std::string const & >(( bp::arg("memberName") )) );
  bp::implicitly_convertible< std::string const &, exfel::util::Schema >();
  Config_exposer.def( bp::init< std::string const &, exfel::util::Schema >(( bp::arg("memberName"), bp::arg("value") )) );
  Config_exposer.def( bp::init< std::string const &, float >(( bp::arg("memberName"), bp::arg("value") )) );
  Config_exposer.def( bp::init< std::string const &, std::string const & >(( bp::arg("memberName"), bp::arg("value") )) );
  Config_exposer.def( bp::init< std::string const &, double >(( bp::arg("memberName"), bp::arg("value") )) );
  Config_exposer.def( bp::init< std::string const &, int >(( bp::arg("memberName"), bp::arg("value") )) );
  Config_exposer.def( bp::init< std::string const &, char const * >(( bp::arg("memberName"), bp::arg("value") )) );
  //Config_exposer.def( bp::init< exfel::util::Hash const & >(( bp::arg("hash") )) );
  //bp::implicitly_convertible< exfel::util::Hash const &, exfel::util::Schema >();
  { //::exfel::util::Schema::getFromPath<double const>

    typedef double const & (::exfel::util::Schema::*getFromPathAsDouble_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsDouble"
            , getFromPathAsDouble_function_type(&::exfel::util::Schema::getFromPath<double const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

  }
  { //::exfel::util::Schema::getFromPath<exfel::util::Schema const>

    typedef ::exfel::util::Schema const & (::exfel::util::Schema::*getFromPathAsConfig_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsConfig"
            , getFromPathAsConfig_function_type(&::exfel::util::Schema::getFromPath<exfel::util::Schema const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

  }
  { //::exfel::util::Schema::getFromPath<float const>

    typedef float const & (::exfel::util::Schema::*getFromPathAsFloat_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsFloat"
            , getFromPathAsFloat_function_type(&::exfel::util::Schema::getFromPath<float const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

  }
  { //::exfel::util::Schema::getFromPath<int const>

    typedef int const & (::exfel::util::Schema::*getFromPathAsInt_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsInt"
            , getFromPathAsInt_function_type(&::exfel::util::Schema::getFromPath<int const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

  }
  { //::exfel::util::Schema::getFromPath<std::string const>

    typedef ::std::string const & (::exfel::util::Schema::*getFromPathAsString_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsString"
            , getFromPathAsString_function_type(&::exfel::util::Schema::getFromPath<std::string const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

  }
  { //::exfel::util::Schema::getFromPath<std::vector<string > const>

    typedef ::std::vector<std::string> const & (::exfel::util::Schema::*getFromPathAsVecString_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsVecString"
            , getFromPathAsVecString_function_type(&::exfel::util::Schema::getFromPath<std::vector<std::string> const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

  }
  { //::exfel::util::Schema::getFromPath<std::vector<int > const>

    typedef ::std::vector<int> const & (::exfel::util::Schema::*getFromPathAsVecInt32_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsVecInt32"
            , getFromPathAsVecInt32_function_type(&::exfel::util::Schema::getFromPath<std::vector<int> const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

  }
  { //::exfel::util::Schema::getFromPath<std::vector<unsigned int > const>

    typedef ::std::vector<unsigned int> const & (::exfel::util::Schema::*getFromPathAsVecUInt32_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsVecUInt32"
            , getFromPathAsVecUInt32_function_type(&::exfel::util::Schema::getFromPath<std::vector<unsigned int> const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

  }
  { //::exfel::util::Schema::getFromPath<std::vector<long long > const>

    typedef ::std::vector<long long> const & (::exfel::util::Schema::*getFromPathAsVecInt64_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsVecInt64"
            , getFromPathAsVecInt64_function_type(&::exfel::util::Schema::getFromPath<std::vector<long long> const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

 }
 { //::exfel::util::Schema::getFromPath<std::vector<unsigned long long > const>

    typedef ::std::vector<unsigned long long> const & (::exfel::util::Schema::*getFromPathAsVecUInt64_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsVecUInt64"
            , getFromPathAsVecUInt64_function_type(&::exfel::util::Schema::getFromPath<std::vector<unsigned long long> const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

 }
 { //::exfel::util::Schema::getFromPath<std::deque<bool > const>

    typedef ::std::deque<bool> const & (::exfel::util::Schema::*getFromPathAsVecBool_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsVecBool"
            , getFromPathAsVecBool_function_type(&::exfel::util::Schema::getFromPath<std::deque<bool> const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

 }
 { //::exfel::util::Schema::getFromPath<std::vector<double > const>

    typedef ::std::vector<double> const & (::exfel::util::Schema::*getFromPathAsVecDouble_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsVecDouble"
            , getFromPathAsVecDouble_function_type(&::exfel::util::Schema::getFromPath<std::vector<double> const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

 }
 { //::exfel::util::Schema::getFromPath<std::vector<float > const>

    typedef ::std::vector<float> const & (::exfel::util::Schema::*getFromPathAsVecFloat_function_type)(::std::string const &, ::std::string);

    Config_exposer.def(
            "getFromPathAsVecFloat"
            , getFromPathAsVecFloat_function_type(&::exfel::util::Schema::getFromPath<std::vector<float> const>)
            , (bp::arg("path"), bp::arg("sep") = ".")
            , bp::return_value_policy< bp::copy_const_reference > ());

 }
{ //::exfel::util::Schema::initParameterDescription

  typedef ::exfel::util::Schema & ( ::exfel::util::Schema::*initParameterDescription_function_type )( ::std::string const &,::exfel::util::AccessType ) ;

  Config_exposer.def(
          "initParameterDescription"
          , initParameterDescription_function_type( &::exfel::util::Schema::initParameterDescription )
          , ( bp::arg("key"), bp::arg("accessMode")=::exfel::util::INIT )
          , bp::return_value_policy< bp::copy_non_const_reference >() );

  }
  { //::exfel::util::Schema::isDescription

    typedef bool ( ::exfel::util::Schema::*isDescription_function_type)() const;

    Config_exposer.def(
            "isDescription"
            , isDescription_function_type(&::exfel::util::Schema::isDescription));

  }
//  { //::exfel::util::Schema::merge
//
//    typedef void ( ::exfel::util::Schema::*merge_function_type)(::exfel::util::Schema const &);
//
//    Config_exposer.def(
//            "merge"
//            , merge_function_type(&::exfel::util::Schema::merge)
//            , (bp::arg("config")));
//
//  }
  { //::exfel::util::Schema::update

    typedef void ( ::exfel::util::Schema::*update_function_type)(::exfel::util::Schema const &);

    Config_exposer.def(
            "update"
            , update_function_type(&::exfel::util::Schema::update)
            , (bp::arg("config")));

  }
//  { //::exfel::util::Schema::options(const std::string& options)
//
//    typedef void ( ::exfel::util::Schema::*options_function_type)(::std::string const &);
//
//    Config_exposer.def(
//            "options"
//            , options_function_type(&::exfel::util::Schema::options)
//            , (bp::arg("options")));
//
//  }
//  { //::exfel::util::Schema::options(const vector<string>& options)
//    typedef std::vector<std::string> optionsVector;
//    typedef void ( exfel::util::Schema::*optionsVect_function_type)(optionsVector const &);
//
//    Config_exposer.def(
//            "options"
//            , optionsVect_function_type(&exfel::util::Schema::options)
//            , (bp::arg("optionsVect")));
//
//  }
  { //::exfel::util::Schema::setFromPath

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, ::exfel::util::Schema const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, float const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, ::std::string const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, double const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, int const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath  vector STRING

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, std::vector< std::string > const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath  vector INT32

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, std::vector< int > const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath  vector UINT32

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, std::vector< unsigned int > const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath  vector INT64

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, std::vector< long long > const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath  vector UINT32

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, std::vector< unsigned long long > const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath  deque BOOL

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, std::deque< bool > const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath  vector DOUBLE

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, std::vector< double > const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath  vector FLOAT

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, std::vector< float > const &, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &, char const *, ::std::string);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

  }
  { //::exfel::util::Schema::setFromPath

    typedef void ( ::exfel::util::Schema::*setFromPath_function_type)(::std::string const &);

    Config_exposer.def(
            "setFromPath"
            , setFromPath_function_type(&::exfel::util::Schema::setFromPath)
            , (bp::arg("path")));

  }
  { //::exfel::util::Schema::validate

    typedef ::exfel::util::Schema(::exfel::util::Schema::*validate_function_type)(::exfel::util::Schema const &) const;

    Config_exposer.def(
            "validate"
            , validate_function_type(&::exfel::util::Schema::validate)
            , (bp::arg("user")));

  }
  { //::exfel::util::Schema::mergeUserInput

    typedef std::vector<exfel::util::Schema> mergeUserInputVector;
    typedef exfel::util::Schema(exfel::util::Schema::*mergeUserInput_function_type)(mergeUserInputVector const &) const;

    Config_exposer.def(
            "mergeUserInput"
            , mergeUserInput_function_type(&exfel::util::Schema::mergeUserInput)
            , (bp::arg("user")));
  }
}// end Schema

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//DefaultValue<SimpleElement...

bp::class_< exfel::util::DefaultValue< exfel::util::SimpleElement< int >, int >, boost::noncopyable >( "DefaultValueInt", bp::no_init )
        .def(
            "defaultValue"
            , (::exfel::util::SimpleElement< int > & ( ::exfel::util::DefaultValue<exfel::util::SimpleElement<int>, int>::* )( int const & ) )( &::exfel::util::DefaultValue< exfel::util::SimpleElement< int >, int >::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (::exfel::util::SimpleElement< int > & ( ::exfel::util::DefaultValue<exfel::util::SimpleElement<int>, int>::* )( ::std::string const & ) )( &::exfel::util::DefaultValue< exfel::util::SimpleElement< int >, int >::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (::exfel::util::SimpleElement< int > & ( ::exfel::util::DefaultValue<exfel::util::SimpleElement<int>, int>::* )(  ) )( &::exfel::util::DefaultValue< exfel::util::SimpleElement< int >, int >::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< unsigned int > SimpleElemUINT32;
    typedef DefaultValue< SimpleElemUINT32, unsigned int > DefValueSimpleElemUINT32;
    bp::class_< DefaultValue< SimpleElement< unsigned int >, unsigned int >, boost::noncopyable >( "DefaultValueSimpleElementUINT32", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemUINT32 & ( DefValueSimpleElemUINT32::* )( unsigned int const & ) )( &DefValueSimpleElemUINT32::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemUINT32 & ( DefValueSimpleElemUINT32::* )( ::std::string const & ) )( &DefValueSimpleElemUINT32::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemUINT32 & ( DefValueSimpleElemUINT32::* )(  ) )( &DefValueSimpleElemUINT32::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< long long > SimpleElemINT64;
    typedef DefaultValue< SimpleElemINT64, long long > DefValueSimpleElemINT64;
    bp::class_< DefaultValue< SimpleElement< long long >, long long >, boost::noncopyable >( "DefaultValueSimpleElementINT64", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemINT64 & ( DefValueSimpleElemINT64::* )( long long const & ) )( &DefValueSimpleElemINT64::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemINT64 & ( DefValueSimpleElemINT64::* )( ::std::string const & ) )( &DefValueSimpleElemINT64::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemINT64 & ( DefValueSimpleElemINT64::* )(  ) )( &DefValueSimpleElemINT64::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< unsigned long long > SimpleElemUINT64;
    typedef DefaultValue< SimpleElemUINT64, unsigned long long > DefValueSimpleElemUINT64;
    bp::class_< DefaultValue< SimpleElement< unsigned long long >, unsigned long long >, boost::noncopyable >( "DefaultValueSimpleElementUINT64", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemUINT64 & ( DefValueSimpleElemUINT64::* )( unsigned long long const & ) )( &DefValueSimpleElemUINT64::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemUINT64 & ( DefValueSimpleElemUINT64::* )( ::std::string const & ) )( &DefValueSimpleElemUINT64::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemUINT64 & ( DefValueSimpleElemUINT64::* )(  ) )( &DefValueSimpleElemUINT64::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< signed char > SimpleElemINT8;
    typedef DefaultValue< SimpleElemINT8, signed char > DefValueSimpleElemINT8;
    bp::class_< DefaultValue< SimpleElement< signed char >, signed char >, boost::noncopyable >( "DefaultValueSimpleElementINT8", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemINT8 & ( DefValueSimpleElemINT8::* )( signed char const & ) )( &DefValueSimpleElemINT8::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemINT8 & ( DefValueSimpleElemINT8::* )( ::std::string const & ) )( &DefValueSimpleElemINT8::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemINT8 & ( DefValueSimpleElemINT8::* )(  ) )( &DefValueSimpleElemINT8::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< unsigned char > SimpleElemUINT8;
    typedef DefaultValue< SimpleElemUINT8, unsigned char > DefValueSimpleElemUINT8;
    bp::class_< DefaultValue< SimpleElement< unsigned char >, unsigned char >, boost::noncopyable >( "DefaultValueSimpleElementUINT8", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemUINT8 & ( DefValueSimpleElemUINT8::* )( unsigned char const & ) )( &DefValueSimpleElemUINT8::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemUINT8 & ( DefValueSimpleElemUINT8::* )( ::std::string const & ) )( &DefValueSimpleElemUINT8::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemUINT8 & ( DefValueSimpleElemUINT8::* )(  ) )( &DefValueSimpleElemUINT8::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< signed short > SimpleElemINT16;
    typedef DefaultValue< SimpleElemINT16, signed short > DefValueSimpleElemINT16;
    bp::class_< DefaultValue< SimpleElement< signed short >, signed short >, boost::noncopyable >( "DefaultValueSimpleElementINT16", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemINT16 & ( DefValueSimpleElemINT16::* )( signed short const & ) )( &DefValueSimpleElemINT16::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemINT16 & ( DefValueSimpleElemINT16::* )( ::std::string const & ) )( &DefValueSimpleElemINT16::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemINT16 & ( DefValueSimpleElemINT16::* )(  ) )( &DefValueSimpleElemINT16::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< unsigned short > SimpleElemUINT16;
    typedef DefaultValue< SimpleElemUINT8, unsigned short > DefValueSimpleElemUINT16;
    bp::class_< DefaultValue< SimpleElement< unsigned short >, unsigned short >, boost::noncopyable >( "DefaultValueSimpleElementUINT16", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemUINT16 & ( DefValueSimpleElemUINT8::* )( unsigned short const & ) )( &DefValueSimpleElemUINT16::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemUINT16 & ( DefValueSimpleElemUINT8::* )( ::std::string const & ) )( &DefValueSimpleElemUINT16::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemUINT16 & ( DefValueSimpleElemUINT8::* )(  ) )( &DefValueSimpleElemUINT16::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< double > SimpleElemDOUBLE;
    typedef DefaultValue< SimpleElemDOUBLE, double > DefValueSimpleElemDOUBLE;
    bp::class_< DefaultValue< SimpleElement< double >, double >, boost::noncopyable >( "DefaultValueSimpleElementDOUBLE", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemDOUBLE & ( DefValueSimpleElemDOUBLE::* )( double const & ) )( &DefValueSimpleElemDOUBLE::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemDOUBLE & ( DefValueSimpleElemDOUBLE::* )( ::std::string const & ) )( &DefValueSimpleElemDOUBLE::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemDOUBLE & ( DefValueSimpleElemDOUBLE::* )(  ) )( &DefValueSimpleElemDOUBLE::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< std::string > SimpleElemSTRING;
    typedef DefaultValue< SimpleElemSTRING, std::string > DefValueSimpleElemSTRING;
    bp::class_< DefaultValue< SimpleElement< std::string >, std::string >, boost::noncopyable >( "DefaultValueSimpleElementSTRING", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemSTRING & ( DefValueSimpleElemSTRING::* )( std::string const & ) )( &DefValueSimpleElemSTRING::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemSTRING & ( DefValueSimpleElemSTRING::* )( ::std::string const & ) )( &DefValueSimpleElemSTRING::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemSTRING & ( DefValueSimpleElemSTRING::* )(  ) )( &DefValueSimpleElemSTRING::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< bool > SimpleElemBOOL;
    typedef DefaultValue< SimpleElemBOOL, bool > DefValueSimpleElemBOOL;
    bp::class_< DefaultValue< SimpleElement< bool >, bool >, boost::noncopyable >( "DefaultValueSimpleElementBOOL", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemBOOL & ( DefValueSimpleElemBOOL::* )( bool const & ) )( &DefValueSimpleElemBOOL::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemBOOL & ( DefValueSimpleElemBOOL::* )( ::std::string const & ) )( &DefValueSimpleElemBOOL::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemBOOL & ( DefValueSimpleElemBOOL::* )(  ) )( &DefValueSimpleElemBOOL::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< float > SimpleElemFLOAT;
    typedef DefaultValue< SimpleElemFLOAT, float > DefValueSimpleElemFLOAT;
    bp::class_< DefaultValue< SimpleElement< float >, float >, boost::noncopyable >( "DefaultValueSimpleElementFLOAT", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemFLOAT & ( DefValueSimpleElemFLOAT::* )( float const & ) )( &DefValueSimpleElemFLOAT::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemFLOAT & ( DefValueSimpleElemFLOAT::* )( ::std::string const & ) )( &DefValueSimpleElemFLOAT::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemFLOAT & ( DefValueSimpleElemFLOAT::* )(  ) )( &DefValueSimpleElemFLOAT::noDefaultValue )
            , bp::return_internal_reference<> () );

    typedef SimpleElement< boost::filesystem::path > SimpleElemPATH;
    typedef DefaultValue< SimpleElemPATH, boost::filesystem::path > DefValueSimpleElemPATH;
    bp::class_< DefaultValue< SimpleElement< boost::filesystem::path >, boost::filesystem::path >, boost::noncopyable >( "DefaultValueSimpleElementPATH", bp::no_init )
        .def(
            "defaultValue"
            , (SimpleElemPATH & ( DefValueSimpleElemPATH::* )( boost::filesystem::path const & ) )( &DefValueSimpleElemPATH::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (SimpleElemPATH & ( DefValueSimpleElemPATH::* )( ::std::string const & ) )( &DefValueSimpleElemPATH::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (SimpleElemPATH & ( DefValueSimpleElemPATH::* )(  ) )( &DefValueSimpleElemPATH::noDefaultValue )
            , bp::return_internal_reference<> () );

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//DefaultValue<VectorElement...

bp::class_< exfel::util::DefaultValue< exfel::util::VectorElement< int, std::vector >, std::vector< int > >, boost::noncopyable >( "DefaultValueVectorInt32", bp::no_init )
        .def(
            "defaultValue"
            , (::exfel::util::VectorElement< int, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<int, std::vector>, std::vector<int, std::allocator<int> > >::* )( ::std::vector< int > const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (::exfel::util::VectorElement< int, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<int, std::vector>, std::vector<int, std::allocator<int> > >::* )( ::std::string const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (::exfel::util::VectorElement< int, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<int, std::vector>, std::vector<int, std::allocator<int> > >::* )(  ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::noDefaultValue )
            , bp::return_internal_reference<> () );
bp::class_< exfel::util::DefaultValue< exfel::util::VectorElement< unsigned int, std::vector >, std::vector< unsigned int > >, boost::noncopyable >( "DefaultValue_less__exfel_scope_util_scope_VectorElement_less__unsigned_int_comma__std_scope_vector__greater__comma__std_scope_vector_less__unsigned_int__greater___greater_", bp::no_init )
        .def(
            "defaultValue"
            , (::exfel::util::VectorElement< unsigned int, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<unsigned int, std::vector>, std::vector<unsigned int, std::allocator<unsigned int> > >::* )( ::std::vector< unsigned int > const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< unsigned int, std::vector >, std::vector< unsigned int > >::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (::exfel::util::VectorElement< unsigned int, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<unsigned int, std::vector>, std::vector<unsigned int, std::allocator<unsigned int> > >::* )( ::std::string const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< unsigned int, std::vector >, std::vector< unsigned int > >::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (::exfel::util::VectorElement< unsigned int, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<unsigned int, std::vector>, std::vector<unsigned int, std::allocator<unsigned int> > >::* )(  ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< unsigned int, std::vector >, std::vector< unsigned int > >::noDefaultValue )
            , bp::return_internal_reference<> () );
bp::class_< exfel::util::DefaultValue< exfel::util::VectorElement< long long, std::vector >, std::vector< long long > >, boost::noncopyable >( "DefaultValue_less__exfel_scope_util_scope_VectorElement_less__long_long_comma__std_scope_vector__greater__comma__std_scope_vector_less__long_long__greater___greater_", bp::no_init )
        .def(
            "defaultValue"
            , (::exfel::util::VectorElement< long long, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<long long, std::vector>, std::vector<long long, std::allocator<long long> > >::* )( ::std::vector< long long int > const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< long long, std::vector >, std::vector< long long > >::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (::exfel::util::VectorElement< long long, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<long long, std::vector>, std::vector<long long, std::allocator<long long> > >::* )( ::std::string const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< long long, std::vector >, std::vector< long long > >::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (::exfel::util::VectorElement< long long, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<long long, std::vector>, std::vector<long long, std::allocator<long long> > >::* )(  ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< long long, std::vector >, std::vector< long long > >::noDefaultValue )
            , bp::return_internal_reference<> () );
bp::class_< exfel::util::DefaultValue< exfel::util::VectorElement< unsigned long long, std::vector >, std::vector< unsigned long long > >, boost::noncopyable >( "DefaultValue_less__exfel_scope_util_scope_VectorElement_less__unsigned_long_long_comma__std_scope_vector__greater__comma__std_scope_vector_less__unsigned_long_long__greater___greater_", bp::no_init )
        .def(
            "defaultValue"
            , (::exfel::util::VectorElement< unsigned long long, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<unsigned long long, std::vector>, std::vector<unsigned long long, std::allocator<unsigned long long> > >::* )( ::std::vector< long long unsigned int > const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< unsigned long long, std::vector >, std::vector< unsigned long long > >::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (::exfel::util::VectorElement< unsigned long long, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<unsigned long long, std::vector>, std::vector<unsigned long long, std::allocator<unsigned long long> > >::* )( ::std::string const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< unsigned long long, std::vector >, std::vector< unsigned long long > >::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (::exfel::util::VectorElement< unsigned long long, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<unsigned long long, std::vector>, std::vector<unsigned long long, std::allocator<unsigned long long> > >::* )(  ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< unsigned long long, std::vector >, std::vector< unsigned long long > >::noDefaultValue )
            , bp::return_internal_reference<> () );
bp::class_< exfel::util::DefaultValue< exfel::util::VectorElement< std::string, std::vector >, std::vector< std::string > >, boost::noncopyable >( "DefaultValue_less__exfel_scope_util_scope_VectorElement_less__std_scope_string_comma__std_scope_vector__greater__comma__std_scope_vector_less__std_scope_string__greater___greater_", bp::no_init )
        .def(
            "defaultValue"
            , (::exfel::util::VectorElement< std::string, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<std::string, std::vector>, std::vector<std::string, std::allocator<std::string> > >::* )( ::std::vector< std::string > const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< std::string, std::vector >, std::vector< std::string > >::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (::exfel::util::VectorElement< std::string, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<std::string, std::vector>, std::vector<std::string, std::allocator<std::string> > >::* )( ::std::string const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< std::string, std::vector >, std::vector< std::string > >::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (::exfel::util::VectorElement< std::string, std::vector > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<std::string, std::vector>, std::vector<std::string, std::allocator<std::string> > >::* )(  ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< std::string, std::vector >, std::vector< std::string > >::noDefaultValue )
            , bp::return_internal_reference<> () );
bp::class_< exfel::util::DefaultValue< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >, boost::noncopyable >( "DefaultValue_less__exfel_scope_util_scope_VectorElement_less__bool_comma__std_scope_deque__greater__comma__std_scope_deque_less__bool__greater___greater_", bp::no_init )
        .def(
            "defaultValue"
            , (::exfel::util::VectorElement< bool, std::deque > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<bool, std::deque>, std::deque<bool, std::allocator<bool> > >::* )( ::std::deque< bool > const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (::exfel::util::VectorElement< bool, std::deque > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<bool, std::deque>, std::deque<bool, std::allocator<bool> > >::* )( ::std::string const & ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (::exfel::util::VectorElement< bool, std::deque > & ( ::exfel::util::DefaultValue<exfel::util::VectorElement<bool, std::deque>, std::deque<bool, std::allocator<bool> > >::* )(  ) )( &::exfel::util::DefaultValue< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >::noDefaultValue )
            , bp::return_internal_reference<> () );

  typedef VectorElement< double, std::vector > VectorElemDouble;
  typedef DefaultValue< VectorElemDouble, std::vector< double > > DefValueDouble;
  bp::class_< DefValueDouble, boost::noncopyable >( "DefaultValueVectorElementDOUBLE", bp::no_init )
        .def(
            "defaultValue"
            , (VectorElemDouble & ( DefaultValue<VectorElemDouble, std::vector<double, std::allocator<double> > >::* )( ::std::vector< double > const & ) )( &DefValueDouble::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (VectorElemDouble & ( DefaultValue<VectorElemDouble, std::vector<double, std::allocator<double> > >::* )( ::std::string const & ) )( &DefValueDouble::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (VectorElemDouble & ( DefaultValue<VectorElemDouble, std::vector<double, std::allocator<double> > >::* )(  ) )( &DefValueDouble::noDefaultValue )
            , bp::return_internal_reference<> () );

  typedef VectorElement< signed char, std::vector > VectorElemInt8;
  typedef DefaultValue< VectorElemInt8, std::vector< signed char > > DefValueInt8;
            bp::class_< DefValueInt8, boost::noncopyable >( "DefaultValueVectorElementINT8", bp::no_init )
        .def(
            "defaultValue"
            , (VectorElemInt8 & ( DefaultValue<VectorElemInt8, std::vector<signed char, std::allocator<signed char> > >::* )( ::std::vector< signed char > const & ) )( &DefValueInt8::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (VectorElemInt8 & ( DefaultValue<VectorElemInt8, std::vector<signed char, std::allocator<signed char> > >::* )( ::std::string const & ) )( &DefValueInt8::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (VectorElemInt8 & ( DefaultValue<VectorElemInt8, std::vector<signed char, std::allocator<signed char> > >::* )(  ) )( &DefValueInt8::noDefaultValue )
            , bp::return_internal_reference<> () );

  typedef VectorElement< unsigned char, std::vector > VectorElemUInt8;
  typedef DefaultValue< VectorElemUInt8, std::vector< unsigned char > > DefValueUInt8;
  bp::class_< DefValueUInt8, boost::noncopyable >( "DefaultValueVectorElementUINT8", bp::no_init )
        .def(
            "defaultValue"
            , (VectorElemUInt8 & ( DefaultValue<VectorElemUInt8, std::vector<unsigned char, std::allocator<unsigned char> > >::* )( ::std::vector< unsigned char > const & ) )( &DefValueUInt8::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (VectorElemUInt8 & ( DefaultValue<VectorElemUInt8, std::vector<unsigned char, std::allocator<unsigned char> > >::* )( ::std::string const & ) )( &DefValueUInt8::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (VectorElemUInt8 & ( DefaultValue<VectorElemUInt8, std::vector<unsigned char, std::allocator<unsigned char> > >::* )(  ) )( &DefValueUInt8::noDefaultValue )
            , bp::return_internal_reference<> () );

  typedef VectorElement< signed short, std::vector > VectorElemInt16;
  typedef DefaultValue< VectorElemInt16, std::vector< signed short > > DefValueInt16;
            bp::class_< DefValueInt16, boost::noncopyable >( "DefaultValueVectorElementINT16", bp::no_init )
        .def(
            "defaultValue"
            , (VectorElemInt16 & ( DefaultValue<VectorElemInt16, std::vector<signed short, std::allocator<signed short> > >::* )( ::std::vector< signed short > const & ) )( &DefValueInt16::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (VectorElemInt16 & ( DefaultValue<VectorElemInt16, std::vector<signed short, std::allocator<signed short> > >::* )( ::std::string const & ) )( &DefValueInt16::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (VectorElemInt16 & ( DefaultValue<VectorElemInt16, std::vector<signed short, std::allocator<signed short> > >::* )(  ) )( &DefValueInt16::noDefaultValue )
            , bp::return_internal_reference<> () );

  typedef VectorElement< unsigned short, std::vector > VectorElemUInt16;
  typedef DefaultValue< VectorElemUInt16, std::vector< unsigned short > > DefValueUInt16;
  bp::class_< DefValueUInt16, boost::noncopyable >( "DefaultValueVectorElementUINT16", bp::no_init )
        .def(
            "defaultValue"
            , (VectorElemUInt16 & ( DefaultValue<VectorElemUInt16, std::vector<unsigned short, std::allocator<unsigned short> > >::* )( ::std::vector< unsigned short > const & ) )( &DefValueUInt16::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (VectorElemUInt16 & ( DefaultValue<VectorElemUInt16, std::vector<unsigned short, std::allocator<unsigned short> > >::* )( ::std::string const & ) )( &DefValueUInt16::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (VectorElemUInt16 & ( DefaultValue<VectorElemUInt16, std::vector<unsigned short, std::allocator<unsigned short> > >::* )(  ) )( &DefValueUInt16::noDefaultValue )
            , bp::return_internal_reference<> () );

  typedef VectorElement< float, std::vector > VectorElemFLOAT;
  typedef DefaultValue< VectorElemFLOAT, std::vector< float > > DefValueFLOAT;
  bp::class_< DefValueFLOAT, boost::noncopyable >( "DefaultValueVectorElementFLOAT", bp::no_init )
        .def(
            "defaultValue"
            , (VectorElemFLOAT & ( DefaultValue< VectorElemFLOAT, std::vector<float, std::allocator<float> > >::* )( std::vector< float > const & ) )( &DefValueFLOAT::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (VectorElemFLOAT & ( DefaultValue<VectorElemFLOAT, std::vector<float, std::allocator<float> > >::* )( std::string const & ) )( &DefValueFLOAT::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (VectorElemFLOAT & ( DefaultValue<VectorElemFLOAT, std::vector<float, std::allocator<float> > >::* )(  ) )( &DefValueFLOAT::noDefaultValue )
            , bp::return_internal_reference<> () );

  typedef VectorElement< boost::filesystem::path, std::vector > VectorElemPATH;
  typedef DefaultValue< VectorElemPATH, std::vector< boost::filesystem::path > > DefValuePATH;
  bp::class_< DefValuePATH, boost::noncopyable >( "DefaultValueVectorElementPATH", bp::no_init )
        .def(
            "defaultValue"
            , (VectorElemPATH & ( DefaultValue< VectorElemPATH, std::vector<boost::filesystem::path, std::allocator<boost::filesystem::path> > >::* )( std::vector< boost::filesystem::path > const & ) )( &DefValuePATH::defaultValue )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "defaultValueFromString"
            , (VectorElemPATH & ( DefaultValue<VectorElemPATH, std::vector<boost::filesystem::path, std::allocator<boost::filesystem::path> > >::* )( std::string const & ) )( &DefValuePATH::defaultValueFromString )
            , ( bp::arg("defaultValue") )
            , bp::return_internal_reference<> () )
        .def(
            "noDefaultValue"
            , (VectorElemPATH & ( DefaultValue<VectorElemPATH, std::vector<boost::filesystem::path, std::allocator<boost::filesystem::path> > >::* )(  ) )( &DefValuePATH::noDefaultValue )
            , bp::return_internal_reference<> () );

  //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
  //GenerichElement<SimpleElement ...

     { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >
        typedef bp::class_< exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >, boost::noncopyable > GenericElementInt_exposer_t;
        GenericElementInt_exposer_t GenericElementInt_exposer = GenericElementInt_exposer_t( "GenericElementInt", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) );
        bp::scope GenericElementInt_scope( GenericElementInt_exposer );
        bp::implicitly_convertible< exfel::util::Schema &, exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > >();

        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::key

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*key_function_type )( ::std::string const & ) ;

            GenericElementInt_exposer.def(
                "key"
                , key_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::displayedName

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*displayedName_function_type )( ::std::string const & ) ;

            GenericElementInt_exposer.def(
                "displayedName"
                , displayedName_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::description

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*description_function_type )( ::std::string const & ) ;

            GenericElementInt_exposer.def(
                "description"
                , description_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::unitName

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*unitName_function_type )( ::std::string const & ) ;

            GenericElementInt_exposer.def(
                "unitName"
                , unitName_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::unitSymbol

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*unitSymbol_function_type )( ::std::string const & ) ;

            GenericElementInt_exposer.def(
                "unitSymbol"
                , unitSymbol_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::advanced

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*advanced_function_type )(  ) ;

            GenericElementInt_exposer.def(
                "advanced"
                , advanced_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::advanced )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::reconfigurable

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*reconfigurable_function_type )(  ) ;

            GenericElementInt_exposer.def(
                "reconfigurable"
                , reconfigurable_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::reconfigurable )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::readOnly

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*readOnly_function_type )(  ) ;

            GenericElementInt_exposer.def(
                "readOnly"
                , readOnly_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::readOnly )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::init

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*init_function_type )(  ) ;

            GenericElementInt_exposer.def(
                "init"
                , init_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::init )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::assignmentMandatory

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*assignmentMandatory_function_type )(  ) ;

            GenericElementInt_exposer.def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::assignmentMandatory )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::assignmentOptional

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::DefaultValue< exfel::util::SimpleElement< int >, int > & ( exported_class_t::*assignmentOptional_function_type )(  ) ;

            GenericElementInt_exposer.def(
                "assignmentOptional"
                , assignmentOptional_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::assignmentOptional )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::assignmentInternal

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::DefaultValue< exfel::util::SimpleElement< int >, int > & ( exported_class_t::*assignmentInternal_function_type )(  ) ;

            GenericElementInt_exposer.def(
                "assignmentInternal"
                , assignmentInternal_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::assignmentInternal )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::commit

            typedef exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > exported_class_t;
            typedef ::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > & ( exported_class_t::*commit_function_type )(  ) ;

            GenericElementInt_exposer.def(
                "commit"
                , commit_function_type( &::exfel::util::GenericElement< exfel::util::SimpleElement< int >, int >::commit )
                , bp::return_internal_reference<> () );

        }
    }
    { //::exfel::util::GenericElement< exfel::util::SimpleElement< unsigned int >, unsigned int >
        typedef SimpleElement< unsigned int > U;
        typedef GenericElement< U, unsigned int > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementUINT32_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, unsigned int > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, unsigned int > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementUINT32_exposer_t( "GenericElementSimpleElementUINT32", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
    { //::exfel::util::GenericElement< exfel::util::SimpleElement< long long >, long long >
        typedef SimpleElement< long long > U;
        typedef GenericElement< U, long long > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementINT64_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, long long > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, long long > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementINT64_exposer_t( "GenericElementSimpleElementINT64", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< unsigned long long >, unsigned long long >
        typedef SimpleElement< unsigned long long > U;
        typedef GenericElement< U, unsigned long long > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementUINT64_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, unsigned long long > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, unsigned long long > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementUINT64_exposer_t( "GenericElementSimpleElementUINT64", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< signed char >, signed char >
        typedef SimpleElement< signed char > U;
        typedef GenericElement< U, signed char > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementINT8_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, signed char > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, signed char > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementINT8_exposer_t( "GenericElementSimpleElementINT8", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< unsigned char >, unsigned char >
        typedef SimpleElement< unsigned char > U;
        typedef GenericElement< U, unsigned char > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementUINT8_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, unsigned char > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, unsigned char > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementUINT8_exposer_t( "GenericElementSimpleElementUINT8", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< signed short >, signed short >
        typedef SimpleElement< signed short > U;
        typedef GenericElement< U, signed short > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementINT16_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, signed short > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, signed short > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementINT16_exposer_t( "GenericElementSimpleElementINT16", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< unsigned short >, unsigned short >
        typedef SimpleElement< unsigned short > U;
        typedef GenericElement< U, unsigned short > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementUINT16_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, unsigned short > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, unsigned short > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementUINT16_exposer_t( "GenericElementSimpleElementUINT16", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< double >, double >
        typedef SimpleElement< double > U;
        typedef GenericElement< U, double > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementDOUBLE_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, double > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, double > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementDOUBLE_exposer_t( "GenericElementSimpleElementDOUBLE", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< std::string >, std::string >
        typedef SimpleElement< std::string > U;
        typedef GenericElement< U, std::string > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementSTRING_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::string > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::string > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementSTRING_exposer_t( "GenericElementSimpleElementSTRING", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< bool >, bool >
        typedef SimpleElement< bool > U;
        typedef GenericElement< U, bool > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementBOOL_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, bool > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, bool > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementBOOL_exposer_t( "GenericElementSimpleElementBOOL", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< float >, float >
        typedef SimpleElement< float > U;
        typedef GenericElement< U, float > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementFLOAT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, float > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, float > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementFLOAT_exposer_t( "GenericElementSimpleElementFLOAT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
     { //::exfel::util::GenericElement< exfel::util::SimpleElement< boost::filesystem::path >, boost::filesystem::path >
        typedef SimpleElement< boost::filesystem::path > U;
        typedef GenericElement< U, boost::filesystem::path > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementSimpleElementPATH_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, boost::filesystem::path > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, boost::filesystem::path > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
        typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementSimpleElementPATH_exposer_t( "GenericElementSimpleElementPATH", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
             .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
             .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
             .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
             .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
             .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
            .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
            .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
            .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
            .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
            ;
     }
    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    //GenerichElement<VectorElement ...

    { //::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >
        typedef bp::class_< exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >, boost::noncopyable > GenericElementVectorInt32_exposer_t;
        bp::implicitly_convertible< exfel::util::Schema &, exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > > >();
        typedef GenericElement< VectorElement< int, std::vector >, std::vector< int > > exported_class_t;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*advanced_function_type )(  ) ;
        typedef ::exfel::util::DefaultValue< exfel::util::VectorElement< int, std::vector >, std::vector< int > > & ( exported_class_t::*assignmentInternal_function_type )(  ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*assignmentMandatory_function_type )(  ) ;
        typedef ::exfel::util::DefaultValue< exfel::util::VectorElement< int, std::vector >, std::vector< int > > & ( exported_class_t::*assignmentOptional_function_type )(  ) ;
        typedef ::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > > & ( exported_class_t::*commit_function_type )(  ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*description_function_type )( ::std::string const & ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*unitName_function_type )( ::std::string const & ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*displayedName_function_type )( ::std::string const & ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*init_function_type )(  ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*key_function_type )( ::std::string const & ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*readOnly_function_type )(  ) ;
        typedef ::exfel::util::VectorElement< int, std::vector > & ( exported_class_t::*reconfigurable_function_type )(  ) ;

        GenericElementVectorInt32_exposer_t( "GenericElementVectorInt32", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &::exfel::util::GenericElement< exfel::util::VectorElement< int, std::vector >, std::vector< int > >::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< unsigned int, std::vector >, std::vector< unsigned int > >
        typedef VectorElement< unsigned int, std::vector > U;
        typedef GenericElement< U, std::vector< unsigned int > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorUInt32_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< unsigned int > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< unsigned int > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< unsigned int > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorUInt32_exposer_t( "GenericElementVectorUInt32", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< long long, std::vector >, std::vector< long long > >
        typedef VectorElement< long long, std::vector > U;
        typedef GenericElement< U, std::vector< long long > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorInt64_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< long long > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< long long > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< long long > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorInt64_exposer_t( "GenericElementVectorInt64", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< unsigned long long, std::vector >, std::vector< unsigned long long > >
        typedef VectorElement< unsigned long long, std::vector > U;
        typedef GenericElement< U, std::vector< unsigned long long > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorUInt64_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< unsigned long long > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< unsigned long long > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< unsigned long long > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorUInt64_exposer_t( "GenericElementVectorUInt64", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< std::string, std::vector >, std::vector< std::string > >
        typedef VectorElement< std::string, std::vector > U;
        typedef GenericElement< U, std::vector< std::string > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorSTRING_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< std::string > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< std::string > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< std::string > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorSTRING_exposer_t( "GenericElementVectorSTRING", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >
        typedef VectorElement<  bool, std::deque > U;
        typedef GenericElement< U, std::deque< bool > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorBOOL_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::deque< bool > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::deque< bool > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::deque< bool > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorBOOL_exposer_t( "GenericElementVectorBOOL", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< double, std::vector >, std::vector< double > >
        typedef VectorElement< double, std::vector > U;
        typedef GenericElement< U, std::vector< double > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorDOUBLE_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< double > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< double > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< double > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorDOUBLE_exposer_t( "GenericElementVectorDOUBLE", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< signed char, std::vector >, std::vector< signed char > >
        typedef VectorElement< signed char, std::vector > U;
        typedef GenericElement< U, std::vector< signed char > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorINT8_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< signed char > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< signed char > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< signed char > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorINT8_exposer_t( "GenericElementVectorINT8", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< unsigned char, std::vector >, std::vector< unsigned char > >
        typedef VectorElement< unsigned char, std::vector > U;
        typedef GenericElement< U, std::vector< unsigned char > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorUINT8_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< unsigned char > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< unsigned char > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< unsigned char > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorUINT8_exposer_t( "GenericElementVectorUINT8", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< signed short, std::vector >, std::vector< signed short > >
        typedef VectorElement< signed short, std::vector > U;
        typedef GenericElement< U, std::vector< signed short > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorINT16_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< signed short > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< signed short > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< signed short > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorINT16_exposer_t( "GenericElementVectorINT16", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< unsigned short, std::vector >, std::vector< unsigned short > >
        typedef VectorElement< unsigned short, std::vector > U;
        typedef GenericElement< U, std::vector< unsigned short > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorUINT16_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< unsigned short > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< unsigned short > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< unsigned short > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;

        GenericElementVectorUINT16_exposer_t( "GenericElementVectorUINT16", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< float, std::vector >, std::vector< float > >
        typedef VectorElement< float, std::vector > U;
        typedef GenericElement< U, std::vector< float > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorFLOAT_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< float > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< float > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< float > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementVectorFLOAT_exposer_t( "GenericElementVectorFLOAT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }
    { //::exfel::util::GenericElement< exfel::util::VectorElement< boost::filesystem::path, std::vector >, std::vector< boost::filesystem::path > >
        typedef VectorElement< boost::filesystem::path, std::vector > U;
        typedef GenericElement< U, std::vector< boost::filesystem::path > > T;
        typedef bp::class_< T, boost::noncopyable > GenericElementVectorPATH_exposer_t;
        bp::implicitly_convertible< Schema &, GenericElement< U, std::vector< boost::filesystem::path > > >();
        typedef U & ( T::*advanced_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< boost::filesystem::path > > & ( T::*assignmentInternal_function_type )(  ) ;
        typedef U & ( T::*assignmentMandatory_function_type )(  ) ;
        typedef DefaultValue< U, std::vector< boost::filesystem::path > > & ( T::*assignmentOptional_function_type )(  ) ;
        typedef T & ( T::*commit_function_type )(  ) ;
        typedef U & ( T::*description_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*unitSymbol_function_type )( ::std::string const & ) ;
        typedef U & ( T::*displayedName_function_type )( ::std::string const & ) ;
        typedef U & ( T::*init_function_type )(  ) ;
        typedef U & ( T::*key_function_type )( ::std::string const & ) ;
        typedef U & ( T::*readOnly_function_type )(  ) ;
        typedef U & ( T::*reconfigurable_function_type )(  ) ;
        GenericElementVectorPATH_exposer_t( "GenericElementVectorPATH", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "advanced"
                , advanced_function_type( &T::advanced )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentInternal"
                , assignmentInternal_function_type( &T::assignmentInternal )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentMandatory"
                , assignmentMandatory_function_type( &T::assignmentMandatory )
                , bp::return_internal_reference<> () )
              .def(
                "assignmentOptional"
                , assignmentOptional_function_type( &T::assignmentOptional )
                , bp::return_internal_reference<> () )
              .def(
                "commit"
                , commit_function_type( &T::commit )
                , bp::return_internal_reference<> () )
              .def(
                "description"
                , description_function_type( &T::description )
                , ( bp::arg("desc") )
                , bp::return_internal_reference<> () )
              .def(
                "unitName"
                , unitName_function_type( &T::unitName )
                , ( bp::arg("unitName") )
                , bp::return_internal_reference<> () )
              .def(
                "unitSymbol"
                , unitSymbol_function_type( &T::unitSymbol )
                , ( bp::arg("unitSymbol") )
                , bp::return_internal_reference<> () )
              .def(
                "displayedName"
                , displayedName_function_type( &T::displayedName )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "init"
                , init_function_type( &T::init )
                , bp::return_internal_reference<> () )
              .def(
                "key"
                , key_function_type( &T::key )
                , ( bp::arg("name") )
                , bp::return_internal_reference<> () )
              .def(
                "readOnly"
                , readOnly_function_type( &T::readOnly )
                , bp::return_internal_reference<> () )
              .def(
                "reconfigurable"
                , reconfigurable_function_type( &T::reconfigurable )
                , bp::return_internal_reference<> () )
              ;
    }

    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    //SimpleElement< ... >

    { //::exfel::util::SimpleElement< int >
        typedef bp::class_< exfel::util::SimpleElement< int >, bp::bases< exfel::util::GenericElement< exfel::util::SimpleElement< int >, int > >, boost::noncopyable > INT32_ELEMENT_exposer_t;
        INT32_ELEMENT_exposer_t INT32_ELEMENT_exposer = INT32_ELEMENT_exposer_t( "INT32_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) );
        bp::scope INT32_ELEMENT_scope( INT32_ELEMENT_exposer );
        bp::implicitly_convertible< exfel::util::Schema &, exfel::util::SimpleElement< int > >();
        { //::exfel::util::SimpleElement< int >::maxExc

            typedef exfel::util::SimpleElement< int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*maxExc_function_type )( int const & ) ;

            INT32_ELEMENT_exposer.def(
                "maxExc"
                , maxExc_function_type( &::exfel::util::SimpleElement< int >::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::SimpleElement< int >::maxInc

            typedef exfel::util::SimpleElement< int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*maxInc_function_type )( int const & ) ;

            INT32_ELEMENT_exposer.def(
                "maxInc"
                , maxInc_function_type( &::exfel::util::SimpleElement< int >::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::SimpleElement< int >::minExc

            typedef exfel::util::SimpleElement< int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*minExc_function_type )( int const & ) ;

            INT32_ELEMENT_exposer.def(
                "minExc"
                , minExc_function_type( &::exfel::util::SimpleElement< int >::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::SimpleElement< int >::minInc

            typedef exfel::util::SimpleElement< int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*minInc_function_type )( int const & ) ;

            INT32_ELEMENT_exposer.def(
                "minInc"
                , minInc_function_type( &::exfel::util::SimpleElement< int >::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::SimpleElement< int >::options

            typedef exfel::util::SimpleElement< int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*options_function_type )( ::std::string const &, ::std::string const & ) ;
            
            INT32_ELEMENT_exposer.def(
                "options"
                , options_function_type( &::exfel::util::SimpleElement< int >::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () );

        }
        { //::exfel::util::SimpleElement< int >::options

            typedef exfel::util::SimpleElement< int > exported_class_t;
            typedef ::exfel::util::SimpleElement< int > & ( exported_class_t::*options_function_type )( ::std::vector< std::string > const & ) ;

            INT32_ELEMENT_exposer.def(
                "options"
                , options_function_type( &::exfel::util::SimpleElement< int >::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
        }
   }
   { //::exfel::util::SimpleElement< unsigned int >
        typedef SimpleElement< unsigned int > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, unsigned int > >, boost::noncopyable > UINT32_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( unsigned int const & ) ;
        typedef T & ( T::*maxInc_function_type )( unsigned int const & ) ;
        typedef T & ( T::*minExc_function_type )( unsigned int const & ) ;
        typedef T & ( T::*minInc_function_type )( unsigned int const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const &,::std::string const &  ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        UINT32_ELEMENT_exposer_t( "UINT32_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
   }
   { //::exfel::util::SimpleElement< long long >   "INT64_ELEMENT"
        typedef SimpleElement< long long > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, long long > >, boost::noncopyable > INT64_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( long long const & ) ;
        typedef T & ( T::*maxInc_function_type )( long long const & ) ;
        typedef T & ( T::*minExc_function_type )( long long const & ) ;
        typedef T & ( T::*minInc_function_type )( long long const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        INT64_ELEMENT_exposer_t( "INT64_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
    }
   { //::exfel::util::SimpleElement< unsigned long long >   "UINT64_ELEMENT"
        typedef SimpleElement< unsigned long long > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, unsigned long long > >, boost::noncopyable > UINT64_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( unsigned long long const & ) ;
        typedef T & ( T::*maxInc_function_type )( unsigned long long const & ) ;
        typedef T & ( T::*minExc_function_type )( unsigned long long const & ) ;
        typedef T & ( T::*minInc_function_type )( unsigned long long const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        UINT64_ELEMENT_exposer_t( "UINT64_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
   }
    { //::exfel::util::SimpleElement< signed char >   "INT8_ELEMENT"
        typedef SimpleElement< signed char > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, signed char > >, boost::noncopyable > INT8_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( signed char const & ) ;
        typedef T & ( T::*maxInc_function_type )( signed char const & ) ;
        typedef T & ( T::*minExc_function_type )( signed char const & ) ;
        typedef T & ( T::*minInc_function_type )( signed char const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        INT8_ELEMENT_exposer_t( "INT8_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
   }
   { //::exfel::util::SimpleElement< unsigned char >   "UINT8_ELEMENT"
        typedef SimpleElement< unsigned char > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, unsigned char > >, boost::noncopyable > UINT8_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( unsigned char const & ) ;
        typedef T & ( T::*maxInc_function_type )( unsigned char const & ) ;
        typedef T & ( T::*minExc_function_type )( unsigned char const & ) ;
        typedef T & ( T::*minInc_function_type )( unsigned char const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        UINT8_ELEMENT_exposer_t( "UINT8_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
   }
   { //::exfel::util::SimpleElement< signed short >   "INT16_ELEMENT"
        typedef SimpleElement< signed short > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, signed short > >, boost::noncopyable > INT16_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( signed short const & ) ;
        typedef T & ( T::*maxInc_function_type )( signed short const & ) ;
        typedef T & ( T::*minExc_function_type )( signed short const & ) ;
        typedef T & ( T::*minInc_function_type )( signed short const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const & , ::std::string const &) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        INT16_ELEMENT_exposer_t( "INT16_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
   }
   { //::exfel::util::SimpleElement< unsigned short >   "UINT16_ELEMENT"
        typedef SimpleElement< unsigned short > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, unsigned short > >, boost::noncopyable > UINT16_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( unsigned short const & ) ;
        typedef T & ( T::*maxInc_function_type )( unsigned short const & ) ;
        typedef T & ( T::*minExc_function_type )( unsigned short const & ) ;
        typedef T & ( T::*minInc_function_type )( unsigned short const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        UINT16_ELEMENT_exposer_t( "UINT16_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
   }
   { //::exfel::util::SimpleElement< double >   "DOUBLE_ELEMENT"
        typedef SimpleElement< double > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, double > >, boost::noncopyable > DOUBLE_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( double const & ) ;
        typedef T & ( T::*maxInc_function_type )( double const & ) ;
        typedef T & ( T::*minExc_function_type )( double const & ) ;
        typedef T & ( T::*minInc_function_type )( double const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        DOUBLE_ELEMENT_exposer_t( "DOUBLE_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
   }
   { //::exfel::util::SimpleElement< float >   "FLOAT_ELEMENT"
        typedef SimpleElement< float > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, float > >, boost::noncopyable > FLOAT_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( float const & ) ;
        typedef T & ( T::*maxInc_function_type )( float const & ) ;
        typedef T & ( T::*minExc_function_type )( float const & ) ;
        typedef T & ( T::*minInc_function_type )( float const & ) ;
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        FLOAT_ELEMENT_exposer_t( "FLOAT_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
              .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
              .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , ( bp::arg("val") )
                , bp::return_internal_reference<> () )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
   }
   { //::exfel::util::SimpleElement< std::string >   "STRING_ELEMENT"
        typedef SimpleElement< std::string > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::string > >, boost::noncopyable > STRING_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        STRING_ELEMENT_exposer_t( "STRING_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
    }
    { //::exfel::util::SimpleElement< bool >   "BOOL_ELEMENT"
        typedef SimpleElement< bool > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, bool > >, boost::noncopyable > BOOL_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        BOOL_ELEMENT_exposer_t( "BOOL_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
    }
    { //::exfel::util::SimpleElement< boost::filesystem::path >   "PATH_ELEMENT"
        typedef SimpleElement< boost::filesystem::path > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, boost::filesystem::path > >, boost::noncopyable > PATH_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*options_function_type1 )( ::std::string const &, ::std::string const & ) ;
        typedef T & ( T::*options_function_type2 )( ::std::vector< std::string > const & ) ;
        PATH_ELEMENT_exposer_t( "PATH_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )
             .def(
                "options"
                , options_function_type1( &T::options )
                , ( bp::arg("opts"), bp::arg("sep") )
                , bp::return_internal_reference<> () )
            .def(
                "options"
                , options_function_type2( &T::options )
                , ( bp::arg("opts") )
                , bp::return_internal_reference<> () );
    }

   //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
   // Exposing VECTOR_..._ELEMENT to Python :
   //"VECTOR_INT32_ELEMENT", "VECTOR_UINT32_ELEMENT", "VECTOR_INT64_ELEMENT", "VECTOR_UINT64_ELEMENT",
   //"VECTOR_STRING_ELEMENT", "VECTOR_BOOL_ELEMENT", "VECTOR_INT8_ELEMENT", "VECTOR_UINT8_ELEMENT"
   { //::exfel::util::VectorElement< int, std::vector >  "VECTOR_INT32_ELEMENT"
        typedef bp::class_< VectorElement< int, std::vector >, bp::bases< GenericElement< VectorElement< int, std::vector >, std::vector< int > > >, boost::noncopyable > VECTOR_INT32_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, VectorElement< int, std::vector > >();
        typedef exfel::util::VectorElement< int, std::vector > exported_class_t;
        typedef VectorElement< int, std::vector > & ( exported_class_t::*maxExc_function_type )( int const & ) ;
        typedef VectorElement< int, std::vector > & ( exported_class_t::*maxInc_function_type )( int const & ) ;
        typedef VectorElement< int, std::vector > & ( exported_class_t::*maxSize_function_type )( int const & ) ;
        typedef VectorElement< int, std::vector > & ( exported_class_t::*minExc_function_type )( int const & ) ;
        typedef VectorElement< int, std::vector > & ( exported_class_t::*minInc_function_type )( int const & ) ;
        typedef VectorElement< int, std::vector > & ( exported_class_t::*minSize_function_type )( int const & ) ;
        VECTOR_INT32_ELEMENT_exposer_t( "VECTOR_INT32_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &VectorElement< int, std::vector >::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &VectorElement< int, std::vector >::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &VectorElement< int, std::vector >::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &VectorElement< int, std::vector >::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &VectorElement< int, std::vector >::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &VectorElement< int, std::vector >::minSize )
                , bp::return_internal_reference<> () )
           ;
    }
    { //::exfel::util::VectorElement< unsigned int, std::vector > "VECTOR_UINT32_ELEMENT"
        typedef VectorElement< unsigned int, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< unsigned int > > >, boost::noncopyable > VECTOR_UINT32_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( unsigned int const & ) ;
        typedef T & ( T::*maxInc_function_type )( unsigned int const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( unsigned int const & ) ;
        typedef T & ( T::*minInc_function_type )( unsigned int const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_UINT32_ELEMENT_exposer_t( "VECTOR_UINT32_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
    }
   { //::exfel::util::VectorElement< long long, std::vector > "VECTOR_INT64_ELEMENT"
        typedef VectorElement< long long, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< long long > > >, boost::noncopyable > VECTOR_INT64_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( long long const & ) ;
        typedef T & ( T::*maxInc_function_type )( long long const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( long long const & ) ;
        typedef T & ( T::*minInc_function_type )( long long const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_INT64_ELEMENT_exposer_t( "VECTOR_INT64_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< unsigned long long, std::vector > "VECTOR_UINT64_ELEMENT"
        typedef VectorElement< unsigned long long, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< unsigned long long > > >, boost::noncopyable > VECTOR_UINT64_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( unsigned long long const & ) ;
        typedef T & ( T::*maxInc_function_type )( unsigned long long const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( unsigned long long const & ) ;
        typedef T & ( T::*minInc_function_type )( unsigned long long const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_UINT64_ELEMENT_exposer_t( "VECTOR_UINT64_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< std::string, std::vector > "VECTOR_STRING_ELEMENT"
        typedef VectorElement< std::string, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< std::string > > >, boost::noncopyable > VECTOR_STRING_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_STRING_ELEMENT_exposer_t( "VECTOR_STRING_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< bool, std::deque > "VECTOR_BOOL_ELEMENT"
        typedef VectorElement< bool, std::deque > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::deque< bool > > >, boost::noncopyable > VECTOR_BOOL_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_BOOL_ELEMENT_exposer_t( "VECTOR_BOOL_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< double, std::vector > "VECTOR_DOUBLE_ELEMENT"
        typedef VectorElement< double, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< double > > >, boost::noncopyable > VECTOR_DOUBLE_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( double const & ) ;
        typedef T & ( T::*maxInc_function_type )( double const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( double const & ) ;
        typedef T & ( T::*minInc_function_type )( double const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_DOUBLE_ELEMENT_exposer_t( "VECTOR_DOUBLE_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< signed char, std::vector > "VECTOR_INT8_ELEMENT"
        typedef VectorElement< signed char, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< signed char > > >, boost::noncopyable > VECTOR_INT8_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( signed char const & ) ;
        typedef T & ( T::*maxInc_function_type )( signed char const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( signed char const & ) ;
        typedef T & ( T::*minInc_function_type )( signed char const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_INT8_ELEMENT_exposer_t( "VECTOR_INT8_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< unsigned char, std::vector > "VECTOR_UINT8_ELEMENT"
        typedef VectorElement< unsigned char, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< unsigned char > > >, boost::noncopyable > VECTOR_UINT8_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( unsigned char const & ) ;
        typedef T & ( T::*maxInc_function_type )( unsigned char const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( unsigned char const & ) ;
        typedef T & ( T::*minInc_function_type )( unsigned char const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_UINT8_ELEMENT_exposer_t( "VECTOR_UINT8_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< signed short, std::vector > "VECTOR_INT16_ELEMENT"
        typedef VectorElement< signed short, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< signed short > > >, boost::noncopyable > VECTOR_INT16_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( signed short const & ) ;
        typedef T & ( T::*maxInc_function_type )( signed short const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( signed short const & ) ;
        typedef T & ( T::*minInc_function_type )( signed short const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_INT16_ELEMENT_exposer_t( "VECTOR_INT16_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< unsigned short, std::vector > "VECTOR_UINT16_ELEMENT"
        typedef VectorElement< unsigned short, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< unsigned short > > >, boost::noncopyable > VECTOR_UINT16_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( unsigned short const & ) ;
        typedef T & ( T::*maxInc_function_type )( unsigned short const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( unsigned short const & ) ;
        typedef T & ( T::*minInc_function_type )( unsigned short const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_UINT16_ELEMENT_exposer_t( "VECTOR_UINT16_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< float, std::vector > "VECTOR_FLOAT_ELEMENT"
        typedef VectorElement< float, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< float > > >, boost::noncopyable > VECTOR_FLOAT_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxExc_function_type )( float const & ) ;
        typedef T & ( T::*maxInc_function_type )( float const & ) ;
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minExc_function_type )( float const & ) ;
        typedef T & ( T::*minInc_function_type )( float const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_FLOAT_ELEMENT_exposer_t( "VECTOR_FLOAT_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxExc"
                , maxExc_function_type( &T::maxExc )
                , bp::return_internal_reference<> () )
            .def(
                "maxInc"
                , maxInc_function_type( &T::maxInc )
                , bp::return_internal_reference<> () )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minExc"
                , minExc_function_type( &T::minExc )
                , bp::return_internal_reference<> () )
            .def(
                "minInc"
                , minInc_function_type( &T::minInc )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }
      { //::exfel::util::VectorElement< boost::filesystem::path, std::vector > "VECTOR_PATH_ELEMENT"
        typedef VectorElement< boost::filesystem::path, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< boost::filesystem::path > > >, boost::noncopyable > VECTOR_PATH_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T >();
        typedef T & ( T::*maxSize_function_type )( int const & ) ;
        typedef T & ( T::*minSize_function_type )( int const & ) ;
        VECTOR_PATH_ELEMENT_exposer_t( "VECTOR_PATH_ELEMENT", bp::init< exfel::util::Schema & >(( bp::arg("expected") )) )
            .def(
                "maxSize"
                , maxSize_function_type( &T::maxSize )
                , bp::return_internal_reference<> () )
            .def(
                "minSize"
                , minSize_function_type( &T::minSize )
                , bp::return_internal_reference<> () )
           ;
      }

}//end  exportPyUtilConfig

