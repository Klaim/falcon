/*
   FALCON - The Falcon Programming Language.
   FILE: parser/parser.h

   Parser subsystem main class.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sat, 09 Apr 2011 17:36:38 +0200

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#ifndef _FALCON_PARSER_PARSER_H_
#define	_FALCON_PARSER_PARSER_H_

#include <falcon/setup.h>
#include <falcon/string.h>
#include <falcon/enumerator.h>

// Include all to simplify child subclasses.
#include <falcon/parser/tokeninstance.h>
#include <falcon/parser/terminal.h>
#include <falcon/parser/nonterminal.h>
#include <falcon/parser/rule.h>
#include <falcon/parser/state.h>

namespace Falcon {

class GenericError;
class Error;

namespace Parsing {

class Lexer;

/** Generic Falcon Parser class.

 A parser scan a text-oriented input stream applying a grammar in order to:
 - obtain an output.
 - modify a forming context.

 Falcon parser matches against an input stream that is pre-tokenized through a
 Lexer class which feeds back a continuous stream of TokenInstance which contain
 a Token definition and possibly a concrete value that the token assumes.

 The forming stream of tokens is matched against NonTerminal tokens which are
 organized in set of State instances; each state represents a list of
 alternative NonTerminal tokens that may be found in the input sequence.

 Each NonTerminal uses one or more Rule instacnes that describe various
 alternatives under which the given NonTerminal token is formed. Eventually,
 Terminal tokens or Literal tokens (tokens having an univoque value) which can
 be generated by the lexer, describe the NonTerminal tokens in terms of atomic
 input units.

 Each rule has an Apply function attached; when a rule is selected, that is, a
 sequence of tokens (either Terminal or NonTerminal) is found to be consisted with
 an higher-level NonTerminal token, the Apply function is applied, and the Rule
 may:
 - Transform the parsed tokens in a single higher level token, usually the token
 representing the NonTerminal exposing the rule, syntethizing a value out of the
 values that are attached to each token.
 - Push or pop a parser state.
 - Alter the forming context.
 - Signal error.

 */
class FALCON_DYN_CLASS Parser
{
public:
   /** Generates the parser.
    */
   Parser();
   virtual ~Parser();

   /** Adds a state to the known parser states.
    */
   void addState( State& state );

   /** Pushes a state.
    */
   void pushState( const String& name , bool notify = true);

   typedef void (*StateFrameFunc)(void*);

   /** Pusing a state, adding a callback function for the frame.
    */
   void pushState( const String& name, Parser::StateFrameFunc cf, void* data=0 );

   /** Method to be called from pushState. Must be overriden in a child class.
    */

   virtual void onPushState( bool isPushedState )=0;
   virtual void onPopState()=0;

   /** Re-enables previous state.
   */
   void popState();

   /** Start the parsing process.
      \param mainState The state from which starting the parsing.
      \return true on success, false on error.
      \throw CodeError if the main state is not found, or if a lexer is not ready.

    Call this method after having called addState() and pushLexer().

    This method can be called multiple times, provided that all the lexers that
    might have been previously pushed are popped.
    */
   bool parse( const String& mainState );

   /** Class used to store information about detected errors.

    This class is used by the parser to store persistent information about
    the errors detected while parsing a file.

    In case there are errors in the
    */
   class ErrorDef
   {
   public:
      /** Line where the error happened. */
      int nLine;
      /** Character where the error happened. */
      int nChar;
      /** Falcon Error code. */
      int nCode;
      /** Context information -- line where the failing context begun. */
      int nOpenContext;
      /** Extra information (error description is inferred through nCode). */
      String sExtra;
      /** Uri of the source stream. */
      String sUri;

      Error* objError;

      ErrorDef( int code, const String& uri, int l, int c, int ctx, const String& extra ):
         nLine(l),
         nChar(c),
         nCode(code),
         nOpenContext(ctx),
         sExtra( extra ),
         sUri( uri ),
         objError( 0 )
      {}

      ErrorDef( int code, const String& uri, int l, int c, int ctx=0  ):
         nLine(l),
         nChar(c),
         nCode(code),
         nOpenContext( ctx ),
         sUri( uri ),
         objError( 0 )
      {}

      ErrorDef( Error* error )
      {
         objError = error;
      }

   };

   /** Returns true if the parser has exausted all the tokens.
      \return true If all the tokens are consumed.

    When a parse is not complete, then some tokens are left in the stack.

    It might be an error or it may just be a sign that some more input is
    needed to complete a parse, depending on the context.
    */
   bool isComplete() const;

   /** Checks if some errors are active.
      \return True if there is some error still accounted in the compiler.
      \see clearErrors
    */
   bool hasErrors() const;

   /** Create an Error instance that can be propagated in the system.
    \return A new Error instance or 0 if no errors are in the parser.

    \note After this call the errors are still accounted; call clearErrors()
    to clear them.
    */
   GenericError* makeError() const;

   /** Clear errors previously accounted in the engine.

    This method is automatically called by parse() and step().
    */
   void clearErrors();

   /** Clear token temporarily left in the parser stack on incomplete parse.
    */
   void clearTokens();

