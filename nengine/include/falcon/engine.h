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
#define	_FALCON_ENGINE_H_

#include <falcon/setup.h>
#include <falcon/itemid.h>
#include <falcon/string.h>
#include <falcon/vfsiface.h>

#include "vfsprovider.h"
#include "vfsiface.h"

namespace Falcon
{
class Class;
class Collector;
class Mutex;
class Transcoder;
class TranscoderMap;

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
class FALCON_DYN_SYM Engine
{
public:

   /** Initializes the Falcon subsystem. */
   static void init();

   /** Terminates the Falcon subsystem. */
   static void shutdown();

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
   Collector* collector() const;

   //==========================================================================
   // Type handlers
   //

   /** Returns the global instance of the Function class.
   \return the Engine instance of the Function Class (handler).

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* functionClass() const;

   /** Returns the global instance of the String class.
   \return the Engine instance of the String Class (handler).

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* stringClass() const;

   /** Returns the global instance of the Array class.
   \return the Engine instance of the Array Class (handler).

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* arrayClass() const;

   //==========================================================================
   // Error handlers
   //

   /** Returns the global instance of the CodeError class.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* codeErrorClass() const;

   /** Returns the global instance of the GenericError class.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* genericErrorClass() const;

   /** Returns the global instance of the OperandError class.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* operandErrorClass() const;

   /** Returns the global instance of the UnsupportedError class.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* unsupportedErrorClass() const;

   /** Returns the global instance of the IOError class.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* ioErrorClass() const;

   /** Returns the global instance of the InterruptedError class.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* interruptedErrorClass() const;

   /** Returns the global instance of the EncodingError class.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* encodingErrorClass() const;

   /** Returns the global instance of the SyntaxError class.

    Method init() must have been called before.

    @note This method will assert and terminate the program if compiled in debug mode
    in case the engine has not been initialized. In release, it will just
    return a null pointer.
    */
   Class* syntaxErrorClass() const;

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

   VFSIface& vsf() { return m_vfs; }
   const VFSIface& vsf() const { return m_vfs; }

protected:
   Engine();
   ~Engine();

   static Engine* m_instance;
   Mutex* m_mtx;
   Collector* m_collector;
   Class* m_classes[FLC_ITEM_COUNT];

   VFSIface m_vfs;
   //===============================================
   // Global settings
   //
   bool m_bWindowsNamesConversion;

   //===============================================
   // Global type handlers
   //
   Class* m_functionClass;
   Class* m_stringClass;
   Class* m_arrayClass;

   //===============================================
   // Standard error handlers
   //
   Class* m_codeErrorClass;
   Class* m_genericErrorClass;
   Class* m_operandErrorClass;
   Class* m_unsupportedErrorClass;
   Class* m_ioErrorClass;
   Class* m_interruptedErrorClass;
   Class* m_encodingErrorClass;
   Class* m_syntaxErrorClass;


   //===============================================
   // Transcoders
   //
   TranscoderMap* m_tcoders;
};

}

#endif	/* _FALCON_ENGINE_H_ */

/* end of engine.h */