/*
   FALCON - The Falcon Programming Language.
   FILE: classmodspace.cpp

   Handler for dynamically created module spaces.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Tue, 05 Feb 2013 18:07:35 +0100

   -------------------------------------------------------------------
   (C) Copyright 2013: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#undef SRC
#define SRC "engine/classes/classmodspace.cpp"

#include <falcon/fassert.h>
#include <falcon/vmcontext.h>
#include <falcon/stderrors.h>
#include <falcon/function.h>
#include <falcon/engine.h>
#include <falcon/modspace.h>
#include <falcon/module.h>
#include <falcon/uri.h>
#include <falcon/stdsteps.h>
#include <falcon/modloader.h>

#include <falcon/classes/classmodspace.h>


namespace Falcon {

/*#
@class ModSpace
@brief Interface for loading modules in a sandbox.

Falcon organizes modules in groups called @i{module spaces}.
A module space is where a main module loads the required dependencies,
and where the loaded modules cooperate in exporting and importing common
symbols.

Each Falcon @a VMProcess has a main ModSpace which is where the modules
are originally loaded. Functions loading modules dynamically, as include(),
create a new module space where the required module can then store its dependencies
and it's exported symbols, so that the new module doesn't pollute the process space.

Once all the modules living in a ModSpace are dropped, and all the references to the
data they exported are removed from the process, the ModSpace unloads the modules and deletes
their data, eventually destroying itself.

Module spaces are organized in a hierarchy that has the process main space as the top one. Each
module space created there after has a parent, which is the module space where a the module
executing the load request resides, or the top space if the code being execute is module-less (i.e.
dynamic code created on the fly). All the modules and globals visible in a parent module space
are also visible to the modules residing in the children spaces, while the children globals are not
visible to the parent or siblings (unless explicitly and dynamically accessed).

In this way, it is possible to load multiple times the same module, which has the same dependencies
and exports the same variables, without generating a symbol export clash, and keeping their state
isolated so that each loaded copy can work separately and autonomously.

@section modspace_loading

Other than organizing the living space of modules, the ModSpace class is also responsible for
dynamic module loading. It provides asynchronous load facility and support; compiling source files
or loading pre-compiled .fam modules can be done through its compiler interface.

*/


static int checkEnumParam( const Item& value, int max )
{
   if( ! value.isOrdinal() )
   {
      throw FALCON_SIGN_XERROR( ParamError, e_inv_prop_value, .extra("N") );
   }

   int64 v = value.forceInteger();

   if( v < 0 || v > max )
   {
      throw FALCON_SIGN_XERROR( ParamError, e_inv_prop_value, .extra(String("0<=N<=").N(max)) );
   }

   return v;
}


/*#
 @prop savePC ModSpace
 @brief Indicates how the module space should store modules once it compiles them on the fly.

 When the module space loads a module by compiling a source Falcon script, it might automatically
 save the serialized compiled result to a .fam script, so that the next time the same script is
 serached for, the .fam serialized module is de-serialized at a fraction of the time needed to
 compile the original script.

 However, it might be impossible to save the .fam module on a certain location; the default action
 is to ignore the problem. This property can be used to change it.

 It might assume one of the following values:
 - ModSpace.savePC_NEVER: Don't try to save the pre-compiled modules.
 - ModSpace.savePC_TRY: Try to save the the pre-compiled modules, but ignore I/O errors.
 - ModSpace.savePC_MANDATORY: Try to save the pre-compiled modules and throw an error on failure.
 */

static void get_savePC( const Class*, const String&, void* instance, Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   value.setInteger( static_cast<int64>(ms->modLoader()->savePC()) );
}

static void set_savePC( const Class*, const String&, void* instance, const Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   int v = checkEnumParam( value, static_cast<int>(ModLoader::e_save_mandatory) );
   ms->modLoader()->savePC( static_cast<ModLoader::t_save_pc>(v) );
}


/*#
 @prop checkFTD ModSpace
 @brief Determines how Falcon Template Documents are checked and interpreted.

 When loading a source Falcon script, by default, the module loader decides to interpret
 it as a FTD (Falcon Template Document) if the file has a .ftd extension.

 This setting changes this behavior, and can assume one of the following values:
 - ModSpace.checkFTD_NEVER: All the source files are interpreted as falcon scripts.
 - ModSpace.checkFTD_CHECK: Source files having the .ftd extension will be compiled as FTD scripts.
 - ModSpace.checkFTD_ALWAYS: All the source files are interpreted as FTD scripts.
 */

