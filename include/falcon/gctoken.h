/*
   FALCON - The Falcon Programming Language.
   FILE: gctoken.h

   This is the representation of an item in the garbage collector.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 23 Jan 2011 11:55:54 +0100

   -------------------------------------------------------------------
   (C) Copyright 2004: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/
#ifndef FALCON_GCTOKEN_H
#define	FALCON_GCTOKEN_H

#include <falcon/class.h>

#include <stdio.h>
namespace Falcon
{

class Collector;
class VMContexte;

/** Data carrier for the garbage collector.
 *
 * This class is used internally by the engine to deliver an item to the garbatge
 * collector.
 *
 * It requires a class which knows how to handle the garbage value, and the
 * value itself as a void*.
 *
 */
class GCToken
{
public:
    void dispose() { m_cls->dispose( m_data ); }

    void *data() const { return m_data; }
    Class* cls() const { return m_cls; }
    
private:
    GCToken( Class* cls, void* data ):
         m_cls( cls ),
         m_data( data )
    {}

    ~GCToken() {}


    Class* m_cls;
    void* m_data;

    GCToken* m_next;
    GCToken* m_prev;

    friend class Collector;
    friend class VMContext;
};

}

#endif	/* GCTOKEN_H */