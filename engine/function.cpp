/*
   FALCON - The Falcon Programming Language.
   FILE: function.cpp

   Function objects.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sat, 15 Jan 2011 19:09:07 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#include <falcon/classes/classfunction.h>

#include <falcon/function.h>
#include <falcon/symbol.h>
#include <falcon/item.h>
#include <falcon/collector.h>
#include <falcon/module.h>
#include <falcon/closure.h>
#include <falcon/callframe.h>
#include <falcon/stdhandlers.h>
#include <falcon/textwriter.h>
#include <falcon/pstep.h>

#include <falcon/engine.h>
#include <falcon/errors/paramerror.h>

#include <map>

namespace Falcon
{

Function::EtaSetter Function::eta;

Function::Function( const String& name, Module* module, int32 line ):
   Mantra( name, module, line ),
   m_methodOf( 0 ),
   m_bEta(false)
{
   m_category = e_c_function;
}

Function::~Function()
{
}

void Function::methodOf( Class* cls )
{
   if ( m_module == 0 && cls != 0 ) {
      m_module = cls->module();
   }
   m_methodOf = cls;
}


String Function::fullName() const
{
   if ( m_methodOf == 0 )
   {
      return name();
   }
   else
   {
      return m_methodOf->name() + "." + name();
   }
}


Class* Function::handler() const
{
   static Class* cls = Engine::handlers()->functionClass();   
   return cls;
}


bool Function::parseDescription( const String& params )
{
   m_signature = "";
   
   length_t ppos = 0;
   char_t chr;
   while( (chr = params.getCharAt(ppos)) == '&' )
   {
      setEta(true);
      ++ppos;
   }
      
   if ( ppos >= params.length() )
   {
      return true;
   }
   
   length_t pos;
   do 
   {
      pos = params.find( ',', ppos );   
      String param = params.subString(ppos, pos);
      param.trim();
      length_t pColon = param.find( ":" );
      if( pColon == String::npos || pColon == 0 )
      {
         return false;
      }
      else
      {
         String pname = param.subString(0,pColon); pname.trim();
         String psig = param.subString(pColon+1); psig.trim();
         
         addParam( pname );
         if( m_signature.size() > 0 )
         {
            m_signature += ",";
         }
         m_signature += psig;
      }
      
      ppos = pos+1;      
   }
   while( pos != String::npos );
   
   return true;
}


Error* Function::paramError(int line, const char* place ) const
{
   String placeName = place == 0 ? (m_module == 0 ? "" : m_module->name() ) : place;
   placeName.bufferize();
   return new ParamError(
           ErrorParam(e_inv_params, line == 0 ? m_sr.line(): line, placeName)
           .extra(m_signature) );
   
}


void Function::render( TextWriter* tgt, int32 depth ) const
{
   if( depth > 0 ) {
      tgt->write( String(" ").replicate(depth*PStep::depthIndent) );
   }

   if( name() == "" || name().startsWith("_anon#") ) {
      tgt->write( "{" );
   }
   else {
      tgt->write( "function " );
      tgt->write( name() );
      tgt->write( "(" );
   }

   // write the parameters
   int32 pcount = paramCount();
   for( int32 i = 0; i < pcount; ++i ) {
      const String& param = variables().getParamName(i);
      if( i > 0 )
      {
         tgt->write(", ");
      }
      tgt->write(param);
   }

   if( name() == "" || name().startsWith("_anon#") ) {
      tgt->write( " => \n" );
   }
   else {
      tgt->write( ")\n" );
   }

   renderFunctionBody( tgt, (depth < 0 ? -depth : depth+1) );

   if( depth > 0 ) {
      tgt->write( String(" ").replicate(depth*PStep::depthIndent) );
   }

   if( name() == "" || name().startsWith("_anon#") ) {
      tgt->write( "}" );
   }
   else {
      tgt->write( "end" );
   }

   if( depth >= 0 )
   {
      tgt->write("\n");
   }
}


void Function::renderFunctionBody( TextWriter* tgt, int32 depth ) const
{
   if( depth > 0 ) {
      tgt->write( String(" ").replicate(depth*PStep::depthIndent) );
   }

   tgt->write( "/* Native function */" );

   if( depth > 0 ) {
      tgt->write( "\n" );
   }
}

}

/* end of function.cpp */