static void get_checkFTD( const Class*, const String&, void* instance, Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   value.setInteger( static_cast<int64>(ms->modLoader()->checkFTD()) );
}

static void set_checkFTD( const Class*, const String&, void* instance, const Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   int v = checkEnumParam( value, static_cast<int>(ModLoader::e_ftd_force) );
   ms->modLoader()->checkFTD( static_cast<ModLoader::t_check_ftd>(v) );
}


/*#
 @prop useSources ModSpace
 @brief Determines if and when precompiled modules are preferred to source modules.

 When asked to load a source script, the module space can search for a pre-compiled
 version of the same module and load it instead. This property controls how this decision
 is taken, and can assume one of the following values:

 - ModSpace.useSources_NEWER: Use the precompiled .fam module if it's is newer than the source file (the default).
 - ModSpace.useSource_ALWAYS: Always use the source files, ignoring precompiled modules.
 - ModSpace.checkFTD_NEVER: Never use the source files, try to load .fam modules only.

 The result of the checkFTD_NEVER setting is also that all modules that are requested for loading
 will be interpreted as pre-compiled modules, regardless of their extension, and thus, an error will be
 raised if they fail to be deserialized as pre-compiled modules.
*/

static void get_useSources( const Class*, const String&, void* instance, Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   value.setInteger( static_cast<int64>(ms->modLoader()->useSources()) );
}

static void set_useSources( const Class*, const String&, void* instance, const Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   int v = checkEnumParam( value, static_cast<int>(ModLoader::e_us_never) );
   ms->modLoader()->useSources( static_cast<ModLoader::t_use_sources>(v) );
}

/*#
 @prop saveRemote ModSpace
 @brief Tells the ModSpace to try to save precompiled modules also on remote VFS systems.

 Can be either @b true or @b false.
*/

static void get_saveRemote( const Class*, const String&, void* instance, Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   value.setBoolean( ms->modLoader()->saveRemote() );
}

static void set_saveRemote( const Class*, const String&, void* instance, const Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   ms->modLoader()->saveRemote( value.isTrue() );
}



/*#
 @prop senc ModSpace
 @brief Source text encoding used to read the source modules.

 It's a string representing the name of the text encoding used to
 read the source files. It defaults to the source encoding used to
 load the main module of the current process.
*/

static void get_senc( const Class*, const String&, void* instance, Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   String* res = new String( ms->modLoader()->sourceEncoding());
   value = FALCON_GC_HANDLE(res);
}

static void set_senc( const Class*, const String&, void* instance, const Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   if( ! value.isString() )
   {
      throw FALCON_SIGN_XERROR(TypeError, e_inv_prop_value, .extra("S") );
   }

   const String& encoding = *value.asString();
   bool ok = ms->modLoader()->sourceEncoding( encoding );
   if( ! ok )
   {
      throw FALCON_SIGN_XERROR(ParamError, e_inv_prop_value, .extra("Unkonwn encoding " + encoding) );
   }
}


/*#
 @prop famExt ModSpace
 @brief File extension used to search and/or create pre-compiled moldules.

 It's a string representing the file extension (last part of the filename,
 after a final dot) used when reading or writing a pre-compiled module.
*/

static void get_famExt( const Class*, const String&, void* instance, Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   String* res = new String( ms->modLoader()->famExt());
   value = FALCON_GC_HANDLE(res);
}

static void set_famExt( const Class*, const String&, void* instance, const Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   if( ! value.isString() )
   {
      throw FALCON_SIGN_XERROR(TypeError, e_inv_prop_value, .extra("S") );
   }

   const String& v = *value.asString();
   ms->modLoader()->famExt( v );
}


/*#
 @prop ftdExt ModSpace
 @brief File extension used to search for Falcon Template Document source files.
*/

static void get_ftdExt( const Class*, const String&, void* instance, Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   String* res = new String( ms->modLoader()->ftdExt());
   value = FALCON_GC_HANDLE(res);
}

static void set_ftdExt( const Class*, const String&, void* instance, const Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   if( ! value.isString() )
   {
      throw FALCON_SIGN_XERROR(TypeError, e_inv_prop_value, .extra("S") );
   }

   const String& v = *value.asString();
   ms->modLoader()->ftdExt( v );
}



/*#
 @prop path ModSpace
 @brief Search path for required modules.

 The path is a semi-comma separated list of URIs.
*/

static void get_path( const Class*, const String&, void* instance, Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   String* res = new String( ms->modLoader()->getSearchPath());
   value = FALCON_GC_HANDLE(res);
}

