/* 
 * File:   CArray.hh
 * Author: irinak
 *
 * Created on August 2, 2011, 3:53 PM
 */

#ifndef EXFEL_UTIL_CARRAY_HH
#define	EXFEL_UTIL_CARRAY_HH

namespace exfel {
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

#endif	/* EXFEL_UTIL_CARRAY_HH */