   /** Clear all the frames up to current decision (and all the tokens read up to date).
    */
   void clearFrames();
   
   /** Perform a single-step compilation.
      \return true on success, false if there is an error.

    Ask to generate a compilation keeping the current context and the current
    tokens previously parsed by the lexer.

    Prior calling step() the first time, pushLexer() and pushState() should have
    been called once to initialize the parser.
    */
   bool step();

   /** Callback functor receiving errors.
    */
   typedef Enumerator<ErrorDef> ErrorEnumerator;

   /** Enumerate received errors.
    In case parse returned false, calling this method will provide detailed
    error description for all the errors that have been found.
      \see Enumerator
    */
   void enumerateErrors( ErrorEnumerator& e ) const;

   /** Sets a context data.
    \param ctx User specific context data.

    This context data can be used by user code to store information together
    with the parser as the parsing proceeds.

    This is a good place where to store data that must be accessed by the rules.
    */
   void setContext( void* ctx );

   /** Return the parser context.

    The context is user data that can be used by more specific parser code.
    */
   void* context() const { return m_ctx; }

   /** Pushes a new lexer.
    \param lexer The new lexer to be used from now on.

    Userful to support hard-inclusion, the new lexer punches in at current point.
    When the input terminates, the lexer is removed and the previous one continues
    to provide tokens.

    @Note: The lexer is owned by the parser; this means that it will be destroyed
    when going out of scope.
    */
   void pushLexer( Lexer* lexer );

   /** Removed current lexer.

    If the current lexer is also the topmost one, parsing is interrupted.
    The removed lexer is destroyed.
    */
   void popLexer();

   /** Adds an error for the parser.
      \param code The error code (as a falcon error code number).
      \param uri The URI of the stream or file where the error was detected.
      \param l The line where the error happened (1 based; 0 if the error was not detected on a specific line).
      \param c The character where the error happened (1 based; 0 if the error was not detected on a specific character).
      \param ctx The line where an error due to a remote line was first originated.
      \param extra Extra error description (beside the textual explanation of code).

      Once called, this method marks the current parsing as faulty, and parse() will return false.
    */
   virtual void addError( int code, const String& uri, int l, int c, int ctx, const String& extra );

   /** Adds a preconfigured error to the parser.
      \param error The pre-configured error.

      Warning: this methdo does NOT incref the received error. Do not  decref
      on the other side.
    */
   virtual void addError( Error* error );

   /** Adds an error for the parser.

      \param code The error code (as a falcon error code number).
      \param uri The URI of the stream or file where the error was detected.
      \param l The line where the error happened (1 based; 0 if the error was not detected on a specific line).
      \param c The character where the error happened (1 based; 0 if the error was not detected on a specific character).
      \param ctx The line where an error due to a remote line was first originated.

    Once called, this method marks the current parsing as faulty, and parse() will return false.
    */
   virtual void addError( int code, const String& uri, int l, int c=0, int ctx=0  );

   /** Returns true if the parser should terminate asap.
      \return true on termination requested.
    */
   bool isDone() const { return m_bIsDone; }

   /** Ask the parser to terminate.
      A rule apply may invoke this routine to terminate the parsing process.
      The parser main loop returns as soon as it gets back in control.

    \note This call has not effect if called from code not invoked by rule
          application.
    */
   void terminate() { m_bIsDone = true; }


   /** Gets the number of tokens currently laying in the stack.
    \return Number of total tokens read in the stack.
    \note This is not an inline. Call sparcely.
    */
   int32 tokenCount();

   /** Gets the number of tokens in the current stack context
    \return Number of in the stack available to the current rule.
    \note This is not an inline. Call sparcely.
    */
   int32 availTokens();

   /** Gets the next token that is available for this rule.
    \return Next token instance stored in the stack.

    This method gets the token in the stack seen by the current rule
    one at a time.

    Token ownership stays on the Parser.

    Calls to simplify() or resetNextToken() will put the token index
    back to the top.

    \note It is granted that the  token index is reset before any rule apply
    function is called.

    \note If called more than availTokens() times, it will return 0.
    \see simplify
    */
   TokenInstance* getNextToken();

   /** Gets the last available token in the current rule.
    \return The last available token, if any, or 0 if the stack is empty.

    This method gets the last token that is currently available in the
    rule parsing.

    (Usually, as error are detected when a non-unified token is found,
    this means that the last available token is very probably the one
    having caused an error).
    */
   TokenInstance* getLastToken();

   /** Keeps the token that have been read and discard the rest.

    This method discards all the tokens that are currently available and
    would be taken with subsequent calls to getNextToken();
    */
   void trimFromCurrentToken();

   /** Keeps the first base tokens, and discard the next count tokens.

    */
   void trimFromBase(unsigned int base, unsigned int count);

   /** Trims the topmost (last) n parsed stack elements away.
    \param count Count of element to remove from the top (latest) parsed stack elements.
    */
   void trim( unsigned int count );

   /** Reposition the token index for getNextToken at top.
    \see getNextToken
    */
   void resetNextToken();

