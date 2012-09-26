/* 
 * File:   CArray.hh
 * Author: irinak
 *
 * Created on August 2, 2011, 3:53 PM
 */

#ifndef KARABO_UTIL_CARRAY_HH
#define	KARABO_UTIL_CARRAY_HH

namespace karabo {
  namespace util {

    template<typename T>
    struct CArray {

      CArray() : ptr(0), size(0) {
      }

      explicit CArray(T* aptr, size_t asize) : ptr(aptr), size(asize) {
      }

      inline void set(T* aptr, size_t asize) {
        ptr = aptr;
        size = asize;
      }

      inline void get(T* aptr, size_t& asize) {
        aptr = ptr;
        asize = size;
      }

      T* ptr;
      size_t size;
     
    };
    
    

    template<typename T>
    struct CMatrix : public CArray<T> {
      size_t mrows;
      size_t ncolumns;
    };

  }
}

#endif	/* KARABO_UTIL_CARRAY_HH */

