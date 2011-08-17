/*
   FALCON - The Falcon Programming Language.
   FILE: itemdict.h

   Class storing lexicographic ordered item dictionaries.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Mon, 18 Jul 2011 02:22:35 +0200

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/
#ifndef _FALCON_ITEMDICT_H
#define	_FALCON_ITEMDICT_H

#include <falcon/setup.h>
#include <falcon/string.h>
#include <falcon/overridableclass.h>
#include <falcon/genericitem.h>

namespace Falcon
{

/** Class storing lexicographic ordered item dictionaries.

 */
class FALCON_DYN_CLASS ItemDict
{
public:

   ItemDict();
   ItemDict( const ItemDict& other );
   ~ItemDict();

   ItemDict* clone() const { return new ItemDict(*this); }


   void gcMark( uint32 mark );
   uint32 currentMark() const { return m_currentMark; }

   uint32 flags() const { return m_flags; }
   void flags( uint32 v ) { m_flags = v; }

   void insert( const Item& key, const Item& value );
   void remove( const Item& key );
   Item* find( const Item& key );

   length_t size() const;

   void describe( String& target, int depth = 3, int maxlen = 60 ) const;

   class Enumerator {
   public:
      virtual void operator()( const Item& key, Item& value )=0;
   };
   
   void enumerate( Enumerator& rator );   
   uint32 version() const { return m_version; }
   
   /** Iterator used by ClassDict to iterate with op_first/op_next. */
   class Iterator: public GenericItem
   {
   public:
      Iterator( ItemDict* item );      
      virtual ~Iterator();
      
      virtual bool gcCheck( uint32 value );
      virtual void gcMark( uint32 value );
      virtual Iterator* clone() const;
      virtual void describe( String& target ) const;

      /** Prepare the next pair on the target item. */
      bool next( Item& target );
            
   private:
      class Private;
      Private* _pm;
      
      ItemDict* m_dict;
      uint32 m_version;      
      uint32 m_currentMark;
      bool m_complete;
      String m_tempString;
      
      /** Internal iterator advancement. */
      void advance();
      
      typedef enum
      {
         e_st_none,
         e_st_nil,
         e_st_true,
         e_st_false,
         e_st_int,
         e_st_range,
         e_st_string,
         e_st_other,
         e_st_done
      } state;
      
      state m_state;
   };
   
private:
   class Private;
   Private* _p;
   
   uint32 m_flags;
   uint32 m_currentMark;
   uint32 m_version;
   
   friend class Iterator;
};

}

#endif /* _FALCON_ITEMDICT_H_ */

/* end of itemdict.h */