   /** Simplify 1 or more tokens in the stack with a new token instance.
    \param tcount Number of token to be simplified.
    \param newtoken substitute for the previously existing tokens, or 0 for nothing.
    \throw CodeError if tcount is out of range.

    This method destroys a certain number of tokens in the stack (possibly, one or more,
    but it's possible to use 0 as well to insert new tokens).

    If a newtoken parameter is given, the removed tokens are substituted by the
    new token given there. Otherwise, the tokens are simply removed.

    The semantic

    @code
    parser->simplify( tokenCount() );
    @endcode

    can be used to remove all the contents of the token stack, for example, after a toplevel
    rule match which is supposed to leave the parser in an initial state.

    The following semantic is also useful:
    @code
    parser->simplify( availTokens(), myToken );
    @endcode

    to clear everything that's left in the stack and change it with something
    a rule wants to store.

    \note After this call, any token read through getNextToken() should be
    considered invalid (as some or all of them may be destroyed).

    */
   void simplify( int32 tcount, TokenInstance* newtoken = 0 );

   /** Returns a string representation of the tokens in the stak.
      \return A list of toekns in the stack as a String representation.

    This method can be used for debugging purpose to inspect the current status of the
    parser stack.

    The returned stack is in format:
    @code
    TokenName, TokenName, >> TokenName, ..., TokenName -- next: TokenName
    @endcode

    where ">>" indicates the point in the stack where the rule currently being
    parsed is starting to match the tokens, and next: is the read-ahead token.
    */
   String dumpStack()  const;

   /** Generate a syntax error at current stack position.

    This generates a generic syntax error which is marked at the position of
    the first token in the stack, and then clear the stack contents.

    */
   void syntaxError();

   /** Add a state to this parser. */
   inline Parser& operator <<( State& s ) { addState(s); return *this; }

   /** URI of the currently lexed source.
    \return A string representing the current source currently under parsing.
    */
   const String& currentSource() const;
   
   /** Returns the current line in the current lexer. */
   int currentLine() const;

   /** Returns the interactive mode status.
    \return true if the parser is in interactive mode.
    */
   bool interactive() const { return m_bInteractive; }

   /** Sets the interactive mode.
    \param mode If true, the parser is in interactive mode.

    In interactive mode, the parser won't complain for partial unresolved
    code at the end of input. Instead, it will return normally, and the caller
    will need to feed in more data in the stream that's controlled by the lexer,
    and then call step() again to continue from the previous position.

    Also, in interactive mode, lexers are not automatically popped when they reach
    the end of their input, so it is possible to feed new data back in them
    without having to recreate them.
    */
   void interactive( bool mode ) { m_bInteractive = mode; }

   /** Returns the last line at which an error was found.
    \return a line number or 0 if no error is found yet.
    
    This method is useful to avoid raising extra errors in error handlers.
    */
   int32 lastErrorLine() const;

   //=======================================
   // To be documented
   typedef void* Path;

   Path createPath() const;
   Path copyPath( Path p ) const;
   void discardPath( Path p ) const;
   void confirmPath( Path p ) const;
   void addRuleToPath( Path p, Rule* r ) const;

     // -----

   void addRuleToPath( const Rule* r ) const;
   void addParseFrame( const NonTerminal* token, int pos = -1);

   size_t rulesDepth() const;
   size_t frameDepth() const;
   void unroll( size_t fd, size_t rd );

   bool findPaths( bool bIncremental );
   bool applyPaths();
   void parseError();
   void setFramePriority( const Token& token );


   TokenInstance* getCurrentToken( int& pos ) const;

   /** Clears the current parser status.

    This completely resets the parser, clearing the stack of received tokens
    and allowing subclasses to clear their own state.

    The State vector is cleared as well, so it's necessary to add a
    state before proceeding in parsing.

    \note Lexers are left untouched in their current state.
    */
   virtual void reset();
   
   Lexer* currentLexer() const;

   //=================================================================
   // Common terminals
   Terminal T_EOF;
   Terminal T_EOL;
   Terminal T_Float;
   Terminal T_Int;
   Terminal T_Name;
   Terminal T_String;
   Terminal T_DummyTerminal;

   void consumeUpTo( const Token& token ) { m_consumeToken = &token; }
   
   /**
    * Returns the last line parsed by the previous lexer.
    *
    * After a lexer terminates (and is removed), the last line
    * is recorded.
    */
   int lastLine() const {return m_lastLine;}

   const String& lastSource() const { return m_lastSource; }

protected:
   void* m_ctx;
   bool m_bIsDone;
   bool m_bInteractive;

   void parserLoop();
   void followPaths();

   // Checks performed after a new token arrived.
   void onNewToken();

   void explorePaths();

   // simplifies the topmost rule.
   void applyCurrentRule();
   
private:
   friend class Rule;
   class Private;

   // Data that requires local instantation
   Parser::Private* _p;

   const Token* m_consumeToken;
   int m_lastLine;
   String m_lastSource;
};

}
}

#endif	/* _FALCON_PARSER_PARSER_H_ */

/* end of parser/parser.h */
