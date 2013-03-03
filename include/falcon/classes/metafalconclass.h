/*
   FALCON - The Falcon Programming Language.
   FILE: metafalconclass.h

   Handler for classes defined by a Falcon script.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sat, 10 Mar 2012 23:27:16 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#ifndef _FALCON_METAFALCONCLASS_H_
#define _FALCON_METAFALCONCLASS_H_

#include <falcon/setup.h>
#include <falcon/classes/metaclass.h>

namespace Falcon
{

/** Handler for classes defined by a Falcon script.

 This class implements a class handler for classes a Falcon script. In other words,
 it is a handler for the "class type". The content of this type is a FalconClass,
 where properties and methods declared in a Falcon script class declaration
 are stored.
 
 */
class FALCON_DYN_CLASS MetaFalconClass: public MetaClass
{
public:

   MetaFalconClass();
   virtual ~MetaFalconClass();

   virtual void store( VMContext* ctx, DataWriter* stream, void* instance ) const;
   virtual void restore( VMContext* ctx, DataReader* stream ) const;
   virtual void flatten( VMContext* ctx, ItemArray& subItems, void* instance ) const;
   virtual void unflatten( VMContext* ctx, ItemArray& subItems, void* instance ) const;
   
   Class* handler() const;

   //=========================================================================
   // Initialize
   //
   virtual bool op_init( VMContext* ctx, void* instance, int32 pcount ) const;
   virtual void* createInstance() const; 
   
};

}

#endif /* _FALCON_METAFALCONCLASS_H_ */

/* end of metafalconclass.h */
