/*
   FALCON - The Falcon Programming Language.
   FILE: error.h

   Error class.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Fri, 04 Feb 2011 18:39:36 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/


#ifndef FALCON_ERROR_H
#define	FALCON_ERROR_H

#include <falcon/setup.h>
#include <falcon/types.h>
#include <falcon/string.h>
#include <falcon/item.h>
#include <falcon/enumerator.h>
#include <falcon/tracestep.h>

namespace Falcon {

class Error;
class Class;
class Error_p;

// Declare the error IDS
#define FLC_DECLARE_ERROR_TABLE
#include <falcon/error_messages.h>
#undef FLC_DECLARE_ERROR_TABLE


/** Error Parameter class.
   This class provides the main Error class and its subclasses with named parameter idiom.
   Errors have many parameters and their configuration is bourdensome and also a big
   "bloaty" exactly in spots when one would want code to be small.

   This class, completely inlined, provides the compiler and the programmer with a fast
   and easy way to configure the needed parameters, preventing the other, unneded details
   from getting into the way of the coders.

   The Error class (and its subclasses) has a constructor accepting an ErrorParameter
   by reference.
   \code
      Error *e = new SomeKindOfError( ErrorParam( ... ).p1().p2()....pn() )
   \endcode

   is an acceptable grammar to create an Error.
*/

class ErrorParam
{

public:
   typedef enum {
      e_orig_unknown = 0,
      e_orig_compiler = 1,
      e_orig_assembler = 2,
      e_orig_loader = 3,
      e_orig_vm = 4,
      e_orig_script = 5,
      e_orig_runtime = 9,
      e_orig_mod = 10
   } t_origin;

   /** Standard constructor.
      In the constructor a source line may be provided. This makes possible to use the
      __LINE__ ansi C macro to indicate the point in the source C++ file where an error
      is raised.

    Similarly, the file is parameter can be set to __FILE__.

      \param code error code.
      \param file the file where the error is raised.
      \param line optional line where error occurs.
   */
   ErrorParam( int code, uint32 line = 0, const char* file = 0 ):
      m_errorCode( code ),
      m_line( line ),
      m_module( file == 0 ? "" : file, String::npos ),    // force buffering
      m_sysError( 0 ),
      m_origin( e_orig_mod ),
      m_catchable( true )
      {}

   ErrorParam &code( int code ) { m_errorCode = code; return *this; }
   ErrorParam &desc( const String &d ) { m_description = d; return *this; }
   ErrorParam &extra( const String &e ) { m_extra.bufferize(e); return *this; }
   ErrorParam &symbol( const String &sym ) { m_symbol = sym; return *this; }
   ErrorParam &module( const String &mod ) { m_module = mod; return *this; }
   ErrorParam &line( uint32 line ) { m_line = line; return *this; }
   ErrorParam &sysError( uint32 e ) { m_sysError = e; return *this; }
   ErrorParam &origin( t_origin orig ) { m_origin = orig; return *this; }
   ErrorParam &hard() { m_catchable = false; return *this; }

private:
   friend class Error;

   int m_errorCode;
   String m_description;
   String m_extra;
   String m_symbol;
   String m_module;

   uint32 m_line;
   uint32 m_sysError;

   t_origin m_origin;
   bool m_catchable;
};


/** The Error class.
   This class implements an error instance.
   Errors represent problems occoured both during falcon engine operations
   (i.e. compilation syntax errors, link errors, file I/O errors, dynamic
   library load errors ands o on) AND during runtime (i.e. VM opcode
   processing errors, falcon program exceptions, module function errors).

   When an error is raised by an engine element whith this capability
   (i.e. the compiler, the assembler, the runtime etc.), it is directly
   passed to the error handler, which has the duty to do something with
   it and eventually destroy it.

   When an error is raised by a module function with the VMachine::raiseError()
   method, the error is stored in the VM; if the error is "catchable" AND it
   occours inside a try/catch statement, it is turned into a Falcon Error
   object and passed to the script.

   When a script raises an error both explicitly via the "raise" function or
   by performing a programming error (i.e. array out of bounds), if there is
   a try/catch block at work the error is turned into a Falcon error and
   passed to the script.

   If there isn't a try/catch block or if the error is raised again by the
   script, the error instance is passed to the VM error handler.

   Scripts may raise any item, which may not necessary be Error instances.
   The item is then copied in the m_item member and passed to the error
   handler.
*/

class FALCON_DYN_CLASS Error
{
public:

   /** Enumerator for trace steps. 
    @see enumerateSteps
   */
   typedef Enumerator<TraceStep> StepEnumerator;

   /** Enumerator for sub-errors.
    @see enumerateSuberrors
    */
   typedef Enumerator<Error*> ErrorEnumerator;

