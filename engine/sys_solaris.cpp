/*
   FALCON - The Falcon Programming Language.
   FILE: sys_solaris.cpp

   System specific (UNIX - solaris) support for VM.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: mar nov 9 2004

   -------------------------------------------------------------------
   (C) Copyright 2004: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

/** \file
   Short description
*/

#define _REENTRANT
#include <sys/time.h>

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <falcon/setup.h>
#include <falcon/types.h>
#include <falcon/memory.h>
#include <falcon/sys.h>
#include <falcon/string.h>
#include <falcon/transcoding.h>
#include <falcon/autocstring.h>

namespace Falcon {
namespace Sys {


numeric _seconds()
{
   struct timeval time;
   gettimeofday( &time, 0 );
   return time.tv_sec + (time.tv_usec / 1000000.0 );
}


numeric _localSeconds()
{
   struct timeval current;
   struct tm date_local, date_gm;
   time_t t;

   gettimeofday( &current, 0 );
   time( &t );
   localtime_r( &t, &date_local );
   gmtime_r( &t, &date_gm );
   time_t leap = mktime( &date_local) - mktime( &date_gm );

   return leap + current.tv_sec + (current.tv_usec / 1000000.0 );
}

uint32 _milliseconds()
{
#if POSIX_TIMERS > 0
   struct timespec time;
   clock_gettime( CLOCK_REALTIME, &time );
   return time.tv_sec * 1000 + time.tv_nsec / 1000000;
#else
   struct timeval time;
   gettimeofday( &time, 0 );
   return time.tv_sec * 1000 + time.tv_usec / 1000;
#endif
}

void _tempName( String &res )
{
   static bool first = true;
   const char *temp_dir;
   char *fname;
   struct stat st;

   if( first ) {
      first = false;
      time_t t;
      srand( (unsigned int ) time( &t ) );
   }

   temp_dir = getenv( "TMP" );

   if ( temp_dir == 0 )
      temp_dir = getenv( "TMPDIR" );

   if ( temp_dir == 0 ) {
      temp_dir = DEFAULT_TEMP_DIR;
   }

   if ( stat( temp_dir, &st ) == -1 || ! S_ISDIR( st.st_mode ) ) {
      temp_dir = ".";
   }

   res = temp_dir;
   res.append( "/falcon_tmp_" );
   res.writeNumber( (int64) getpid() );
   res.append("_");
   res.writeNumber( (int64) rand() );
   res.bufferize();
   // force buffering
}

bool _describeError( int64 eid, String &target )
{
   const char *error = strerror( eid );
   if( error != 0 ) {
      target.bufferize( error );
      return true;
   }

   return false;
}

int64 _lastError()
{
   return (int64) errno;
}

bool _getEnv( const String &var, String &result )
{
   static char convertBuf[512]; // system var names larger than 512 are crazy.
   // in unix system, we have at worst UTF-8 var names.
   if ( var.toCString( convertBuf, 512 ) >= 0 )
   {
      char *value = getenv( convertBuf );
      if ( value != 0 )
      {
         TranscodeFromString( value, "utf-8", result );
         return true;
      }
   }

   return false;
}

int setenv (const char * name, const char * value, int overwrite) {
  size_t namelen = strlen(name);
  size_t valuelen = (value==NULL ? 0 : strlen(value));
  char* buffer = (char*)malloc(namelen+1+valuelen+1);
  if (!buffer)
    return -1; /* no need to set errno = ENOMEM */
  memcpy(buffer,name,namelen);
  if (value != NULL) {
    buffer[namelen] = '=';
    memcpy(buffer+namelen+1,value,valuelen);
    buffer[namelen+1+valuelen] = 0;
  } else
    buffer[namelen] = 0;
  return putenv(buffer);
}

bool _setEnv( const String &var, const String &value )
{
   // in unix system, we have at worst UTF-8 var names.
   uint32 varLen = var.length() * 4 + 2;
   uint32 valueLen = value.length() * 4 + 2;
   char *varBuf = (char *) memAlloc( varLen );
   char *valueBuf = (char *) memAlloc( valueLen );

   var.toCString( varBuf, varLen );
   value.toCString( valueBuf, valueLen );

   bool result = setenv( varBuf, valueBuf, 1 ) == 0;
   memFree( varBuf );
   memFree( valueBuf );
   return result;

}

bool _unsetEnv( const String &var )
{
   // in unix system, we have at worst UTF-8 var names.
   String sVarValue = var + "=";
   AutoCString buffer( sVarValue );
   bool result = putenv( (char*) buffer.c_str()) == 0;
   return result;
}

}
}


/* end of sys_solaris.cpp */
