/*
   FALCON - The Falcon Programming Language.
   FILE: parser_switch.h

   Parser for Falcon source files -- switch and select
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Wed, 02 May 2012 21:03:20 +0200

   -------------------------------------------------------------------
   (C) Copyright 2012: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#ifndef _FALCON_SP_PARSER_SWITCH_H_
#define _FALCON_SP_PARSER_SWITCH_H_

#include <falcon/setup.h>

namespace Falcon {

namespace Parsing
{
class Rule;
class Parser;
}

using namespace Parsing;

bool switch_errhand(const NonTerminal&, Parser& p);

void apply_switch( const Rule&, Parser& p );
void apply_select( const Rule&, Parser& p );
void apply_case( const Rule&, Parser& p );
void apply_case_short( const Rule&, Parser& p );
void apply_default( const Rule&, Parser& p );
void apply_default_short( const Rule&, Parser& p );

void apply_CaseListRange_int( const Rule&, Parser& p );
void apply_CaseListRange_string( const Rule&, Parser& p );
void apply_CaseListToken_range( const Rule&, Parser& p );
void apply_CaseListToken_nil( const Rule&, Parser& p );
void apply_CaseListToken_true( const Rule&, Parser& p );
void apply_CaseListToken_false( const Rule&, Parser& p );
void apply_CaseListToken_int( const Rule&, Parser& p );
void apply_CaseListToken_string( const Rule&, Parser& p );
void apply_CaseListToken_sym( const Rule&, Parser& p );

void apply_CaseList_next( const Rule&, Parser& p );
void apply_CaseList_first( const Rule&, Parser& p );
}

#endif

/* end of parser_switch.h */
