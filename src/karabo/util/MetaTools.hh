/*
 * File:   MetaTools.hh
 * Author: heisenb
 *
 * Created on August 10, 2016, 3:56 PM
 */

#ifndef KARABO_UTIL_METATOOLS_HH
#define	KARABO_UTIL_METATOOLS_HH

namespace karabo {
    namespace util {

        class Hash;

        template <class T>
        struct is_shared_ptr : boost::false_type {

        };

        template <class T>
        struct is_shared_ptr<boost::shared_ptr<T> > : boost::true_type {

        };

        template<typename is_hash_base>
        struct conditional_hash_cast {

            template<typename T>
            static const Hash& cast(const T& v) {
                return reinterpret_cast<const Hash&> (v);
            }

            template<typename T>
            static const boost::shared_ptr<T> cast(const boost::shared_ptr<T>& v) {
                inserting_pointers_of_hash_and_derived_classes_into_the_hash_is_not_supported();
                return v;
            }


            void inserting_pointers_of_hash_and_derived_classes_into_the_hash_is_not_supported();
        };

        template<>
        struct conditional_hash_cast<boost::false_type> {

            template<typename T>
            static T& cast(T& v) {
                return v;
            }

            template<typename T>
            static const T& cast(const T& v) {
                return v;
            }



        };



    }
}


#endif	/* METATOOLS_HH */

