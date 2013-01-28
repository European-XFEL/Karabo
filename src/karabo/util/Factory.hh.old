/*
 * $Id: Factory.hh 7154 2012-09-06 11:17:38Z heisenb $
 *
 * File:   Factory.hh
 * Author: <krzysztof.wrona@xfel.eu> and <burkhard.heisen@xfel.eu> and <nicola.coppola@xfel.eu>
 *
 * Created on August 15, 2010, 5:41 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_FACTORY_HH
#define	KARABO_UTIL_FACTORY_HH


#include <boost/shared_ptr.hpp>

#include "GenericFactory.hh"
#include "Hash.hh"
//#include "Schema.hh"
//#include "SimpleElement.hh"
//#include "VectorElement.hh"
//#include "SingleElement.hh"
//#include "ChoiceElement.hh"
//#include "ListElement.hh"
//#include "TargetActualElement.hh"
//#include "MonitorableElement.hh"
//#include "OverwriteElement.hh"
//#include "ImageElement.hh"
//#include "ConfigConstants.hh"
#include "Traits.hh"
#include "ClassInfo.hh"



namespace karabo {
    namespace util {

        /**
         * Factory<BaseType> creates objects complying with a BaseType interface. The BaseType interface may have several
         * implementations (derived classes). The actual type of the created object can be selected using a string based
         * configuration parameter. This gives a developer the flexibility to choose the implementation class without any code
         * modifications. Client code is not exposed to the details of implementations, but rather deals only with the well
         * defined interface. In fact, not only the implementation class can be chosen using the string parameter, but also
         * the implementation class properties are configurable. The Factory utilizes the Hash object for that purpose.
         * It also defines a concept of expected parameters, which describe all necessary parameters for the given object.
         * Furthermore, when it is given the Hash object it can assert supplied parameters and throws runtime exception
         * if the Hash object does not match the expected one (for example, if some mandatory parameters are missing).
         * 
         * The Factory template uses the GenericFactory class to register appropriate concrete factory. In order to make
         * the registration mechanism easy to use in the client code several macros are defined.
         *
         * The Factory class is typically not used directly in the client code, but rather the class which defines
         * the interface should define static methods by including an appropriate macro (KARABO_FACTORY_BASE_CLASS)
         */
        template< class BaseType>
        class Factory {
            typedef boost::shared_ptr<BaseType> BaseTypePointer;

        public:

            virtual ~Factory() {
            }

            /**
             * The factory creates and configures objects using this method. Note, that the client should
             * use static methods "create", which internally use this one.
             * @param input configuration object
             * @param classId registration key
             * @param assert if true configuration object is validated
             */
            BaseTypePointer makeObject(const Hash& user, const std::string& classId, bool assert) {
                if (assert) {
                    karabo::util::Schema master;
                    Schema& tmp = master.initParameterDescription(classId);
                    expectedParameters(tmp); // This changes master
                    Hash working = master.validate(user);
                    return makeConfiguredObject(working.get<Hash > (classId));
                } else {
                    return makeConfiguredObject(user.get<Hash > (classId));
                }
            }

            /**
             *  Creates object of a class registered with classId. This method uses parameterless constructor. Note that this
             *  variant does not allow for setting any class parameters. If this is required use other variant of create method.
             *  @param classId registration key for concrete class implementation
             *  @return boost shared pointer to BaseType
             */
            static boost::shared_ptr< BaseType > createDefault(const std::string& classId) {
                boost::shared_ptr<karabo::util::Factory<BaseType> > concreteMaker =
                        karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance().create(classId);
                return concreteMaker->makeDefaultConstructedObject();
            }

            /**
             *  Creates object using configuration parameters. The configuration object must be defined according
             *  to the expected parameters. The object is created and configured.
             *  @param conf configuration needed to instantiate the object
             *  @param assert if set to true config is validated against expected parameters
             *  @return boost shared pointer to BaseType
             */
            static boost::shared_ptr< BaseType > create(const karabo::util::Hash& config, bool assert = true) {
                if (config.empty()) throw KARABO_PARAMETER_EXCEPTION("Empty configuration object handed to factory");
                const std::string & classId = config.begin()->first; // Root key encodes classId for factory
                boost::shared_ptr<karabo::util::Factory<BaseType> > concreteMaker =
                        karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance().create(classId);
                return concreteMaker->makeObject(config, classId, assert);
            }
            
            static boost::shared_ptr< BaseType > create(const std::string& classId, const karabo::util::Hash& params, bool assert = true) {
                boost::shared_ptr<karabo::util::Factory<BaseType> > concreteMaker =
                        karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance().create(classId);
                Hash config(classId, params);
                return concreteMaker->makeObject(config, classId, assert);
            }

            /**
             * Creates and configures object using part of the Hash tree referenced by "key".
             */
            static boost::shared_ptr< BaseType > createChoice(const std::string& key, const karabo::util::Hash& input, bool assert = true) {
                karabo::util::GenericFactory<karabo::util::Factory<BaseType> >& gf = karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance();
                std::string classId;
                if (gf.has(key)) {
                    classId = key;
                    boost::shared_ptr<karabo::util::Factory<BaseType> > concreteMaker =
                            karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance().create(classId);
                    return concreteMaker->makeObject(Hash(classId, input.get<Hash > (key)), classId, assert);
                } else {
                    if (!input.has(key)) {
                        throw KARABO_PARAMETER_EXCEPTION("Configuration object to non-factory key \"" + key + "\" does not contain sufficient information to be manufactured");
                    } else {
                        const Hash& createInfo = input.get<Hash > (key);
                        classId = createInfo.begin()->first;
                        boost::shared_ptr<karabo::util::Factory<BaseType> > concreteMaker =
                                karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance().create(classId);
                        return concreteMaker->makeObject(createInfo, classId, assert);
                    }
                }
            }

            static boost::shared_ptr< BaseType > createSingle(const std::string& key, const std::string& classId, const karabo::util::Hash& input, bool assert = true) {
                boost::shared_ptr<karabo::util::Factory<BaseType> > concreteMaker =
                        karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance().create(classId);
                return concreteMaker->makeObject(Hash(classId, input.get<karabo::util::Hash > (key)), classId, assert);
            }

            static std::vector<boost::shared_ptr<BaseType> > createList(const std::string& key, const karabo::util::Hash& input, bool assert = true) {
                const std::vector<karabo::util::Hash>& configs = input.get < std::vector<karabo::util::Hash> > (key);
                std::vector<boost::shared_ptr<BaseType> > objects(configs.size());
                for (size_t i = 0; i < configs.size(); ++i) {
                    objects[i] = create(configs[i], assert);
                }
                return objects;
            }

            /**
             *  Creates factory for objects of class registered with classId. Having the factory you can
             *  call directly the make method. This can be useful if you need to create many objects of the same
             *  concrete type with different configuration parameters as you can reuse the same factory. However, if the
             *  performance is the only argument we recommend you to evaluate the performance gain before you decide to
             *  use it.
             *  @param classId concrete class name
             *  @return shared pointer to the factory for BaseType objects
             */
            static boost::shared_ptr<karabo::util::Factory<BaseType> > createFactoryFor(const std::string& classId) {
                return karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance().create(classId);
            }

            /**
             * Provides description of expected parameters for an implementation class registered with the key classId.
             * Access type describes which parameters are included: initial, reconfigurable or monitorable.
             * displayedClassId can be used only with single element factory to specify different configuration key.
             * @param classId key registered in the factory
             * @param at access type
             * @param displayedClassId defines configuration key for single element factory
             */
            static Schema expectedParameters(const std::string& classId, karabo::util::AccessType at, const std::string& currentState, const std::string& displayedClassId) {
                std::string key(classId);
                if (!displayedClassId.empty()) key = displayedClassId;
                Schema expected;
                Schema& tmp = expected.initParameterDescription(key, at, currentState);
                createFactoryFor(classId)->expectedParameters(tmp);
                return expected;
            }

            /**
             * Provides description of expected parameters for all implementation classes registered
             * in this factory for a given access type.
             * @param at access type        
             * @return description of expected parameters
             */
            static Schema expectedParameters(karabo::util::AccessType at, const std::string& currentState) {
                Schema schema;
                std::vector<std::string> keys = getRegisteredKeys();
                for (size_t i = 0; i < keys.size(); ++i) {
                    schema.set(keys[i], expectedParameters(keys[i], at, currentState, ""));
                }
                return schema;
            }


            /**
             *  Each BaseType class may use factory for objects creation. Concrete factories should be registered
             *  in the BaseType class specific registry.
             *  This method returns keys for all registered concrete factories for a specific BasType.
             *
             */
            static std::vector<std::string> getRegisteredKeys() {
                return karabo::util::GenericFactory<karabo::util::Factory<BaseType> >::getInstance().getKeysAsVector();
            }

        protected:

            /**
             * Defines expected parameters
             */
            virtual void expectedParameters(karabo::util::Schema& expected) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("The object to be created DOES NOT implement the expectedParameters functions, you may only default construct this object");
            }

            /**
             * Creates object without any attempt to configure it
             */
            virtual BaseTypePointer makeDefaultConstructedObject() {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("The object to be created implements the configure and expectedParameters functions, default/zero construction is thus unsupported");
            }

            /**
             * Creates object and configure it according to delivered config
             */
            virtual BaseTypePointer makeConfiguredObject(const Hash& param) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("The object to be created DOES NOT implement the configure and expectedParameters functions, thus configuration-based creation is unsuported");
            }

        };

        /**
         * Template class for concrete factory. ConcreteFactory creates objects of classes
         * which inherit from BaseClass. It fully implements Factory<BaseClass> interface and passes the responsibility
         * to define expectedParameters and configure object to the Base and Derived classes.
         * Derived class must always have a default constructor (without arguments).
         * In addition it must have two methods implemented: <code>static void expectedParameters(Schema&)</code> and
         * <code>void configure(const Hash&)</code>. Due to the following template specialization implementing
         * SFINAE pattern the base class may implement both methods or skip them (either both implemented or not).
         * If implemented they are also called by this factory. Base class may do it to define common parameters needed
         * for all derived classes.
         * The client code will indirectly use this template class by calling REGISTER_FACTORY_CC macro
         */
        template<class BaseClass, class DerivedClass, bool configureBase, bool configureDerived>
        class ConcreteFactory : public Factory<BaseClass> {
        private:
            // used for self registering
            static const karabo::util::RegisterInFactory< karabo::util::Factory<BaseClass >,
            ConcreteFactory<BaseClass, DerivedClass, configureBase, configureDerived >, DerivedClass > registerMe;

        protected:

            boost::shared_ptr<BaseClass > makeDefaultConstructedObject() {
                return boost::shared_ptr<BaseClass > (new DerivedClass());
            }
        };
        
        /**
         *  Partial specialization of ConcreteFactory template, making use of SFINAE pattern.
         *  It is used when BaseClass and DerivedClass implement expectedParameters and configure methods.
         */
        template<class BaseClass, class DerivedClass>
        class ConcreteFactory<BaseClass, DerivedClass, false, true > : public Factory<BaseClass> {
        private:
            // used for self registering
            static const karabo::util::RegisterInFactory< karabo::util::Factory<BaseClass >,
            ConcreteFactory<BaseClass, DerivedClass, false, true >, DerivedClass > registerMe;

        public:

            void expectedParameters(karabo::util::Schema& expected) {
                DerivedClass::expectedParameters(expected);
            }

        protected:

            boost::shared_ptr<BaseClass > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<DerivedClass> derivedClassPointer(new DerivedClass());
                boost::shared_ptr<BaseClass> baseClassPointer = derivedClassPointer;
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
         /**
         *  Partial specialization of ConcreteFactory template, making use of SFINAE pattern.
         *  It is used when BaseClass and DerivedClass implement expectedParameters and configure methods.
         */
        template<class BaseClass, class DerivedClass>
        class ConcreteFactory<BaseClass, DerivedClass, true, false > : public Factory<BaseClass> {
        private:
            // used for self registering
            static const karabo::util::RegisterInFactory< karabo::util::Factory<BaseClass >,
            ConcreteFactory<BaseClass, DerivedClass, true, false >, DerivedClass > registerMe;

        public:

            void expectedParameters(karabo::util::Schema& expected) {
                BaseClass::expectedParameters(expected);
            }

        protected:

            boost::shared_ptr<BaseClass > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<DerivedClass> derivedClassPointer(new DerivedClass());
                boost::shared_ptr<BaseClass> baseClassPointer = derivedClassPointer;
                baseClassPointer->BaseClass::configure(conf);
                return baseClassPointer;
            }
        };

        /**
         *  Partial specialization of ConcreteFactory template, making use of SFINAE pattern.
         *  It is used when BaseClass and DerivedClass implement expectedParameters and configure methods.
         */
        template<class BaseClass, class DerivedClass>
        class ConcreteFactory<BaseClass, DerivedClass, true, true > : public Factory<BaseClass> {
        private:
            // used for self registering
            static const karabo::util::RegisterInFactory< karabo::util::Factory<BaseClass >,
            ConcreteFactory<BaseClass, DerivedClass, true, true >, DerivedClass > registerMe;

        public:

            void expectedParameters(karabo::util::Schema& expected) {
                BaseClass::expectedParameters(expected);
                DerivedClass::expectedParameters(expected);
            }

        protected:

            boost::shared_ptr<BaseClass > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<DerivedClass> derivedClassPointer(new DerivedClass());
                boost::shared_ptr<BaseClass> baseClassPointer = derivedClassPointer;
                baseClassPointer->BaseClass::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };


        /**************************************************************/
        /*              Three Levels of Inheritance                   */

        /**************************************************************/

        template<class Base, class Middle, class Derived, bool configureBase, bool configureMiddle, bool configureDerived>
        class ConcreteFactoryThirdLevel : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryThirdLevel<Base, Middle, Derived, configureBase, configureMiddle, configureDerived >, Derived > registerMe;

        protected:

            boost::shared_ptr<Base > makeDefaultConstructedObject() {
                return boost::shared_ptr<Base > (new Derived());
            }
        };

        template<class Base, class Middle, class Derived>
        class ConcreteFactoryThirdLevel<Base, Middle, Derived, false, false, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryThirdLevel<Base, Middle, Derived, false, false, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Middle, class Derived>
        class ConcreteFactoryThirdLevel<Base, Middle, Derived, false, true, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryThirdLevel<Base, Middle, Derived, false, true, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Middle::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Middle> middleClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                middleClassPointer->Middle::configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Middle, class Derived>
        class ConcreteFactoryThirdLevel<Base, Middle, Derived, true, false, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryThirdLevel<Base, Middle, Derived, true, false, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Middle, class Derived>
        class ConcreteFactoryThirdLevel<Base, Middle, Derived, false, true, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryThirdLevel<Base, Middle, Derived, false, true, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Middle::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Middle> middleClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                middleClassPointer->Middle::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Middle, class Derived>
        class ConcreteFactoryThirdLevel<Base, Middle, Derived, true, false, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryThirdLevel<Base, Middle, Derived, true, false, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Middle, class Derived>
        class ConcreteFactoryThirdLevel<Base, Middle, Derived, true, true, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryThirdLevel<Base, Middle, Derived, true, true, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                Middle::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Middle> middleClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                middleClassPointer->Middle::configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Middle, class Derived>
        class ConcreteFactoryThirdLevel<Base, Middle, Derived, true, true, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryThirdLevel<Base, Middle, Derived, true, true, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                Middle::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Middle> middleClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                middleClassPointer->Middle::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        
        /**************************************************************/
        /*              Four Levels of Inheritance                   */

        /**************************************************************/
        template<class Base, class Second, class Third, class Derived, bool configureBase, bool configureSecond, bool configureThird, bool configureDerived>
        class ConcreteFactoryFourthLevel : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, configureBase, configureSecond, configureThird, configureDerived >, Derived > registerMe;

        protected:

            boost::shared_ptr<Base > makeDefaultConstructedObject() {
                return boost::shared_ptr<Base > (new Derived());
            }
        };
        //order 0 true, 1 true, 2 true, 3 true
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, false, false, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, false, false, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, false, true, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, false, true, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                //Base::expectedParameters(expected);
                //Second::expectedParameters(expected);
                Third::expectedParameters(expected);
                //Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                //boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                //baseClassPointer->Base::configure(conf);
                //secondClassPointer->Second::configure(conf);
                thirdClassPointer->Third::configure(conf);
                //derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, true, false, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, true, false, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                //Base::expectedParameters(expected);
                Second::expectedParameters(expected);
                //Third::expectedParameters(expected);
                //Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                //boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                //baseClassPointer->Base::configure(conf);
                secondClassPointer->Second::configure(conf);
                //thirdClassPointer->Third::configure(conf);
                //derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, false, false, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, false, false, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                //Second::expectedParameters(expected);
                //Third::expectedParameters(expected);
                //Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                //boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                //boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                //secondClassPointer->Second::configure(conf);
                //thirdClassPointer->Third::configure(conf);
                //derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, false, true, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, false, true, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                //Base::expectedParameters(expected);
                //Second::expectedParameters(expected);
                Third::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                //boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                //baseClassPointer->Base::configure(conf);
                //secondClassPointer->Second::configure(conf);
                thirdClassPointer->Third::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, true, false, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, true, false, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                //Base::expectedParameters(expected);
                Second::expectedParameters(expected);
                //Third::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                //boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                //baseClassPointer->Base::configure(conf);
                secondClassPointer->Second::configure(conf);
                //thirdClassPointer->Third::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, false, false, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, false, false, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                //Second::expectedParameters(expected);
                //Third::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                //boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                //boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                //secondClassPointer->Second::configure(conf);
                //thirdClassPointer->Third::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, true, true, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, true, true, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                //Base::expectedParameters(expected);
                Second::expectedParameters(expected);
                Third::expectedParameters(expected);
                //Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                //baseClassPointer->Base::configure(conf);
                secondClassPointer->Second::configure(conf);
                thirdClassPointer->Third::configure(conf);
                //derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, false, true, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, false, true, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                //Second::expectedParameters(expected);
                Third::expectedParameters(expected);
                //Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                //boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                //secondClassPointer->Second::configure(conf);
                thirdClassPointer->Third::configure(conf);
                //derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, true, false, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, true, false, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                Second::expectedParameters(expected);
                //Third::expectedParameters(expected);
                //Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                //boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                secondClassPointer->Second::configure(conf);
                //thirdClassPointer->Third::configure(conf);
                //derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, true, true, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, false, true, true, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                //Base::expectedParameters(expected);
                Second::expectedParameters(expected);
                Third::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                //baseClassPointer->Base::configure(conf);
                secondClassPointer->Second::configure(conf);
                thirdClassPointer->Third::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, false, true, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, false, true, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                //Second::expectedParameters(expected);
                Third::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                //boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                //secondClassPointer->Second::configure(conf);
                thirdClassPointer->Third::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, true, false, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, true, false, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                Second::expectedParameters(expected);
                //Third::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                //boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                secondClassPointer->Second::configure(conf);
                //thirdClassPointer->Third::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };
        
        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, true, true, false > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, true, true, false >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                Second::expectedParameters(expected);
                Third::expectedParameters(expected);
                //Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                secondClassPointer->Second::configure(conf);
                thirdClassPointer->Third::configure(conf);
                //derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        template<class Base, class Second, class Third, class Derived>
        class ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, true, true, true > : public Factory<Base> {
            static const karabo::util::RegisterInFactory< karabo::util::Factory<Base >,
            ConcreteFactoryFourthLevel<Base, Second, Third, Derived, true, true, true, true >, Derived > registerMe;

            void expectedParameters(karabo::util::Schema& expected) {
                Base::expectedParameters(expected);
                Second::expectedParameters(expected);
                Third::expectedParameters(expected);
                Derived::expectedParameters(expected);
            }
        protected:

            boost::shared_ptr<Base > makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<Derived> derivedClassPointer(new Derived());
                boost::shared_ptr<Second> secondClassPointer = derivedClassPointer;
                boost::shared_ptr<Third> thirdClassPointer = derivedClassPointer;
                boost::shared_ptr<Base> baseClassPointer = derivedClassPointer;
                baseClassPointer->Base::configure(conf);
                secondClassPointer->Second::configure(conf);
                thirdClassPointer->Third::configure(conf);
                derivedClassPointer->configure(conf);
                return baseClassPointer;
            }
        };

        
        /**
         * Template class for a configurable object. This template is useful to configure individual objects which do not
         * follow base-derived class design. Most frequently you will use this template for objects representing the root of a
         * configuration tree.
         * The client code will indirectly use this template class by calling REGISTER_ONLY_ME_CC macro
         */
        template<class ConfigureMe>
        class SingleClassFactory : public Factory<ConfigureMe> {
        private:
            // used for self registering
            static const karabo::util::RegisterInFactory< karabo::util::Factory<ConfigureMe >,
            SingleClassFactory<ConfigureMe>, ConfigureMe > registerMe;

        public:

            void expectedParameters(karabo::util::Schema& expected) {
                ConfigureMe::expectedParameters(expected);
            }

        protected:

            boost::shared_ptr<ConfigureMe> makeConfiguredObject(const Hash& conf) {
                boost::shared_ptr<ConfigureMe> configureMePointer = boost::shared_ptr<ConfigureMe > (new ConfigureMe());
                configureMePointer->configure(conf);
                return configureMePointer;
            }

            boost::shared_ptr<ConfigureMe > makeDefaultConstructedObject() {
                return boost::shared_ptr<ConfigureMe > (new ConfigureMe());
            }
        };

        
        
        // This macro should be called in header file, inside public section of the class declaration
        // but only for the base classes as used within Factory  (see tests/Vehicle.hh)
