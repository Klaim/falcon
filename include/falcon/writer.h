/*
   FALCON - The Falcon Programming Language.
   FILE: writer.h

   Base abstract class for file writers.
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Sun, 20 Mar 2011 20:46:23 +0100

   -------------------------------------------------------------------
   (C) Copyright 2011: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/


#ifndef _FALCON_WRITER_H
#define	_FALCON_WRITER_H

#include <falcon/setup.h>
#include <falcon/types.h>
#include <falcon/stream.h>
#include <falcon/mt.h>
#include <falcon/refcounter.h>

namespace Falcon {

/** Base abstract class for stream writers.

 A Falcon Stream is a very basic, raw representation of an I/O resource. It's
 methods maps directly to the lowest possible level of system direct resource
 handling functions.

 This makes raw access to streams extremely efficient, but more articulated access,
 as the steps require to serialize or de-serialize a resource, might be extremely
 inefficient.

 \see Reader
 */
class FALCON_DYN_CLASS Writer
{
public:

   /** Delegates another Writer.
    \param target Another Writer that will be in charge of handling this stream.

    The stream control, and eventually ownership, is passed onto another writer.
    The state of the underlying stream, including its current buffer, is maintained
    coherent and passed onto the target.

    \note Using this writer after the this call has an undefined behavior.
    */
   void delegate( Writer& target );

   /** Changes the buffer size.

    By default, the I/O buffering is set to exactly the size of a "memory page"
    (4096 bytes on most systems). It is suggested to set this size to a multiple
    of the memory page size.

    */
   virtual void setBufferSize( length_t bs );
   
   length_t bufferSize() const { return m_bufSize; }

   /** Write all the pending data that's left.
    \throw IOError on error in flushing the stream.
    \throw InterruptedError if the I/O operation has been interrupted.
   */
   virtual bool flush();

   /** Writes to the internal buffer and eventually store what's written to a file.

   */
   virtual bool writeRaw( const byte* data, size_t dataSize );

   /** Changes the underlying stream.
    \param s The new stream that this reader should read from.
    \param bOwn if true, the stream is owned by this Reader (and destroyed at Reader destruction).
    \param bDiscard Discard the buffered data still unread coming from the old stream.
    \throw IOError if the flush write fails.
    
    Pending write operations on the previous stream are flushed. This behavior can be overridden by setting
    bDiscard to true; in this case any data still unwritten is silently discarded.

    If it was owned, the previous stream is destroyed.
    */
   virtual void changeStream( Stream* s, bool bDiscard = false );

   Stream* underlying() const { return m_stream; }

   /** Checks if this entity is in GC. */
   bool isInGC() const { return m_gcMark != 0; }

   /** Mark this entity for GC. */
   void gcMark( uint32 mark ) {
      m_gcMark = mark;
      if( m_stream != 0 ) {
         m_stream->gcMark(mark);
      }
   }

   /** Gets the GC entity. */
   uint32 gcMark() const { return m_gcMark; }

protected:
   /** Create for normal operations. */
   Writer( Stream* stream );

   /** Gets the other stream and increfs it. */
   Writer( const Writer& other );

   /** Create for immediate delegation. */
   Writer();

   /** Grant that there is enough space in the currentBuffer(). */
   void ensure( size_t size );

   byte* currentBuffer() const { return m_buffer + m_bufPos; }

   byte* m_buffer;
   length_t m_bufPos;
   length_t m_bufSize;

   uint32 m_gcMark;

protected:
   Stream* m_stream;
   virtual ~Writer();

private:
   Mutex m_mtx;
   FALCON_REFERENCECOUNT_DECLARE_INCDEC(Writer);
};

}

#endif	/* _FALCON_WRITER_H */

/* end of writer.h */