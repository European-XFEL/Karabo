/* 
 * File:   SchemaBaseCategory.hh
 * Author: haufs
 *
 * Created on July 14, 2016, 10:17 AM
 */

#ifndef SCHEMABASECATEGORY_HH
#define	SCHEMABASECATEGORY_HH

#include "Schema.hh"

namespace karabo {
    namespace util {
        
        
        template <class T>
        class SchemaBaseCategory : public SchemaBaseCategory_, public Schema {
            friend class Schema;
        public:
            KARABO_CLASSINFO(SchemaBaseCategory, "SchemaBaseCategory", "1.0")
            
            SchemaBaseCategory(){}
            SchemaBaseCategory(Hash & hash_) {m_hash = hash_;};
            SchemaBaseCategory(const Hash & hash_) {m_hash = hash_;};
            virtual ~SchemaBaseCategory() {};
            
           
            virtual T create(Hash & hash_){
                return T(hash_);
            }
            
           
            virtual const T create(const Hash & hash_) const{
                return T(hash_);
            }
            
            
            
        protected:
            virtual boost::shared_ptr<SchemaBaseCategory_> clone() {
                return boost::shared_ptr<SchemaBaseCategory_>(new T(m_hash));
            };
            
            virtual boost::shared_ptr<const SchemaBaseCategory_> clone() const {
                return boost::shared_ptr<const SchemaBaseCategory_>(new T(m_hash));
            };
        
        };
            
            
    }
}


#endif	/* SCHEMABASECATEGORY_HH */

