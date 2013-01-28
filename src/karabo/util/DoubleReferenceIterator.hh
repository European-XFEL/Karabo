/* 
 * Author: boukhelef
 *
 * Created on January 3, 2013, 12:30 PM
 */


#ifndef KARABO_UTIL_DOUBLE_REFERENCE_ITERATOR_HH
#define KARABO_UTIL_DOUBLE_REFERENCE_ITERATOR_HH

namespace karabo {
    namespace util {

        template<class T, class R = typename T::MappedType>
        class DoubleReferenceIterator : public T::iterator {
        public:

            DoubleReferenceIterator(const typename T::iterator& it) : T::iterator(it) {
            }

            R& operator*() {
                return *(T::iterator::operator*());
            }

            R& operator*() const {
                return *(T::iterator::operator*());
            }

            R* operator->() {
                return T::iterator::operator*();
            }

            const R* operator->() const {
                return T::iterator::operator*();
            }
        };

        template<class T, class R = typename T::MappedType>
        class ConstDoubleReferenceIterator : public T::const_iterator {
        public:

            ConstDoubleReferenceIterator(const typename T::const_iterator& it) : T::const_iterator(it) {
            }

            ConstDoubleReferenceIterator(const typename T::iterator& it) : T::const_iterator(it) {
            }

            const R& operator*() const {
                return *(T::const_iterator::operator*());
            }

            const R* operator->() const {
                return T::const_iterator::operator*();
            }
        };
    }
}
#endif