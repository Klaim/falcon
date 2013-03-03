/*
   FALCON - The Falcon Programming Language.
   FILE: private_caselist.h

   List of cases for switch, select and catch.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 10 Feb 2013 05:53:22 +0100

   -------------------------------------------------------------------
   (C) Copyright 2013: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#ifndef _FALCON_PRIVATE_CASELIST_H_
#define _FALCON_PRIVATE_CASELIST_H_

#include <falcon/setup.h>
#include <falcon/types.h>

#include <deque>

namespace Falcon {

class CaseItem {
public:
   typedef enum {
      e_nil,
      e_true,
      e_false,
      e_int,
      e_string,
      e_sym,
      e_rngInt,
      e_rngString
   }
   t_type;

   t_type m_type;

   int64 m_iLow;
   int64 m_iHigh;
   String* m_sLow;
   String* m_sHigh;
   Symbol* m_sym;

   CaseItem():
      m_type( e_nil ),
      m_sLow(0),
      m_sHigh(0)
   {}

   CaseItem( const CaseItem& other ):
      m_type( other.m_type ),
      m_iLow( other.m_iLow ),
      m_iHigh( other.m_iHigh),
      m_sLow( other.m_sLow ),
      m_sHigh( other.m_sHigh ),
      m_sym( other.m_sym )
   {}


   explicit CaseItem( bool mode ):
      m_type( mode ? e_true : e_false ) ,
      m_sLow(0),
      m_sHigh(0)
   {}

   explicit CaseItem( int64 value ):
      m_type( e_int ),
      m_iLow( value ),
      m_sLow(0),
      m_sHigh(0)
   {}

   CaseItem( int64 value, int64 v2 ):
      m_type( e_rngInt ),
      m_iLow( value ),
      m_iHigh( v2 ),
      m_sLow(0),
      m_sHigh(0)
   {}

   CaseItem( String* value ):
      m_type( e_string ),
      m_sLow( value ),
      m_sHigh(0)
   {}

   CaseItem( String* value, String* v2 ):
      m_type( e_rngString ),
      m_sLow( value ),
      m_sHigh( v2 )
   {}

   explicit CaseItem( Symbol* sym ):
      m_type( e_sym ),
      m_sLow(0),
      m_sHigh(0),
      m_sym( sym )
   {}

   ~CaseItem() {
      delete m_sLow;
      delete m_sHigh;
   }

   static void deletor( void* data ) {
      delete static_cast<CaseItem*>(data);
   }
};

class CaseList: public std::deque<CaseItem*>
{
public:
   ~CaseList() {
      iterator iter = begin();
      while (iter != end() ) {
         delete *iter;
         ++iter;
      }
   }

   static void deletor( void* data ) {
      CaseList* self = static_cast<CaseList*>(data);
      delete self;
   }
};

}

#endif

/* end of private_caselist.h */
