/*
   FALCON - The Falcon Programming Language
   FILE: engine.cpp

   Engine static/global data setup and initialization
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 13 Feb 2011 12:39:16 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#include <map>

#include <falcon/trace.h>
#include <falcon/engine.h>

#include <falcon/setup.h>
#include <falcon/fassert.h>
#include <falcon/string.h>
#include <falcon/mt.h>

//--- Virtual file systems ---
#include <falcon/vfs_file.h>

//--- standard transcoder headers ---

#include <falcon/transcoderc.h>
#include <falcon/transcoderutf8.h>

//--- object headers ---

#include <falcon/collector.h>

//--- type headers ---
#include <falcon/corefunction.h>
#include <falcon/corenil.h>
#include <falcon/coreint.h>
#include <falcon/corestring.h>
#include <falcon/corearray.h>

//--- error headers ---
#include <falcon/errorclass.h>
#include <falcon/codeerror.h>
#include <falcon/genericerror.h>
#include <falcon/interruptederror.h>
#include <falcon/ioerror.h>
#include <falcon/operanderror.h>
#include <falcon/unsupportederror.h>
#include <falcon/syntaxerror.h>

#include <falcon/paranoid.h>
#include <map>

#include "falcon/encodingerror.h"
#include "falcon/vfs_file.h"

namespace Falcon
{


//=======================================================
// Private classes known by the engine -- Utils
//

class TranscoderMap: public std::map<String, Transcoder*>
{
};

//=======================================================
// Private classes known by the engine
//

class CodeErrorClass: public ErrorClass
{
public:
   CodeErrorClass():
      ErrorClass("CodeError")
   {}

   virtual void* create(void* creationParams ) const
   {
      return new CodeError( *static_cast<ErrorParam*>(creationParams) );
   }
};

class GenericErrorClass: public ErrorClass
{
public:
   GenericErrorClass():
      ErrorClass( "GenericError" )
      {}

   virtual void* create(void* creationParams ) const
   {
      return new GenericError( *static_cast<ErrorParam*>(creationParams) );
   }
};

class InterruptedErrorClass: public ErrorClass
{
public:
   InterruptedErrorClass():
      ErrorClass( "InterruptedError" )
      {}

   virtual void* create(void* creationParams ) const
   {
      return new InterruptedError( *static_cast<ErrorParam*>(creationParams) );
   }
};


class IOErrorClass: public ErrorClass
{
public:
   IOErrorClass():
      ErrorClass( "IOError" )
      {}

   virtual void* create(void* creationParams ) const
   {
      return new IOError( *static_cast<ErrorParam*>(creationParams) );
   }
};


class OperandErrorClass: public ErrorClass
{
public:
   OperandErrorClass():
      ErrorClass( "OperandError" )
      {}

   virtual void* create(void* creationParams ) const
   {
      return new OperandError( *static_cast<ErrorParam*>(creationParams) );
   }
};


class UnsupportedErrorClass: public ErrorClass
{
public:
   UnsupportedErrorClass():
      ErrorClass( "UnsupportedError" )
      {}

   virtual void* create(void* creationParams ) const
   {
      return new UnsupportedError( *static_cast<ErrorParam*>(creationParams) );
   }
};


class EncodingErrorClass: public ErrorClass
{
public:
   EncodingErrorClass():
      ErrorClass( "EncodingError" )
      {}

   virtual void* create(void* creationParams ) const
   {
      return new EncodingError( *static_cast<ErrorParam*>(creationParams) );
   }
};

class SyntaxErrorClass: public ErrorClass
{
public:
   SyntaxErrorClass():
      ErrorClass( "SyntaxError" )
      {}

   virtual void* create(void* creationParams ) const
   {
      return new SyntaxError( *static_cast<ErrorParam*>(creationParams) );
   }
};

//=======================================================
// Engine static declarations
//

Engine* Engine::m_instance = 0;

//=======================================================
// Engine implementation
//

Engine::Engine()
{
   TRACE("Engine creation started", 0 )
   #ifdef FALCON_SYSTEM_WIN
   m_bWindowsNamesConversion = true;
   #else
   m_bWindowsNamesConversion = false;
   #endif

   m_mtx = new Mutex;
   m_collector = new Collector;


   //=====================================
   // Standard file systems.
   //
   m_vfs.addVFS("", new VFSFile );
   m_vfs.addVFS("file", new VFSFile );


   //=====================================
   // Initialization of standard deep types.
   //
   m_functionClass = new CoreFunction;
   m_stringClass = new CoreString;
   m_arrayClass = new CoreArray;

   // Initialization of the class vector.
   m_classes[FLC_ITEM_NIL] = new CoreNil;
   m_classes[FLC_ITEM_BOOL] = new CoreNil;
   m_classes[FLC_ITEM_INT] = new CoreInt;
   m_classes[FLC_ITEM_NUM] = new CoreNil;
   m_classes[FLC_ITEM_USER] = 0;
   m_classes[FLC_ITEM_FRAMING] = 0;
   m_classes[FLC_ITEM_FUNC] = m_functionClass;
   m_classes[FLC_ITEM_METHOD] = new CoreNil;
   m_classes[FLC_ITEM_BASEMETHOD] = new CoreNil;
   m_classes[FLC_ITEM_DEEP] = 0;

   //=====================================
   // Initialization of standard errors.
   //
   m_codeErrorClass = new CodeErrorClass;
   m_genericErrorClass = new GenericErrorClass;
   m_interruptedErrorClass = new InterruptedErrorClass;
   m_ioErrorClass = new IOErrorClass;
   m_operandErrorClass = new OperandErrorClass;
   m_unsupportedErrorClass = new UnsupportedErrorClass;
   m_encodingErrorClass = new EncodingErrorClass;
   m_syntaxErrorClass = new SyntaxErrorClass;

   //=====================================
   // Adding standard transcoders.
   //

   m_tcoders = new TranscoderMap;
   addTranscoder( new TranscoderC );
   addTranscoder( new TranscoderUTF8 );

   TRACE("Engine creation complete", 0 )
}


Engine::~Engine()
{
   TRACE("Engine destruction started", 0 )

   delete m_mtx;
   delete m_collector;
   delete m_stringClass;
   delete m_arrayClass;

   // ===============================
   // Delete standard error classes
   //
   delete m_codeErrorClass;
   delete m_genericErrorClass;
   delete m_ioErrorClass;
   delete m_interruptedErrorClass;
   delete m_operandErrorClass;
   delete m_unsupportedErrorClass;
   delete m_encodingErrorClass;

   // ===============================
   // Delete standard item classes
   //
   for ( int count = 0; count < FLC_ITEM_COUNT; ++count )
   {
      delete m_classes[count];
   }

   // ===============================
   // delete registered transcoders
   //

   {
      TranscoderMap::iterator iter = m_tcoders->begin();
      while( iter != m_tcoders->end() )
      {
         delete iter->second;
         ++iter;
      }
   }

   delete m_tcoders;
   TRACE("Engine destroyed", 0 )
}

void Engine::init()
{
   TRACE("Engine init()", 0 )
   fassert( m_instance == 0 );
   if( m_instance == 0 )
   {
      m_instance = new Engine;

      // TODO
      // m_instance->collector()->start();
   }
}

void Engine::shutdown()
{
   TRACE("Engine shutdown started", 0 )
   fassert( m_instance != 0 );
   if( m_instance != 0 )
   {
      // TODO
      // m_instance->collector()->start();

      delete m_instance;
      m_instance = 0;
      TRACE("Engine shutdown complete", 0 )
   }
}

//=====================================================
// Global settings
//

bool Engine::isWindows() const
{
   fassert( m_instance != 0 );
   return m_instance->m_bWindowsNamesConversion;
}


//=====================================================
// Transcoding
//

bool Engine::addTranscoder( Transcoder* ts )
{
   m_mtx->lock();
   TranscoderMap::iterator iter = m_tcoders->find(ts->name());
   if ( iter != m_tcoders->end() )
   {
      m_mtx->unlock();
      return false;
   }

   (*m_tcoders)[ts->name()] = ts;
   m_mtx->unlock();
   return true;
}


Transcoder* Engine::getTranscoder( const String& name )
{
   m_mtx->lock();
   TranscoderMap::iterator iter = m_tcoders->find(name);
   if ( iter == m_tcoders->end() )
   {
      m_mtx->unlock();
      return 0;
   }

   Transcoder* ret = iter->second;
   m_mtx->unlock();
   return ret;
}

//=====================================================
// Global objects
//

Engine* Engine::instance()
{
   fassert( m_instance != 0 );
   return m_instance;
}


 
Collector* Engine::collector() const
{
   fassert( m_instance != 0 );
   return m_instance->m_collector;
}

//=====================================================
// Type handlers
//

Class* Engine::getTypeClass( int type )
{
   PARANOID("type out of range", (type < FLC_ITEM_DEEP && type != FLC_ITEM_USER) );
   return m_classes[type];
}

Class* Engine::functionClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_functionClass;
}


Class* Engine::stringClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_stringClass;
}

Class* Engine::arrayClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_arrayClass;
}

//=====================================================
// Error handlers
//

Class* Engine::codeErrorClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_codeErrorClass;
}

Class* Engine::genericErrorClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_genericErrorClass;
}

Class* Engine::ioErrorClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_ioErrorClass;
}

Class* Engine::interruptedErrorClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_interruptedErrorClass;
}

Class* Engine::encodingErrorClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_encodingErrorClass;
}

Class* Engine::syntaxErrorClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_syntaxErrorClass;
}


Class* Engine::operandErrorClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_operandErrorClass;
}

Class* Engine::unsupportedErrorClass() const
{
   fassert( m_instance != 0 );
   return m_instance->m_unsupportedErrorClass;
}

}

/* end of engine.cpp */