#define KARABO_FACTORY_BASE_CLASS \
typedef boost::shared_ptr<Self > Pointer;\
\
static boost::shared_ptr<Self > create(const karabo::util::Hash& config){\
return karabo::util::Factory< Self >::create(config); }\
\
static boost::shared_ptr<Self > create(const std::string& classId, const karabo::util::Hash& parameters = karabo::util::Hash()){\
return karabo::util::Factory< Self >::create(classId, parameters); }\
\
static boost::shared_ptr<Self > createDefault(const std::string& classId){\
return karabo::util::Factory< Self >::createDefault(classId); }\
\
static boost::shared_ptr< Self > createSingle(const std::string& key, const std::string& classId, const karabo::util::Hash& input){\
return karabo::util::Factory< Self >::createSingle(key, classId, input); }\
\
static boost::shared_ptr< Self > createChoice(const std::string& key, const karabo::util::Hash& input){\
return karabo::util::Factory< Self >::createChoice(key, input); }\
\
static std::vector<boost::shared_ptr< Self > > createList(const std::string& key, const karabo::util::Hash& input){\
return karabo::util::Factory< Self >::createList(key, input); }\
\
static karabo::util::Schema expectedParameters(karabo::util::AccessType at = karabo::util::INIT|karabo::util::WRITE, const std::string& currentState = ""){\
return karabo::util::Factory< Self >::expectedParameters(at, currentState); }\
\
static karabo::util::Schema expectedParameters(const std::string& classId, karabo::util::AccessType at = karabo::util::INIT|karabo::util::WRITE, const std::string& currentState = "", const std::string& displayedClassId = "") {\
return karabo::util::Factory< Self >::expectedParameters(classId, at, currentState, displayedClassId); }\
\
static karabo::util::Schema initialParameters(const std::string& classId, const std::string& currentState = "", const std::string& displayedClassId = ""){\
return karabo::util::Factory< Self >::expectedParameters(classId, karabo::util::INIT, currentState, displayedClassId);}\
\
static karabo::util::Schema initialParameters(){\
return karabo::util::Factory< Self >::expectedParameters(karabo::util::WRITE, ""); }\
\
static karabo::util::Schema monitorableParameters(const std::string& classId, const std::string& currentState = "", const std::string& displayedClassId = ""){\
return karabo::util::Factory< Self >::expectedParameters(classId, karabo::util::READ, currentState, displayedClassId);}\
\
static karabo::util::Schema monitorableParameters(){\
return karabo::util::Factory< Self >::expectedParameters(karabo::util::READ, ""); }\
\
static karabo::util::Schema reconfigurableParameters(const std::string& classId, const std::string& currentState = "", const std::string& displayedClassId = ""){\
return karabo::util::Factory< Self >::expectedParameters(classId, karabo::util::WRITE, currentState, displayedClassId); }\
\
static karabo::util::Schema reconfigurableParameters(){\
return karabo::util::Factory< Self >::expectedParameters(karabo::util::WRITE, ""); }\
\
static void help(const std::string& classId = ""){\
return karabo::util::Factory< Self >::expectedParameters(karabo::util::INIT|karabo::util::WRITE|karabo::util::READ, "").help(classId); }

