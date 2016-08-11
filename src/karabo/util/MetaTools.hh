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
    }
}


#endif	/* METATOOLS_HH */

