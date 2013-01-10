/*
   FALCON - The Falcon Programming Language.
   FILE: exprclosure.h

   Genearte a closure out of a function value.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 01 Jan 2012 21:15:18 +0100

   -------------------------------------------------------------------
   (C) Copyright 2012: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#define SRC "engine/psteps/exprclosure.cpp"

#include <falcon/psteps/exprclosure.h>
#include <falcon/closure.h>
#include <falcon/function.h>
#include <falcon/synclasses.h>

#include "falcon/vmcontext.h"

namespace Falcon {

ExprClosure::ExprClosure():
   m_function(0)
{
   FALCON_DECLARE_SYN_CLASS( expr_closure );
   apply = apply_;
   m_trait = e_trait_composite;
}


ExprClosure::ExprClosure( Function* closed ):
   m_function(closed)
{
   FALCON_DECLARE_SYN_CLASS( expr_closure );
   apply = apply_;   
   m_trait = e_trait_composite;
   
   // TODO: GarbageLock the function
}

ExprClosure::ExprClosure( const ExprClosure& other ):
   Expression(other),
   m_function( other.m_function )
{
   apply = apply_;
   m_trait = e_trait_composite;
   // TODO: GarbageLock the function
}

ExprClosure::~ExprClosure() {}
   

void ExprClosure::describeTo( String& tgt, int ) const
{
   if( m_function == 0 ) {
      tgt = "<Blank ExprClosure>";
      return;
   }
   tgt = "/* close */ " + m_function->name();
}
   
   
void ExprClosure::apply_( const PStep* ps, VMContext* ctx )
{
   const ExprClosure* self = static_cast<const ExprClosure*>(ps);
   fassert( self->m_function != 0 );

   TRACE( "ExprClosure::apply_ %s", self->m_function->name().c_ize() );
   
   // Around just once
   ctx->popCode();
   
   if( self->m_function->variables().closedCount() > 0 ) {
      // Create the closure and close it.
      register Function* func = self->m_function;
      Closure* cls = new Closure( func );
      cls->close( ctx );

      // and return it.
      ctx->pushData( FALCON_GC_HANDLE( cls ) );
   }
   else {
      ctx->pushData( self->m_function );
   }
}

}

/* end of exprclosure.cpp */