   /** Sets the error code.
    \param ecode an error ID.
    */
   void errorCode( int ecode ) { m_errorCode = ecode; }
   /** Sets the system error code.

    Many errors are raised after system errors in I/O operations.
    This is a useful fields that avoids the need to recast or use ad-hoc
    strucures for I/O or system related errors.
    \param ecode The system error that caused this error to be raised.
    */
   void systemError( uint32 ecode ) { m_sysError = ecode; }
   void errorDescription( const String &errorDesc ) { m_description = errorDesc; }
   void extraDescription( const String &extra ) { m_extra = extra; }
   void module( const String &moduleName ) { m_module = moduleName; }
   void symbol( const String &symbolName )  { m_symbol = symbolName; }
   void line( uint32 line ) { m_line = line; }
   void origin( ErrorParam::t_origin o ) { m_origin = o; }
   void catchable( bool c ) { m_catchable = c; }
   void raised( const Item &itm ) { m_raised = itm; }

   int errorCode() const { return m_errorCode; }
   uint32 systemError() const { return m_sysError; }
   const String &errorDescription() const { return m_description; }
   const String &extraDescription() const { return m_extra; }
   const String &module() const { return m_module; }
   const String &symbol() const { return m_symbol; }
   uint32 line() const { return m_line; }
   ErrorParam::t_origin origin() const { return m_origin; }
   bool catchable() const { return m_catchable; }
   const Item &raised() const { return m_raised; }

    /** Renders the error to a string.
    */
   String describe() const { String temp; describe( temp ); return temp; }

   /** Renders the error to a string.
    */
   virtual void describe( String &target ) const;

   /** Writes only the heading of the error to the target string.
      The error heading is everything of the error without the traceback.
      This method never recurse on error lists; only the first heading is returned.
      \note the input target string is not cleared; error contents are added at
         at the end.
      \note The returned string doesn't terminate with a "\n".
   */
   virtual String &heading( String &target ) const;

   /** Adds a sub-error to this error.

    Some errors store multiple errors that cause a more general error condition.
    For example, a compilation may fail due to multiple syntax errors. This fact
    is represented by raising a CompileError which contains all the errors that
    caused the compilation to fail.

    */
   void appendSubError( Error *sub );

   /** Creates a falcon instance that may be used directly by a script.

    The error is referenced and stored in the data field of the item, and
    the handler class is set to the scriptClass that was set when creating
    the instance.

    This makes the item immediately useable from the script.
   */
   void scriptize( Item& tgt );

   Class* handler() const { return m_handler; }

   /** Adds a trace step to this error.
    This method adds a tracestep that lead to the place where the error
    was raised.

    Errors raised outside a script execution may be without trace steps.
    */
   void addTrace( const TraceStep& step );

   /** Enumerate the traceback steps.
    \param rator A StepEnumerator that is called back with each step in turn.
    */
   void enumerateSteps( StepEnumerator &rator ) const;

   /** Enumerate the sub-errors.
    \param rator A ErrorEnumerator that is called back with each sub-error in turn.
    \see appendSubError
    */
   void enumerateErrors( ErrorEnumerator &rator ) const;

   /** Return the name of this error class.
    Set in the constructcor.
    */
   const String &className() const;
   
   /** Gets the first sub-error.
    Some errors are used to wrap a single lower level error. For example,
    a virtual machine may be terminated because an error was raised and
    nothing caught it; in that case, the termination error is UncaughtError,
    and it will "box" the error that was raised originally inside the script.

    This is called error "boxing". This method allows to access the first
    sub-error, that may be the boxed error, without the need to setup an
    enumerator callback.
    \see appendSubError.
    \return The boxed error, or 0 if this error isn't boxing anything.
    */
   Error* getBoxedError() const;

   /** Return true if this error has been filled with a traceback.*/
   bool hasTraceback() const;

   /** Increment the reference count of this object */
   void incref() const;

   /** Decrements the reference count of this object.
    The error must be considered invalid after this call.
    */
   void decref();

protected:

   /** Minimal constructor.
      If the description is not filled, the toString() method will use the default description
      for the given error code.
   */
   Error( Class* handler, const ErrorParam &params );

   mutable int32 m_refCount;

   int m_errorCode;
   String m_description;
   String m_extra;
   String m_symbol;
   String m_module;
   String m_className;
   Class* m_handler;

   uint32 m_line;
   uint32 m_sysError;

   ErrorParam::t_origin m_origin;
   bool m_catchable;
   Item m_raised;

protected:
   /** Private destructor.
      Can be destroyed only via decref.
   */
   virtual ~Error();

private:
   Error_p* _p;
};

}

#endif	/* FALCON_ERROR_H */

/* end of error.h */