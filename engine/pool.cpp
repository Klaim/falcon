/*
   FALCON - The Falcon Programming Language.
   FILE: pool.cpp

   Generic pool for recycleable instances. 
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sat, 21 Apr 2012 21:07:10 +0200

   -------------------------------------------------------------------
   (C) Copyright 2012: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/
#undef SRC
#define SRC "engine/pool.cpp"

#include <falcon/pool.h>
#include <falcon/poolable.h>

namespace Falcon {

Pool::Pool( uint32 maxSize ):
   m_head(0),
   m_size(0),
   m_maxSize(maxSize)
{
}


uint32 Pool::size()
{
   m_mtx.lock();
   uint32 s = m_size;
   m_mtx.unlock();
   return s;
}

void Pool::clear()
{  
   m_mtx.lock();
   while( m_head != 0 )
   {
      Poolable* p = m_head;
      m_head = m_head->m_next;
      m_mtx.unlock();
      
      delete p;
      
      m_mtx.lock();
   }
   m_size = 0;
   m_mtx.unlock();
}
   

Poolable* Pool::get()
{
   m_mtx.lock();
   Poolable* p = m_head;
   if ( p != 0 )
   {
      m_head = m_head->m_next;
      m_size--;
   }
   m_mtx.unlock();
   
   return p;
}

void Pool::release( Poolable* data )
{
   m_mtx.lock();
   if( m_size < m_maxSize )
   {
      m_size++;
      data->m_next = m_head;
      m_head = data;
      m_mtx.unlock();
   }
   else {
      m_mtx.unlock();
      
      delete data;
   }   
}
   
}

/* end of pool.cpp */
