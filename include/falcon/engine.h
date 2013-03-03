/*
   FALCON - The Falcon Programming Language.
   FILE: engine.h

   Global variables known by the falcon System.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 13 Feb 2011 12:25:12 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#ifndef _FALCON_ENGINE_H_
#define _FALCON_ENGINE_H_

#include <falcon/setup.h>
#include <falcon/itemid.h>
#include <falcon/string.h>
#include <falcon/vfsiface.h>

#include <falcon/vfsprovider.h>
#include <falcon/mantra.h>

namespace Falcon
{
class Mutex;
class Transcoder;

class TranscoderMap;
class PredefMap;
class MantraMap;

class Class;
class BOM;
class StdSteps;
class StdHandlers;
class StdErrors;
class SynClasses;

class Collector;
class PoolList;
class Pool;
class GCLock;
class Symbol;
class SymbolPool;

class Module;
class ModSpace;
class Item;

class VMContext;

class StdStreamTraits;
class StdHandlers;
class StdErrors;

class Log;

/** Falcon application global data.

 This class stores the gloal items that must be known by the falcon engine
 library, and starts the subsystems needed by Falcon to handle application-wide
 objects.

 An application is required to call Engine::init() method when the falcon engine
 is first needed, and to call Engine::shutdown() before exit.

 Various assert points are available in debug code to check for correct
 initialization sequence, but they will be removed in release code, so if the
 engine is not properly initialized you may expect random crashes in release
 (right near the first time you use Falcon stuff).

 @note init() and shutdown() code are not thread-safe. Be sure to invoke them
 in a single-thread context.
 
 */
class FALCON_DYN_CLASS Engine
{
public:

   /** Initializes the Falcon subsystem. */
   static void init();

   /** Terminates the Falcon subsystem. */
   static void shutdown();

   /** Terminates the program NOW with an error message.
    \param msg The message that will be displayed on termination.
    */
   static void die( const String& msg );

   /** Returns the current engine instance.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   static Engine* instance();

   //==========================================================================
   // Global Settings
   //

   /** Return the class handing the base type reflected by this item type ID.
    \param type The type of the items for which the handler class needs to be known.
    \return The handler class, or 0 if the type ID is not defined or doesn't have an handler class.

    This method returns the class that, if provided in a User or Deep item as
    handler class, would cause the item to be treated correctly.
    */
   Class* getTypeClass( int type );
      
   /** True when running on windows system.
    
    File naming convention and other details are different on windows systems.
    */
   bool isWindows() const;

   //==========================================================================
   // Global Objects
   //

   /** The global collector.
    */
   static Collector* collector();

   static GCToken* GC_store( const Class* cls, void* data );
   static GCLock* GC_storeLocked( const Class* cls, void* data );
#ifdef FALCON_TRACE_GC
   static GCToken* GC_H_store( const Class* cls, void* data, const String& src, int line );
   static GCLock* GC_H_storeLocked( const Class* cls, void* data, const String& src, int line );
#endif
   static GCLock* GC_lock( const Item& item );
   static void GC_unlock( GCLock* lock );


   /**
    * The global handler class collection.
    */
   StdHandlers* stdHandlers() const { return m_stdHandlers; }

   /**
    * The global handler class collection.
    */
   static StdHandlers* handlers();

   /**
    * The global handler class collection.
    */
   StdStreamTraits* stdStreamTraits() const { return m_stdStreamTraits; }

   /**
    * The global handler class collection.
    */
   static StdStreamTraits* streamTraits();



   /** Returns the collection of standard syntactic tree classes.
   \return the Engine instance of the SynClasses class collection.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   SynClasses* synclasses() const;
   
   /** Returns the standard collection of error handlers.
    */
   StdErrors* stdErrors() const { return m_stdErrors; }

   /** Adds a transcoder to the engine.
    \param A new transcoder to be registered in the engine.
    \return true if the transcoder can be added, false if it was already registered.

    The owenrship of the transcoder is passed to the engine. The transcoder will
    be destroyed at engine destruction.
   */

   bool addTranscoder( Transcoder* enc );

   /** Gets a transcoder.
    \param name The ANSI encoding name of the encoding served by the desired transcoder.
    \return A valid transcoder pointer or 0 if the encoding is unknown.
    */
   Transcoder* getTranscoder( const String& name );

   /* Get a transcoder that will serve the current system text encoding.
    \param bDefault if true, a "C" default transcoder will be returned incase the
           system encoding cannot be found
    \return A valid transcoder pointer or 0 if the encoding is unknown.

    TODO
    */
   //Transcoder* getSystemTranscoder( bool bDefault = false );

   VFSIface& vfs() { return m_vfs; }
   const VFSIface& vfs() const { return m_vfs; }

   /** Returns an instance of the core module that is used as read-only.
    \return A singleton instance of the core module.
    
    This instance is never linked in any VM and is used as a resource for
    undisposeable function (as i.e. the BOM methods).
    */
   Module* getCore() const { return m_core; }