#ifdef _WIN32
#define KARABO_REGISTER_FACTORY_BASE_HH(baseClass, templateExtern, declspecExtern) \
templateExtern template class declspecExtern \
karabo::util::GenericFactory<karabo::util::Factory<baseClass > >;
#else
#define KARABO_REGISTER_FACTORY_BASE_HH(baseClass, templateExtern, declspecExtern)
#endif

// This macro can be used only in the implementation files (*.cc) of derived class. It makes self registration of the
// derivedClass factory in the baseClass factory specific registry.
#define KARABO_REGISTER_FACTORY_CC(baseClass, derivedClass) \
    template<> \
    const karabo::util::RegisterInFactory< \
        karabo::util::Factory<baseClass>, \
        karabo::util::ConcreteFactory< \
            baseClass, \
            derivedClass, \
            karabo::util::ConfigureTraits<baseClass, karabo::util::Hash>::hasFunction, \
            karabo::util::ConfigureTraits<derivedClass, karabo::util::Hash>::hasFunction >, \
        derivedClass > \
    karabo::util::ConcreteFactory< \
        baseClass, \
        derivedClass, \
        karabo::util::ConfigureTraits<baseClass, karabo::util::Hash>::hasFunction, \
        karabo::util::ConfigureTraits<derivedClass, karabo::util::Hash>::hasFunction >::registerMe(1);


