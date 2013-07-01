/*
   FALCON - The Falcon Programming Language.
   FILE: classmodule.cpp

   Module object handler.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Wed, 22 Feb 2012 19:50:45 +0100

   -------------------------------------------------------------------
   (C) Copyright 2012: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#undef SRC
#define SRC "engine/classes/classmodule.cpp"

#include <falcon/classes/classmodule.h>
#include <falcon/module.h>
#include <falcon/trace.h>
#include <falcon/module.h>
#include "../module_private.h"
#include "falcon/vmcontext.h"
#include "falcon/errors/ioerror.h"
#include <falcon/symbol.h>
#include <falcon/itemarray.h>
#include <falcon/modrequest.h>
#include <falcon/importdef.h>
#include <falcon/mantra.h>
#include <falcon/itemdict.h>
#include <falcon/stdhandlers.h>

#include <falcon/datawriter.h>
#include <falcon/datareader.h>

#include <falcon/errors/paramerror.h>
#include <falcon/errors/ioerror.h>
#include <falcon/errors/accesserror.h>

#include <vector>

namespace Falcon
{

ClassModule::ClassModule():
   Class("Module", FLC_CLASS_ID_MODULE)
{
   m_getAttributeMethod.methodOf(this);
   m_setAttributeMethod.methodOf(this);
   m_addMethod.methodOf(this);
   m_clearPriority = 3;
}

ClassModule::~ClassModule()
{
}


void ClassModule::dispose( void* self ) const
{
   Module* mod = static_cast<Module*>(self);
   mod->decref();
}


void* ClassModule::clone( void* source ) const
{
   Module* mod = static_cast<Module*>(source);
   TRACE( "Cloning module %p (%s - %s)", mod, mod->name().c_ize(), mod->uri().c_ize() );

   Module* modcopy = new Module(*mod);
   return modcopy;
}


void* ClassModule::createInstance() const
{
   return new Module;
}


void ClassModule::enumerateProperties( void*, Class::PropertyEnumerator& cb ) const
{
   cb("attributes" );
   cb("name" );
   cb("uri");
}


void ClassModule::enumeratePV( void* instance, Class::PVEnumerator& cb ) const
{
   Module* mod = static_cast<Module*>(instance);

   Item i_name = mod->name();
   Item i_uri = mod->uri();

   cb("name", i_name );
   cb("uri", i_uri );
}


bool ClassModule::hasProperty( void*, const String& prop ) const
{
   return
         prop == "attributes"
         || prop == "getAttribute"
         || prop == "name"
         || prop == "setAttribute"
         || prop == "uri"
         || prop == "add"
         ;
}


void ClassModule::store( VMContext*, DataWriter* stream, void* instance ) const
{
   Module* mod = static_cast<Module*>(instance);
   TRACE( "ClassModule::store -- Storing module %p %s (%s - %s)",
      mod, (mod->isNative()?"native" : "syntactic" ),
      mod->name().c_ize(), mod->uri().c_ize() );

   stream->write( mod->isNative() );
   stream->write(mod->name());

   if( mod->isNative() )
   {
      stream->write( mod->uri() );
      return;
   }

   // otherwise, we don't have to save the URI, as it will be rewritten.
   // First, prepare to save the module ids.
   Module::Private* mp = mod->_p;
   int32 progID;

   // Now store the module requests
   {
      // first, number the module requests.
      Module::Private::ModReqList& mrlist = mp->m_mrlist;
      progID = (int32) mrlist.size();
      TRACE1( "ClassModule::store -- storing %d mod requests", progID );
      stream->write( progID );
      Module::Private::ModReqList::iterator mri = mrlist.begin();
      progID = 0;
      while( mri != mrlist.end() ) {
         ModRequest* req = *mri;
         req->store( stream );
         // save the progressive ID.
         req->id( progID++ );
         ++mri;
      }

      // We can now proceed to the import defs.
      Module::Private::ImportDefList& idlist = mp->m_importDefs;
      progID = (int32) idlist.size();
      TRACE1( "ClassModule::store -- storing %d import definitions", progID );
      stream->write( progID );
      Module::Private::ImportDefList::iterator idi = idlist.begin();
      progID = 0;
      while( idi != idlist.end() ) {
         ImportDef* def = *idi;
         def->id( progID++ );
         def->store( stream );
         if( def->modReq() != 0 )
         {
            stream->write( (int32) def->modReq()->id() );
         }
         else {
            stream->write( (int32) -1 );
         }

         // save the progressive ID.
         ++idi;
      }

      // finally, we can write the modRequest->import deps.
      mri = mrlist.begin();
      while( mri != mrlist.end() )
      {
         ModRequest* req = *mri;
         uint32 count = (uint32) req->importDefCount();
         TRACE1( "ClassModule::store -- Request %d has %d imports", req->id(), count );
         stream->write( count );
         for( uint32 i = 0; i < count; ++i ) {
            ImportDef* idl = req->importDefAt(i);
            uint32 id = idl->id();
            stream->write( id );
            TRACE2( "ClassModule::store -- Request %d -> import %d", req->id(), id );
         }
         ++mri;
      }
   }


   // Import definition
   MESSAGE1( "Module store import definition." );
   {
      Module::Private::Externals& exts = mp->m_externals;
      progID = (int32) exts.size();
      stream->write( progID );
      TRACE1( "ClassModule::store -- storing %d externals", progID );
      Module::Private::Externals::iterator depi = exts.begin();
      progID = 0;
      while( depi != exts.end() ) {
         Symbol* sym = depi->first;

         stream->write( sym->name() );
         // line
         stream->write( depi->second.m_line );
         ImportDef* def = depi->second.m_def;
         stream->write( def == 0 ? -1 : def->id() );
         Symbol* srcSym = depi->second.m_srcSym;
         stream->write( srcSym == 0 ? "" : srcSym->name() );

         ++depi;
      }
   }

   MESSAGE1( "Module store namespace translations." );
   {
      Module::Private::NSTransMap& nstm = mp->m_nsTransMap;
      progID = (int32) nstm.size();
      stream->write( progID );
      TRACE1( "ClassModule::store -- storing %d namespace translations", progID );
      Module::Private::NSTransMap::iterator depi = nstm.begin();
      progID = 0;
      while( depi != nstm.end() ) {
         const String& name = depi->first;
         stream->write( name );
         ImportDef* def = depi->second;
         fassert( def != 0 );
         stream->write( def->id() );

         ++depi;
      }
   }

   MESSAGE1( "Module store attributes." );

   // store the attributes
   mod->attributes().store(stream);

   MESSAGE1( "Module store international strings." );
   {
      Module::Private::StringSet& sset = mod->_p->m_istrings;
      uint32 size = sset.size();
      stream->write( size );
      Module::Private::StringSet::iterator iter = sset.begin();
      while( sset.end() != iter )
      {
         stream->write( *iter );
         ++iter;
      }
   }

   MESSAGE1( "Module store complete." );
}


void ClassModule::restore( VMContext* ctx, DataReader* stream ) const
{
   static Class* mcls = Engine::handlers()->moduleClass();
   MESSAGE( "Restoring module..." );

   bool bIsNative;
   String name;
   stream->read( bIsNative );
   stream->read( name );

   TRACE1( "Module being restored: %s (%s)",
      (bIsNative?"native" : "syntactic" ),
      name.c_ize() );

   if( bIsNative )
   {
      String origUri;
      stream->read( origUri );

      Module* mod = new Module( name, true );
      mod->uri( origUri );
      ctx->pushData( FALCON_GC_STORE( mcls, mod ) );
      return;
   }

   //
   Module* mod = new Module(name, false);

   try {
      restoreModule( mod, stream );
   }
   catch( ... )
   {
      delete mod;
      throw;
   }

   ctx->pushData( Item( mcls, mod ) );
}


void ClassModule::restoreModule( Module* mod, DataReader* stream ) const
{
   TRACE( "ClassModule::restoreModule %s", mod->name().c_ize() );

   int32 progID, count;
   // First, prepare to save the module ids.
   Module::Private* mp = mod->_p;

   // Now restore the module requests
   Module::Private::ModReqList& mrlist = mp->m_mrlist;
   Module::Private::ModReqMap& mrmap = mp->m_mrmap;

   stream->read( count );
   TRACE1( "ClassModule::restoreModule -- reading %d mod requests", count );
   //mrlist.reserve( count );
   progID = 0;
   while( progID < count ) {
      ModRequest* req = new ModRequest;
      try
      {
         req->restore( stream );
         req->id(progID);
         TRACE2( "Read mod request %s (%s) as %s",
            req->name().c_ize(),
            (req->isUri() ? " by uri" : "by name" ),
            (req->isLoad() ? "load" : "import" ) );
      }
      catch( ... )
      {
         delete req;
         throw;
      }

      mrlist.push_back( req );
      mrmap[req->name()] = req;
      ++progID;
   }

   // We can now proceed to the import defs.
   Module::Private::ImportDefList& idlist = mp->m_importDefs;

   stream->read( count );
   TRACE1( "ClassModule::restoreModule -- reading %d import defs", count );
   //idlist.reserve( count );
   progID = 0;
   while( progID < count )
   {
      ImportDef* def = new ImportDef;
      try {
         def->restore( stream );
         int32 modreq = -1;
         stream->read( modreq );
         if( modreq >= 0 )
         {
            if( modreq >= (int32) mrlist.size() )
            {
               throw new IOError( ErrorParam( e_deser, __LINE__, SRC )
                  .origin( ErrorParam::e_orig_loader )
                  .extra(String("Module request ID out of range on ImportDef ").N(progID) )
                  );
            }

            def->modReq( mrlist[modreq] );
            def->id(progID);
         }

         idlist.push_back(def);
      }
      catch( ... ) {
         delete def;
         throw;
      }

      ++progID;
   }

   // finally, we can load the modRequest->import deps.
   {
      Module::Private::ModReqList::iterator mri = mrlist.begin();
      while( mri != mrlist.end() )
      {
         ModRequest* req = *mri;
         uint32 count;
         stream->read( count );
         TRACE1( "ClassModule::restoreModule -- Request %d has %d imports", req->id(), count );

         for( uint32 i = 0; i < count; ++i ) {
            uint32 id;
            stream->read(id);
            if ( id >= idlist.size() ) {
               throw new IOError( ErrorParam( e_deser, __LINE__, SRC )
                        .origin( ErrorParam::e_orig_loader )
                        .extra(String("ImportDef ID out of range on ModReq ").N(req->id()) )
                        );
            }
            ImportDef* idef = idlist[id];
            req->addImportDef(idef);
         }
         ++mri;
      }
   }

   // dependencies.
   Module::Private::Externals& exts = mp->m_externals;
   stream->read( count );
   TRACE1( "ClassModule::restoreModule -- reading %d dependencies", count );
   progID = 0;
   while( progID < count )
   {
      String sName;
      String sSrcName;
      int32 line, idDef;

      stream->read( sName );
      stream->read( line );
      stream->read( idDef );
      stream->read( sSrcName );

      ImportDef* idef = 0;

      if( idDef >= 0 )
      {
         if( idDef >= (int32) idlist.size() )
         {
            throw new IOError( ErrorParam( e_deser, __LINE__, SRC )
               .origin( ErrorParam::e_orig_loader )
               .extra(String("ImportDef out of range dependency ").N(progID) )
               );
         }
         idef = idlist[idDef];
      }

      if( sSrcName !=  "" )
      {
         exts.insert( std::make_pair(Engine::getSymbol(sName), Module::Private::ExtDef(line, idef, sSrcName ) ));
      }
      else {
         exts.insert( std::make_pair(Engine::getSymbol(sName), Module::Private::ExtDef(line, idef ) ));
      }

      TRACE2( "ClassModule::restoreModule -- restored dependency %d: %s idef:%d",
               progID, sName.c_ize(), idDef );

      ++progID;
   }

   // translations.
   Module::Private::NSTransMap& nstm = mp->m_nsTransMap;
   stream->read( count );
   TRACE1( "ClassModule::restoreModule -- reading %d namespace translations", count );
   progID = 0;
   while( progID < count )
   {
      String sName;
      int32 idDef;

      stream->read( sName );
      stream->read( idDef );

      ImportDef* idef = 0;

      if( idDef < 0 || idDef >= (int32) idlist.size() )
      {
         throw new IOError( ErrorParam( e_deser, __LINE__, SRC )
            .origin( ErrorParam::e_orig_loader )
            .extra(String("ImportDef out of range dependency ").N(progID) )
            );
      }

      idef = idlist[idDef];
      nstm.insert(std::make_pair(sName,idef));

      TRACE2( "ClassModule::restoreModule -- restored translation %d: %s idef:%d",
               progID, sName.c_ize(), idDef );

      ++progID;
   }


   MESSAGE1( "Module restore -- attributes" );

   // restore the attributes
   mod->attributes().restore(stream);

   MESSAGE1( "Module restore -- international strings." );
   {
      Module::Private::StringSet& sset = mod->_p->m_istrings;
      uint32 size = 0;
      stream->read( size );
      for( uint32 i = 0; i < size; ++i )
      {
         String temp;
         stream->read( temp );
         sset.insert(temp);
      }
   }

   MESSAGE1( "Module restore complete." );
}

void ClassModule::flatten( VMContext* ctx, ItemArray& subItems, void* instance ) const
{
   Module* mod = static_cast<Module*>(instance);
   TRACE( "Flattening module %p %s (%s - %s)",
      mod,  (mod->isNative()?"native" : "syntactic" ),
      mod->name().c_ize(), mod->uri().c_ize() );

   if( mod->isNative() )
   {
      // nothing to do,
      return;
   }

   // First, get enough lenght
   Module::Private* mp = mod->_p;

   subItems.reserve(
      mod->globals().size()*3 +
      mp->m_mantras.size()+
      mod->attributes().size() * 2 +
      4
   );

   // save all the global variables
   mod->globals().flatten(ctx, subItems);
   TRACE( "ClassModule::flatten -- stored %d variables", (uint32) subItems.length() / 3 );
   // Push a nil as a separator
   subItems.append(Item());

   // save mantras
   {
      Module::Private::MantraMap& mantras = mp->m_mantras;
      Module::Private::MantraMap::iterator fi = mantras.begin();
      while( fi != mantras.end() )
      {
         TRACE1("Flattening mantra %s", fi->first.c_ize() );
         Mantra* mantra = fi->second;
         // skip hyperclasses
         if( ! mantra->isCompatibleWith( Mantra::e_c_hyperclass ))
         {
            Class* cls = mantra->handler();
            TRACE1("Mantra %s has handler %s(%p)", fi->first.c_ize(), cls->name().c_ize(), cls );
            subItems.append( Item(cls, mantra) );
         }
         ++fi;
      }
   }
   // Push a nil as a separator
   subItems.append( Item() );

   // finally push the classes in need of init
   {
      Module::Private::InitList& initList = mp->m_initList;
      Module::Private::InitList::iterator ii = initList.begin();
      while( ii != initList.end() ) {
         Class* cls = *ii;
         subItems.append( Item(cls->handler(), cls) );
         ++ii;
      }
   }
   // Push a nil as a separator
   subItems.append( Item() );

   // save the attributes.
   mod->attributes().flatten(subItems);

   // Push a nil as a separator
   subItems.append( Item() );

   // complete.
}


void ClassModule::unflatten( VMContext* ctx, ItemArray& subItems, void* instance ) const
{
   Module* mod = static_cast<Module*>(instance);
   TRACE( "ClassModule::unflatten -- module %p %s (%s - %s)",
      mod,  (mod->isNative()?"native" : "syntactic" ),
      mod->name().c_ize(), mod->uri().c_ize() );

   if( mod->isNative() )
   {
      // nothing to do,
      return;
   }

   Module::Private* mp = mod->_p;
   uint32 pos = 0;

   // First, restore the global variables.
   mod->globals().unflatten( ctx, subItems, 0, pos);
   TRACE( "ClassModule::unflatten -- restored %d globals", pos/3 );
   ++pos; // skip the nil spearator after globals.

   const Item* current = &subItems[pos];
   while( ! current->isNil() && pos < subItems.length()-2 )
   {
      Mantra* mantra = static_cast<Mantra*>(current->asInst());
      TRACE1( "ClassModule::unflatten -- restoring mantra %s ", mantra->name().c_ize() );

      if( mantra->name() == "__main__" )
      {
         mod->m_mainFunc = static_cast<Function*>(mantra);
         mod->m_mainFunc->setMain(true);
      }

      mp->m_mantras[mantra->name()] = mantra;
      mantra->module( mod );
      // no need to store the mantra in globals:
      // the globals already unflattened and mantras are in place.

      ++pos;
      current = &subItems[pos];
   }

   TRACE( "ClassModule::unflatten -- restored mantras, at position %d", pos );

   // recover init classes
   Module::Private::InitList& inits = mp->m_initList;
   current = &subItems[++pos];
   while( ! current->isNil() && pos < subItems.length()-1)
   {
      Class* cls = static_cast<Class*>(current->asInst());
      TRACE1( "ClassModule::unflatten -- restored class in need of init %s", cls->name().c_ize() );
      inits.push_back( cls );
      ++pos;
      current = &subItems[pos];
   }

   TRACE( "ClassModule::unflatten -- restored init classes, at position %d", pos );

   mod->attributes().unflatten( subItems, pos );

   TRACE( "ClassModule::unflatten -- restored attributes, at position %d", pos );
}


void ClassModule::describe( void* instance, String& target, int , int ) const
{
   Module* mod = static_cast<Module*>(instance);
   target = "Module " + mod->name();
}

void ClassModule::gcMarkInstance( void* instance, uint32 mark ) const
{
   static_cast<Module*>(instance)->gcMark(mark);
}

bool ClassModule::gcCheckInstance( void* instance, uint32 mark ) const
{
   return static_cast<Module*>(instance)->lastGCMark() >= mark;
}

bool ClassModule::op_init( VMContext* ctx, void* instance, int32 pcount ) const
{
   // NAME - URI
   Item& i_name = ctx->opcodeParam(0);
   Item& i_uri = ctx->opcodeParam(1);

   if( pcount < 1
      || ! i_name.isString()
      || (pcount > 1 && ! i_uri.isString())
      )
   {
      throw new ParamError( ErrorParam(e_inv_params, __LINE__, SRC )
         .origin(ErrorParam::e_orig_vm)
         .extra( "S,[S]") );
   }

   Module* module =  static_cast<Module*>(instance);
   module->name( *i_name.asString() );
   if( pcount > 1 )
   {
      module->uri( *i_uri.asString() );
   }

   return false;
}


void ClassModule::op_getProperty( VMContext* ctx, void* instance, const String& prop) const
{
   Module* mod = static_cast<Module*>(instance);

   if( prop == "attributes" )
   {
      ItemDict* dict = new ItemDict;
      uint32 size = mod->attributes().size();
      for( uint32 i = 0; i < size; ++i ) {
         Attribute* attr = mod->attributes().get(i);
         dict->insert( FALCON_GC_HANDLE( new String(attr->name())), attr->value() );
      }
      ctx->stackResult(1, FALCON_GC_HANDLE(dict) );
   }
   else if( prop == "getAttribute" )
   {
      ctx->topData().methodize(&m_getAttributeMethod);
   }
   else if( prop == "name" )
   {
      ctx->stackResult(1, FALCON_GC_HANDLE( new String(mod->name())) );
   }
   else if( prop == "setAttribute" )
   {
      ctx->topData().methodize(&m_setAttributeMethod);
   }
   else if( prop == "uri" )
   {
      ctx->stackResult(1, FALCON_GC_HANDLE( new String(mod->uri())) );
   }
   else if( prop == "add" )
   {
      ctx->topData().methodize(&m_addMethod);
   }
   else {
      Class::op_getProperty(ctx, instance, prop );
   }
}

//========================================================================
// Methods
//

ClassModule::GetAttributeMethod::GetAttributeMethod():
   Function("getAttribute")
{
   signature("S");
   addParam("name");
}

ClassModule::GetAttributeMethod::~GetAttributeMethod()
{}


void ClassModule::GetAttributeMethod::invoke( VMContext* ctx, int32 )
{
   Item& self = ctx->self();
   fassert( self.isUser() );

   Item* i_name = ctx->param(0);
   if( ! i_name->isString() )
   {
      ctx->raiseError(paramError());
      return;
   }

   const String& attName = *i_name->asString();
   Module* mantra = static_cast<Module*>(self.asInst());
   Attribute* attr = mantra->attributes().find(attName);
   if( attr == 0 )
   {
      ctx->raiseError( new AccessError( ErrorParam(e_dict_acc, __LINE__, SRC )
            .symbol("Module.getAttribute")
            .module("[core]")
            .extra(attName) ) );
      return;
   }

   ctx->returnFrame(attr->value());
}

ClassModule::SetAttributeMethod::SetAttributeMethod():
   Function("setAttribute")
{
   signature("S,[X]");
   addParam("name");
   addParam("value");
}

ClassModule::SetAttributeMethod::~SetAttributeMethod()
{}


void ClassModule::SetAttributeMethod::invoke( VMContext* ctx, int32 )
{
   Item& self = ctx->self();
   fassert( self.isUser() );

   Item* i_name = ctx->param(0);
   Item* i_value = ctx->param(1);
   if( i_name == NULL || ! i_name->isString() )
   {
      ctx->raiseError(paramError());
      return;
   }

   const String& attName = *i_name->asString();
   Module* mantra = static_cast<Module*>(self.asInst());

   if( i_value == 0 )
   {
      mantra->attributes().remove(attName);
   }
   else {
      Attribute* attr = mantra->attributes().find(attName);
      if( attr == 0 )
      {
         attr = mantra->attributes().add(attName);
      }

      attr->value().copyInterlocked( *i_value );
   }

   ctx->returnFrame();
}

ClassModule::AddMethod::AddMethod():
   Function("add")
{
   signature("Mantra,[B]");
   addParam("mantra");
   addParam("export");
}

ClassModule::AddMethod::~AddMethod()
{}


void ClassModule::AddMethod::invoke( VMContext* ctx, int32 )
{
   Item& self = ctx->self();
   fassert( self.isUser() );

   static Class* clsMantra = Engine::instance()->handlers()->mantraClass();

   Item* i_mantra = ctx->param(0);
   Item* i_export = ctx->param(1);
   if( i_mantra == NULL || ! i_mantra->asClass()->isDerivedFrom(clsMantra) )
   {
      ctx->raiseError(paramError());
      return;
   }

   bool bExport = true;
   if ( i_export != NULL )
   {
      if ( ! i_export->isBoolean() )
      {
         ctx->raiseError(paramError());
         return;
      }
      bExport = i_export->asBoolean();
   }

   void* inst;
   Class* cls;
   i_mantra->forceClassInst(cls, inst);

   Mantra* mantra = static_cast<Mantra*>(inst);
   Module* module = static_cast<Module*>(self.asInst());

   module->addMantra(mantra, bExport);

   ctx->returnFrame();
}


}

/* end of classmodule.cpp */