   /** Returns the Basic Object Model method collection.
    \return A singleton instance of the core BOM handler collection.
   */
   BOM* getBom() const { return m_bom; }

   /** Archive of standard steps. */
   StdSteps* stdSteps() const { return m_stdSteps; }
   
   
   /** Adds a builtin item.
    @param name The name under which the builtin is known.
    @param value The value that is to be published to the scripts.
    
    */
   bool addBuiltin( const String& name, const Item& value );
   
   /** Gets a pre-defined built-in value. 
    */
   const Item* getBuiltin( const String& name ) const;
   
   /** Centralized repository of publically available classes and functions.
    @param reg The mantra to be stored.
    \return True if the mantra name wasn't already registered, false otherwise.
    
    A Mantra is a invocation for the engine that is known by name. Usually
    it's a Class or a Function.
    
    This method controls a central repository for Mantras that are not provided
    by modules (i.e. built in). are not created by modules. It's the case of classes
    and functions provided by the engine or by embedding applications.
    
    Mantras stored in the engine class register are known only by name, and they
    must have a well-known system-wide uinque name. For instance, "Integer",
    "String" and so on. Class having a module might be added to the register;
    in that case, they will be known by their full ID (module logical name
    preceding their name, separated by a dot). 
    
    However, there isn't any way to unregister a class, so, when registering
    a class stored in a module, the caller must be sure that the module is static
    and will not be destroyed until the end of the process.
    
    The class register is not owning the stored classes; this is just a
    dictionary of well known classes that are not directly accessible in modules
    that a VM may be provided with. 
    
    \note This method automatically calls addBuiltin to create a item visible
    by the scripts. In other words, registered mantras become script-visible.
    
    \note Mantras added this way are owned by the engine; the engine destroys
    them at destruction.
    */
   bool addMantra( Mantra* reg );
   
   /** Gets a previously registered mantra by name.
    @param name The name or full class ID of the desired class.
    @param cat A category to filter out undesired mantras.
    @return The mantra if it had been registered, 0 if the name is not found.
    
    @see addMantra
    */
   Mantra* getMantra( const String& name, Mantra::t_category cat = Mantra::e_c_none ) const;

   /** Set the context run by this thread.
    \param ctx The context being run.
    Each time the VM changes the running context in the current VM execution
    thread, this method is called to update the global visibility of the context.
    */
   void setCurrentContext( VMContext* ctx );
   
   /** Adds an object-specific memory pool.
    
    The pool will be destroyed at engine exit.
    */
   void addPool( Pool* p );

   /** Returns a pointer to the base dynsymbol.
    
    The base symbol is a symbol that is inserted
    at the head of each evaluation frame to save the corresponding
    data stack frame.
    */
   Symbol* baseSymbol() const;
   
   /** Returns a pointer to the rule base dynsymbol.

    The base symbol is a symbol that is inserted
    at the head of each rule frame to save the corresponding
    data stack frame.
    */
   Symbol* ruleBaseSymbol() const;

   /** Returns a pointer to a local or global symbol.
     Each call to this function increases the reference of the
     retrieved symbol.
    */
   static Symbol* getSymbol( const String& name );
   static Symbol* getSymbol( const String& name, bool& isFirst );
   static void refSymbol(Symbol* sym);
   static void releaseSymbol( Symbol* sym );

   Log* log() const;

protected:
   Engine();
   ~Engine();

   static Engine* m_instance;
   Mutex* m_mtx;
   Collector* m_collector;
   Log* m_log;
   Class* m_classes[FLC_ITEM_COUNT];

   VFSIface m_vfs;
   //===============================================
   // Global settings
   //
   bool m_bWindowsNamesConversion;
   
   //===============================================
   // Standard error handlers
   //
   Class* m_accessErrorClass;
   Class* m_accessTypeErrorClass;
   Class* m_codeErrorClass;
   Class* m_genericErrorClass;
   Class* m_operandErrorClass;
   Class* m_unsupportedErrorClass;
   Class* m_ioErrorClass;
   Class* m_interruptedErrorClass;
   Class* m_encodingErrorClass;
   Class* m_syntaxErrorClass;
   Class* m_paramErrorClass;

   
   SynClasses* m_synClasses;
   
   //===============================================
   // Transcoders
   //
   TranscoderMap* m_tcoders;
   
   //===============================================
   // Pools
   //
   PoolList* m_pools;
   SymbolPool* m_symbols;

   //===============================================
   // The core module.
   //
   Module* m_core;
   BOM* m_bom;

   MantraMap* m_mantras;
   PredefMap* m_predefs;
   
   StdSteps* m_stdSteps;
   StdErrors* m_stdErrors;
   StdHandlers* m_stdHandlers;
   StdStreamTraits* m_stdStreamTraits;

   Symbol* m_baseSymbol;
   Symbol* m_ruleBaseSymbol;
};

}

#endif	/* _FALCON_ENGINE_H_ */

/* end of engine.h */
