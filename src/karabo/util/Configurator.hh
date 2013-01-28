/* 
 * File:   Configurator.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 28, 2013, 2:09 PM
 */

#ifndef KARABO_UTIL_CONFIGURATOR_HH_hh
#define	KARABO_UTIL_CONFIGURATOR_HH_hh

#include <string>

#include "Hash.hh"
#include "Factory.hh"

#include "karaboDll.hh"

namespace karabo {
    namespace util {

        namespace confTools {

            template<class Class, class Argument, void (Class::*)(const Argument&) >
            struct FunctionExists {
            };

            template <class Class, class Argument>
            inline void configureIfPossible(const typename Class::Pointer& instance, const Argument& arg, FunctionExists<Class, Argument, &Class::configure >*) {
                return instance->configure(arg);
            }

            template <class Class, class Argument>
            inline void configureIfPossible(const typename Class::Pointer& instance, const Argument& arg, ...) {
                // Do nothing
            }

            inline std::pair<std::string, karabo::util::Hash> retrieveClassIdAndConfiguration(const karabo::util::Hash& rootedConfiguration) {
                if (rootedConfiguration.size() != 1) throw KARABO_LOGIC_EXCEPTION("Expecting exactly one (root-)node identifying the classId in configuration");
                std::string classId = rootedConfiguration.begin()->getKey();
                karabo::util::Hash config = rootedConfiguration.begin()->getValue<Hash > ();
                return std::make_pair(classId, config);
            }
        }

        template <class Base>
        class Configurator {
        public:

            KARABO_CLASSINFO(Configurator<Base>, "Configurator", "1.0");

            inline static typename Base::Pointer create(const karabo::util::Hash& configuration, const bool validate = true) {
                std::pair<std::string, karabo::util::Hash> p = karabo::util::confTools::retrieveClassIdAndConfiguration(configuration);
                return create(p.first, p.second, validate);
            }

            static typename Base::Pointer create(const std::string& classId, const karabo::util::Hash& configuration, const bool validate = true) {
                Pointer p = Factory<Configurator<Base> >::create(classId);
                //                if (validate) {
                //                    Schema schema;
                //                    p->buildUpSchema(schema);
                //                    Hash working = schema.validate(configuration);
                //                    return p->createAndConfigure(configuration);
                //                } else {
                return p->createAndConfigure(configuration);
                //}
            }

        protected:

            //virtual static void buildUpSchema(const karabo::util::Schema& schema) = 0;
            virtual typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) = 0;

        };

        template <class Base, typename Sub1>
        struct Configurator1 : public Configurator<Base> {

            typename Base::Pointer createAndConfigure(const karabo::util::Hash & configuration) {

                typename Base::Pointer basePointer = typename Base::Pointer(new Sub1());
                typename Sub1::Pointer sub1Pointer = boost::static_pointer_cast<Sub1 > (basePointer);

                karabo::util::confTools::configureIfPossible<Base, karabo::util::Hash > (basePointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub1, karabo::util::Hash > (sub1Pointer, configuration, 0);

                return basePointer;
            }
        };
       

        template <class Base, class Sub1, class Sub2>
        struct Configurator2 {

            typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) {

                typename Base::Pointer basePointer = typename Base::Pointer(new Sub2());
                typename Sub1::Pointer sub1Pointer = boost::static_pointer_cast<Sub1 > (basePointer);
                typename Sub2::Pointer sub2Pointer = boost::static_pointer_cast<Sub2 > (basePointer);

                karabo::util::confTools::configureIfPossible<Base, karabo::util::Hash > (basePointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub1, karabo::util::Hash > (sub1Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub2, karabo::util::Hash > (sub2Pointer, configuration, 0);

                return basePointer;
            }
        };

        template <class Base, class Sub1, class Sub2, class Sub3>
        struct Configurator3 {

            typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) {

                typename Base::Pointer basePointer = typename Base::Pointer(new Sub3());
                typename Sub1::Pointer sub1Pointer = boost::static_pointer_cast<Sub1 > (basePointer);
                typename Sub2::Pointer sub2Pointer = boost::static_pointer_cast<Sub2 > (basePointer);
                typename Sub3::Pointer sub3Pointer = boost::static_pointer_cast<Sub3 > (basePointer);

                karabo::util::confTools::configureIfPossible<Base, karabo::util::Hash > (basePointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub1, karabo::util::Hash > (sub1Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub2, karabo::util::Hash > (sub2Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub3, karabo::util::Hash > (sub3Pointer, configuration, 0);

                return basePointer;
            }
        };
        
        #define KARABO_REGISTER_FOR_CONFIGURATION_1(Base, Sub1) \
                template<> const karabo::util::FactoryMember0<karabo::util::Configurator<Base>, karabo::util::Configurator1<Base, Sub1> > \
                karabo::util::Register0<karabo::util::Configurator<Base>, karabo::util::Configurator1<Base, Sub1> >::registerAs(Sub1::classInfo().getClassId());
        
        #define KARABO_REGISTER_FOR_CONFIGURATION_2(Base, Sub1, Sub2) \
                template<> const karabo::util::FactoryMember0<karabo::util::Configurator<Base>, karabo::util::Configurator2<Base, Sub1, Sub2> > \
                karabo::util::Register0<karabo::util::Configurator<Base>, karabo::util::Configurator2<Base, Sub1, Sub2> >::registerAs(Sub2::classInfo().getClassId());
        
        #define KARABO_REGISTER_FOR_CONFIGURATION_3(Base, Sub1, Sub2, Sub3) \
                template<> const karabo::util::FactoryMember0<karabo::util::Configurator<Base>, karabo::util::Configurator3<Base, Sub1, Sub2, Sub3> > \
                karabo::util::Register0<karabo::util::Configurator<Base>, karabo::util::Configurator3<Base, Sub1, Sub2, Sub3> >::registerAs(Sub3::classInfo().getClassId());

    }
}
#endif
