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

        template <class T>
        struct is_shared_ptr : boost::false_type {

        };

        template <class T>
        struct is_shared_ptr<boost::shared_ptr<T> > : boost::true_type {

        };

        typedef char (&yes)[1];
        typedef char (&no)[2];

        template <typename B, typename D>
        struct Host {

            operator B*() const;
            operator D*();
        };

        template <typename B, typename D>
        struct is_base_of {

            template <typename T>
            static yes check(D*, T);
            static no check(B*, int);

            static const bool value = sizeof (check(Host<B, D>(), int())) == sizeof (yes);

        };

        template<typename T, typename F>
        struct alias_cast_t {

            union {

                F* raw;
                T* data;
            };
        };

        template<typename T, typename F>
        struct alias_cast_t_c {

            union {

                const F* raw;
                const T* data;
            };
        };

        template<typename T, typename F>
        T* alias_cast(F* raw_data) {
            alias_cast_t<T, F> ac;
            ac.raw = raw_data;
            return ac.data;
        }

        template<typename T, typename F>
        T& alias_cast(F& raw_data) {
            alias_cast_t<T, F> ac;
            ac.raw = &raw_data;
            return *ac.data;
        }

        template<typename T, typename F>
        const T& alias_cast(const F& raw_data) {
            alias_cast_t_c<T, F> ac;
            ac.raw = &raw_data;
            return *ac.data;
        }



    }
}


#endif	/* METATOOLS_HH */