#define KARABO_REGISTER_FACTORY_2_CC(BaseClass, MiddleClass, DerivedClass) \
    template<> \
    const karabo::util::RegisterInFactory< \
        karabo::util::Factory<BaseClass>, \
        karabo::util::ConcreteFactoryThirdLevel< \
            BaseClass, \
            MiddleClass, \
            DerivedClass, \
            karabo::util::ConfigureTraits<BaseClass, karabo::util::Hash>::hasFunction, \
            karabo::util::ConfigureTraits<MiddleClass, karabo::util::Hash>::hasFunction, \
            karabo::util::ConfigureTraits<DerivedClass, karabo::util::Hash>::hasFunction >, \
        DerivedClass > \
    karabo::util::ConcreteFactoryThirdLevel< \
        BaseClass, \
        MiddleClass, \
        DerivedClass, \
        karabo::util::ConfigureTraits<BaseClass, karabo::util::Hash>::hasFunction, \
        karabo::util::ConfigureTraits<MiddleClass, karabo::util::Hash>::hasFunction, \
        karabo::util::ConfigureTraits<DerivedClass, karabo::util::Hash>::hasFunction >::registerMe(1);


#define KARABO_REGISTER_FACTORY_3_CC(BaseClass, SecondClass, ThirdClass, DerivedClass) \
    template<> \
    const karabo::util::RegisterInFactory< \
        karabo::util::Factory<BaseClass>, \
        karabo::util::ConcreteFactoryFourthLevel< \
            BaseClass, \
            SecondClass, \
            ThirdClass, \
            DerivedClass, \
            karabo::util::ConfigureTraits<BaseClass, karabo::util::Hash>::hasFunction, \
            karabo::util::ConfigureTraits<SecondClass, karabo::util::Hash>::hasFunction, \
            karabo::util::ConfigureTraits<ThirdClass, karabo::util::Hash>::hasFunction, \
            karabo::util::ConfigureTraits<DerivedClass, karabo::util::Hash>::hasFunction >, \
        DerivedClass > \
    karabo::util::ConcreteFactoryFourthLevel< \
        BaseClass, \
        SecondClass, \
        ThirdClass, \
        DerivedClass, \
        karabo::util::ConfigureTraits<BaseClass, karabo::util::Hash>::hasFunction, \
        karabo::util::ConfigureTraits<SecondClass, karabo::util::Hash>::hasFunction, \
        karabo::util::ConfigureTraits<ThirdClass, karabo::util::Hash>::hasFunction, \
        karabo::util::ConfigureTraits<DerivedClass, karabo::util::Hash>::hasFunction >::registerMe(1);       
        
#define KARABO_REGISTER_ONLY_ME_CC(configureMe) \
    template<> \
    const karabo::util::RegisterInFactory< \
        karabo::util::Factory<configureMe>, \
        karabo::util::SingleClassFactory<configureMe>, \
        configureMe > \
    karabo::util::SingleClassFactory<configureMe>::registerMe(1);
    }
}

#endif