static void set_path( const Class*, const String&, void* instance, const Item& value )
{
   ModSpace* ms = static_cast<ModSpace*>(instance);
   if( ! value.isString() )
   {
      throw FALCON_SIGN_XERROR(TypeError, e_inv_prop_value, .extra("S") );
   }

   const String& v = *value.asString();
   ms->modLoader()->setSearchPath( v );
}


namespace CModSpace {

static void internal_find_by( VMContext* ctx, bool byName, const String& name )
{
   static Class* clsModule = static_cast<Class*>(Engine::instance()->getMantra("Module"));
   fassert( clsModule != 0 );

   ModSpace* self = static_cast<ModSpace*>(ctx->self().asInst());
   Module* mod = byName? self->findByName( name ) : self->findByURI( name );
   if( mod != 0 )
   {
      mod->incref();
      ctx->returnFrame( FALCON_GC_STORE( clsModule, mod ) );
   }
   else {
      ctx->returnFrame();
   }
}

/*#
 @method findByName ModSpace
 @brief Searches a module stored in the modspace using its logical name.
 @param name The logical module name under which the required module is stored.
 @return A Module instance or nil if not found.

 This method finds a module stored in the ModSpace using the logical name
 under which is stored as a key.

 */

FALCON_DECLARE_FUNCTION(findByName, "name:S");
FALCON_DEFINE_FUNCTION_P1( findByName )
{
   Item* i_name = ctx->param(0);
   if( i_name == 0 || ! i_name->isString() )
   {
      throw paramError( __LINE__, SRC );
   }

   const String& name = *i_name->asString();
   internal_find_by(ctx, true, name);
}

/*#
 @method findByUri ModSpace
 @brief Searches a module stored in the modspace using its URI.
 @param uri The complete URI of the module.
 @return A Module instance or nil if not found.

 This method uses the physical URI (system or network path) that was
 used to load the module as a key to find the module in the ModSpace.

 The URI under which a module is stored is the one used to load it: it might
 be a relative system path in case it was loaded that way.
 */
FALCON_DECLARE_FUNCTION(findByURI, "uri:S|URI");
FALCON_DEFINE_FUNCTION_P1( findByURI )
{
   static Class* clsUri = static_cast<Class*>(Engine::instance()->getMantra("URI"));
   fassert( clsUri != 0 );

   Item* i_name = ctx->param(0);
   void* data = 0;
   Class* cls = 0;
   if( i_name == 0 )
   {
      throw paramError( __LINE__, SRC );
   }
   else if( i_name->isString() )
   {
      const String& name = *i_name->asString();
      internal_find_by(ctx, false, name);
   }
   else if( i_name->asClassInst(cls,data) && cls->isDerivedFrom(clsUri) )
   {
      URI* uri = static_cast<URI*>(data);
      String name = uri->encode();
      internal_find_by(ctx, false, name);
   }
}

static void internal_append_prepend( Function* func, VMContext* ctx, bool append )
{
   static Class* clsUri = static_cast<Class*>(Engine::instance()->getMantra("URI"));
   fassert( clsUri != 0 );
   ModSpace* self = static_cast<ModSpace*>(ctx->self().asInst());

   Item* i_name = ctx->param(0);
   void* data = 0;
   Class* cls = 0;
   if( i_name == 0 )
   {
      throw func->paramError( __LINE__, SRC );
   }
   else if( i_name->isString() )
   {
      const String& name = *i_name->asString();
      if( append ) {
         self->modLoader()->addDirectoryBack( name );
      }
      else {
         self->modLoader()->addDirectoryFront( name );
      }
   }
   else if( i_name->asClassInst(cls,data) && cls->isDerivedFrom(clsUri) )
   {
      URI* uri = static_cast<URI*>(data);
      String name = uri->encode();
      if( append ) {
         self->modLoader()->addDirectoryBack( name );
      }
      else {
         self->modLoader()->addDirectoryFront( name );
      }
   }
}

/*#
 @method appendPath ModSpace
 @brief Appends a path specification to the search path.
 @param uri An URI or a string indicating a single VFS path entry.
 */
FALCON_DECLARE_FUNCTION(appendPath, "uri:S|URI");
FALCON_DEFINE_FUNCTION_P1( appendPath )
{
   internal_append_prepend( this, ctx, true );
}


/*#
 @method prependPath ModSpace
 @brief Prepends a path specification to the search path.
 @param uri An URI or a string indicating a single VFS path entry.
 */
FALCON_DECLARE_FUNCTION( prependPath, "uri:S|URI");
FALCON_DEFINE_FUNCTION_P1( prependPath )
{
   internal_append_prepend( this, ctx, false );
}

/*#
 @method load ModSpace
 @brief Loads a module.
 @param uri An URI or a string indicating a single VFS path entry.
 */
FALCON_DECLARE_FUNCTION( load, "uri:S|URI,isUri:[B],asLoad:[B],asMain:[B]");
FALCON_DEFINE_FUNCTION_P1( load )
{
   static PStep* step = &Engine::instance()->stdSteps()->m_returnFrameWithTop;
   static Class* clsUri = static_cast<Class*>(Engine::instance()->getMantra("URI"));
   fassert( clsUri != 0 );


   Item* i_name = ctx->param(0);
   Item* i_isUri = ctx->param(1);
   Item* i_asLoad = ctx->param(2);
   Item* i_asMain = ctx->param(3);

   bool isUri = i_isUri != 0 ? i_isUri->isTrue() : false;
   bool asLoad = i_asLoad != 0 ? i_asLoad->isTrue() : false;
   bool asMain = i_asMain != 0 ? i_asMain->isTrue() : true;

   Class* cls = 0;
   void* data = 0;
   ModSpace* self = static_cast<ModSpace*>(ctx->self().asInst());

   if( i_name == 0 )
   {
      throw paramError( __LINE__, SRC );
   }
   else if( i_name->isString() )
   {
      ctx->pushCode( step );
      self->loadModuleInContext(*i_name->asString(), isUri, asLoad, asMain, ctx, module(), true );
   }
   else if( i_name->asClassInst(cls,data) && cls->isDerivedFrom(clsUri) )
   {
      ctx->pushCode( step );
      URI* uri = static_cast<URI*>(data);
      self->loadModuleInContext(uri->encode(), isUri, asLoad, asMain, ctx, module(), true );
   }

   // don't return the frame, the return step will do.
}

}


ClassModSpace::ClassModSpace():
         Class("ModSpace")
{
   addProperty( "savePC", &get_savePC, &set_savePC );
   addProperty( "checkFTD", &get_checkFTD, &set_checkFTD );
   addProperty( "useSources", &get_useSources, &set_useSources );
   addProperty( "saveRemote", &get_saveRemote, &set_saveRemote );
   addProperty( "senc", &get_senc, &set_senc );
   addProperty( "famExt", &get_famExt, &set_famExt );
   addProperty( "ftdExt", &get_ftdExt, &set_ftdExt );
   addProperty( "path", &get_path, &set_path );

   addConstant( "savePC_NEVER", static_cast<int64>(ModLoader::e_save_no) );
   addConstant( "savePC_TRY", static_cast<int64>(ModLoader::e_save_try) );
   addConstant( "savePC_MANDATORY", static_cast<int64>(ModLoader::e_save_mandatory) );

   addConstant( "checkFTD_NEVER", static_cast<int64>(ModLoader::e_ftd_ignore) );
   addConstant( "checkFTD_CHECK", static_cast<int64>(ModLoader::e_ftd_check) );
   addConstant( "checkFTD_ALWAYS", static_cast<int64>(ModLoader::e_ftd_force) );

   addConstant( "useSources_NEWER", static_cast<int64>(ModLoader::e_us_newer) );
   addConstant( "useSource_ALWAYS", static_cast<int64>(ModLoader::e_us_always) );
   addConstant( "checkFTD_NEVER", static_cast<int64>(ModLoader::e_us_never) );

   addMethod( new CModSpace::Function_findByName );
   addMethod( new CModSpace::Function_findByURI );
   addMethod( new CModSpace::Function_appendPath );
   addMethod( new CModSpace::Function_prependPath );

   addMethod( new CModSpace::Function_load );
}

ClassModSpace::~ClassModSpace()
{
}

void* ClassModSpace::createInstance() const
{
   return 0;
}

void ClassModSpace::dispose( void* instance ) const
{
   ModSpace* spc = static_cast<ModSpace*>(instance);
   spc->decref();
}

void* ClassModSpace::clone( void* instance ) const
{
   ModSpace* spc = static_cast<ModSpace*>(instance);
   spc->incref();
   return spc;
}

void ClassModSpace::gcMarkInstance( void* instance, uint32 mark ) const
{
   ModSpace* spc = static_cast<ModSpace*>(instance);
   spc->gcMark(mark);
}

bool ClassModSpace::gcCheckInstance( void* instance, uint32 mark ) const
{
   ModSpace* spc = static_cast<ModSpace*>(instance);
   return spc->currentMark() >= mark;
}
   
}

/* end of classmodspace.cpp */
