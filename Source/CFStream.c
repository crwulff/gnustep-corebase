/* CFStream.c

   Copyright (C) 2012 Free Software Foundation, Inc.

   Written by: Stefan Bidigaray
   Date: August, 2012

   This file is part of GNUstep CoreBase Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.         See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; see the file COPYING.LIB.
   If not, see <http://www.gnu.org/licenses/> or write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "CoreFoundation/CFRuntime.h"
#include "CoreFoundation/CFStream.h"
#include "GSPrivate.h"

#include <string.h>

CONST_STRING_DECL(kCFStreamPropertyDataWritten, "kCFStreamPropertyDataWritten");
CONST_STRING_DECL(kCFStreamPropertySocketNativeHandle,
  "kCFStreamPropertySocketNativeHandle");
CONST_STRING_DECL(kCFStreamPropertySocketRemoteHostName,
  "kCFStreamPropertySocketRemoteHostName");
CONST_STRING_DECL(kCFStreamPropertySocketRemotePortNumber,
  "kCFStreamPropertySocketRemotePortNumber");
CONST_STRING_DECL(kCFStreamPropertyAppendToFile,
  "kCFStreamPropertyAppendToFile");
CONST_STRING_DECL(kCFStreamPropertyFileCurrentOffset,
  "kCFStreamPropertyFileCurrentOffset");

/* Stream _flags */
enum
{
  MIN_STATUS_CODE_BIT = 0,
  MAX_STATUS_CODE_BIT = 4,
  CONSTANT_CALLBACKS  = 5,
  CALLING_CLIENT      = 6,
  HAVE_CLOSED         = 7,
  SHARED_SOURCE       = 8
};

#define STATUS_CODE_MASK (((2 << MAX_STATUS_CODE_BIT)-1) << MIN_STATUS_CODE_BIT)

struct __CFStream;

struct __CFStreamClient
{
  CFStreamClientContext context;
  void (*callback)(struct __CFStream *, CFStreamEventType, void *);
  CFOptionFlags      when;
  CFRunLoopSourceRef rlSource;
  CFMutableArrayRef  runLoopsAndModes;
  CFOptionFlags      whatToSignal;
};
typedef struct __CFStreamClient __CFStreamClient;

struct __CFStreamCallBacks
{
  CFIndex version;

  void *        (*create)         (struct __CFStream *stream, void *info);
  void          (*finalize)       (struct __CFStream *stream, void *info);
  CFStringRef   (*copyDescription)(struct __CFStream *stream, void *info);
  Boolean       (*open)           (struct __CFStream *stream, CFStreamError *error, Boolean *openComplete, void *info);
  Boolean       (*openCompleted)  (struct __CFStream *stream, CFStreamError *error, void *info);
  CFIndex       (*read)           (CFReadStreamRef stream, UInt8 *buffer, CFIndex bufferLength, CFStreamError *error, Boolean *atEOF, void *info);
  const UInt8 * (*getBuffer)      (CFReadStreamRef sream, CFIndex maxBytesToRead, CFIndex *numBytesRead, CFStreamError *error, Boolean *atEOF, void *info);
  Boolean       (*canRead)        (CFReadStreamRef, void *info);
  CFIndex       (*write)          (CFWriteStreamRef, const UInt8 *buffer, CFIndex bufferLength, CFStreamError *error, void *info);
  Boolean       (*canWrite)       (CFWriteStreamRef, void *info);
  void          (*close)          (struct __CFStream *stream, void *info);
  CFTypeRef     (*copyProperty)   (struct __CFStream *stream, CFStringRef propertyName, void *info);
  Boolean       (*setProperty)    (struct __CFStream *stream, CFStringRef propertyName, CFTypeRef propertyValue, void *info);
  void          (*requestEvents)  (struct __CFStream *stream, CFOptionFlags events, void *info);
  void          (*schedule)       (struct __CFStream *stream, CFRunLoopRef runLoop, CFStringRef runLoopMode, void *info);
  void          (*unschedule)     (struct __CFStream *stream, CFRunLoopRef runLoop, CFStringRef runLoopMode, void *info);
};
typedef struct __CFStreamCallBacks __CFStreamCallBacks;

struct __CFStream
{
  CFRuntimeBase              _parent;
  CFOptionFlags              _flags;
  CFStreamError              _error;
  __CFStreamClient          *_client;
  void                      *_info;
  const __CFStreamCallBacks *_callbacks;
  void                      *_reserved1;
};
typedef struct __CFStream __CFStream;

static CFTypeID _kCFReadStreamTypeID;
static CFTypeID _kCFWriteStreamTypeID;

static inline void
_CFStreamScheduleEvent(__CFStream *stream, CFStreamEventType event)
{
  // TODO
}

static void
_CFStreamSignalEvent(__CFStream *stream, CFStreamEventType event, CFStreamError *error, Boolean synchronousAllowed)
{
  // TODO
}

/* Read Data Stream */

struct _CFReadDataStreamContext
{
  CFDataRef    _data;
  const UInt8 *_location;
  Boolean      _scheduled;
};
typedef struct _CFReadDataStreamContext _CFReadDataStreamContext;

void
CFReadStreamSignalEvent(CFReadStreamRef stream, CFStreamEventType event, CFStreamError *error)
{
  _CFStreamSignalEvent((__CFStream *)stream, event, error, TRUE);
}

static void *
readDataCreate(__CFStream *stream, void *info)
{
  _CFReadDataStreamContext *ctxt = (_CFReadDataStreamContext *)info;
  _CFReadDataStreamContext *newCtxt = (_CFReadDataStreamContext *)CFAllocatorAllocate(CFGetAllocator(stream), sizeof(_CFReadDataStreamContext), 0);
  if (newCtxt)
    {
      newCtxt->_data      = CFRetain(ctxt->_data);
      newCtxt->_location  = CFDataGetBytePtr(newCtxt->_data);
      newCtxt->_scheduled = FALSE;
    }
  return (void *)newCtxt;
}

static void
readDataFinalize(__CFStream *stream, void *info)
{
  _CFReadDataStreamContext *ctxt = (_CFReadDataStreamContext *)info;
  CFRelease(ctxt->_data);
  CFAllocatorDeallocate(CFGetAllocator(stream), ctxt);
}

static CFStringRef
readDataCopyDescription(__CFStream *stream, void *info)
{
  return CFCopyDescription(((_CFReadDataStreamContext *)info)->_data);
}

static Boolean
readDataOpen(__CFStream *stream, CFStreamError *errorCode, Boolean *openComplete, void *info)
{
  _CFReadDataStreamContext *ctxt = (_CFReadDataStreamContext *)info;
  if (ctxt->_scheduled)
    {
      CFReadStreamSignalEvent((CFReadStreamRef)stream,
        (CFDataGetLength(ctxt->_data) > 0) ? kCFStreamEventHasBytesAvailable : kCFStreamEventEndEncountered, NULL);
    }

  errorCode->error = 0;
  *openComplete = TRUE;
  return TRUE;
}

static CFIndex
dataRead(CFReadStreamRef stream, UInt8 *buffer, CFIndex bufferLength, CFStreamError *error, Boolean *atEOF, void *info)
{
  _CFReadDataStreamContext *ctxt = (_CFReadDataStreamContext *)info;
  const UInt8 *bytePtr = CFDataGetBytePtr(ctxt->_data);
  CFIndex length = CFDataGetLength(ctxt->_data);
  CFIndex bytesToCopy = bytePtr + length - ctxt->_location;
  if (bytesToCopy > bufferLength)
    {
      bytesToCopy = bufferLength;
    }
  if (bytesToCopy < 0)
    {
      bytesToCopy = 0;
    }
  if (bytesToCopy != 0)
    {
      memmove(buffer, ctxt->_location, bytesToCopy);
      ctxt->_location += bytesToCopy;
    }

  error->error = 0;
  *atEOF = (ctxt->_location < bytePtr + length) ? FALSE : TRUE;
  if (ctxt->_scheduled && !*atEOF)
    {
      CFReadStreamSignalEvent(stream, kCFStreamEventHasBytesAvailable, NULL);
    }
  return bytesToCopy;
}

static const UInt8 *
dataGetBuffer(CFReadStreamRef stream, CFIndex maxBytesToRead, CFIndex *numBytesRead, CFStreamError *error, Boolean *atEOF, void *info)
{
  _CFReadDataStreamContext *ctxt = (_CFReadDataStreamContext *)info;
  const UInt8 *bytes = CFDataGetBytePtr(ctxt->_data);
  if (ctxt->_location - bytes > maxBytesToRead)
    {
      *numBytesRead = maxBytesToRead;
      *atEOF = FALSE;
    }
  else
    {
      *numBytesRead = ctxt->_location - bytes;
      *atEOF = TRUE;
    }

  error->error = 0;
  bytes = ctxt->_location;
  ctxt->_location += *numBytesRead;
  if (ctxt->_scheduled && !*atEOF)
    {
      CFReadStreamSignalEvent(stream, kCFStreamEventHasBytesAvailable, NULL);
    }
  return bytes;
}

static Boolean
dataCanRead(CFReadStreamRef stream, void *info)
{
  _CFReadDataStreamContext *ctxt = (_CFReadDataStreamContext *)info;
  return (CFDataGetBytePtr(ctxt->_data) + CFDataGetLength(ctxt->_data) > ctxt->_location) ? TRUE : FALSE;
}

static void
readDataSchedule(__CFStream *stream, CFRunLoopRef rl, CFStringRef rlMode, void *info)
{
  _CFReadDataStreamContext *ctxt = (_CFReadDataStreamContext *)info;
  if (ctxt->_scheduled == FALSE)
    {
      ctxt->_scheduled = TRUE;
      if (CFReadStreamGetStatus((CFReadStreamRef)stream) != kCFStreamStatusOpen)
        {
          return;
	}
      if (CFDataGetBytePtr(ctxt->_data) + CFDataGetLength(ctxt->_data) > ctxt->_location)
        {
          CFReadStreamSignalEvent((CFReadStreamRef)stream, kCFStreamEventHasBytesAvailable, NULL);
        }
      else
        {
          CFReadStreamSignalEvent((CFReadStreamRef)stream, kCFStreamEventEndEncountered, NULL);
        }
    }
}

static const __CFStreamCallBacks readDataCallbacks =
{
  1,
  readDataCreate,
  readDataFinalize,
  readDataCopyDescription,
  readDataOpen,
  NULL,
  dataRead,
  dataGetBuffer,
  dataCanRead,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  readDataSchedule,
  NULL
};

#if 0
/* Write Data Stream */

static const __CFStreamCallBacks writeDataCallbacks =
{
  1,
  writeDataCreate,
  writeDataFinalize,
  writeDataCopyDescription,
  writeDataOpen,
  NULL,
  NULL,
  NULL,
  NULL,
  dataWrite,
  dataCanWrite,
  NULL,
  dataCopyProperty,
  NULL,
  NULL,
  writeDataSchedule,
  NULL
};
#endif

static inline CFStreamStatus
__CFStreamGetStatus(__CFStream *stream)
{
  return (CFStreamStatus)((stream->_flags & STATUS_CODE_MASK) >> MIN_STATUS_CODE_BIT);
}

static inline void
_CFStreamSetStatusCode(__CFStream *stream, CFStreamStatus newStatus)
{
  switch (__CFStreamGetStatus(stream))
    {
      case kCFStreamStatusError:
        /* If the stream is in error, stay that way */
	break;

      case kCFStreamStatusClosed:
        /* If the stream is closed, only allow setting error */
        if (newStatus != kCFStreamStatusError) { break; }

      default:
        stream->_flags &= ~STATUS_CODE_MASK;
	stream->_flags |= (newStatus << MIN_STATUS_CODE_BIT) & STATUS_CODE_MASK;
	break;
    }
}

static CFStreamStatus
_CFStreamGetStatus(__CFStream *stream) {
  CFStreamStatus status = __CFStreamGetStatus(stream);
  stream->_flags |= (1 << CALLING_CLIENT);

  if (status == kCFStreamStatusOpening)
    {
      if (stream->_callbacks->openCompleted && stream->_callbacks->openCompleted(stream, &stream->_error, stream->_info))
        {
          status = (stream->_error.error == 0) ? kCFStreamStatusOpen : kCFStreamStatusError;
          _CFStreamSetStatusCode(stream, status);
          _CFStreamScheduleEvent(stream, (status == kCFStreamStatusOpen) ? kCFStreamEventOpenCompleted : kCFStreamEventErrorOccurred);
        }
    }
  stream->_flags &= ~(1 << CALLING_CLIENT);
  return status;
}

#define CFSTREAM_SIZE \
  sizeof(__CFStream) - sizeof(struct __CFRuntimeBase)

static __CFStream *
_CFStreamCreate(CFAllocatorRef allocator, Boolean readStream)
{
  __CFStream *new = (__CFStream*)_CFRuntimeCreateInstance(allocator,
    (readStream ? _kCFReadStreamTypeID : _kCFWriteStreamTypeID), CFSTREAM_SIZE, NULL);

  if (new)
    {
      new->_flags = 0;
      _CFStreamSetStatusCode(new, kCFStreamStatusNotOpen);
      new->_error.domain = 0;
      new->_error.error = 0;
      new->_client = NULL;
      new->_info = NULL;
      new->_callbacks = NULL;
    }
  return new;
}

static void
_CFStreamDeallocate(CFTypeRef cf)
{
  // TODO
}

static __CFStream *
_CFStreamCreateWithConstantCallbacks(CFAllocatorRef alloc, void *info, const __CFStreamCallBacks *callbacks, Boolean readStream)
{
  __CFStream *new = NULL;
  if (callbacks->version == 1)
    {
      new = _CFStreamCreate(alloc, readStream);
      if (new)
        {
          new->_flags |= 1 << CONSTANT_CALLBACKS;
          new->_callbacks = callbacks;
          new->_info = (callbacks->create ? callbacks->create(new, info) : info);
        }
    }

  return new;
}

static CFStringRef
_CFStreamCopyDescription(CFTypeRef cf)
{
  __CFStream *stream = (__CFStream *)cf;
  CFStringRef infoDesc;
  CFStringRef desc;

  if (stream->_callbacks->copyDescription)
    {
      /* Version 0 takes only the info, Version 1 takes the stream and the info */
      if (stream->_callbacks->version == 0)
        {
          infoDesc = ((CFStringRef (*)(void*))stream->_callbacks->copyDescription)(stream->_info);
	}
      else
	{
          infoDesc = stream->_callbacks->copyDescription(stream, stream->_info);
        }
    }
  else
    {
      infoDesc = CFStringCreateWithFormat(CFGetAllocator(stream), NULL, CFSTR("info = 0x%lx"), (uint32_t)(uintptr_t)stream->_info);
    }

  desc = CFStringCreateWithFormat(CFGetAllocator(stream), NULL, CFSTR("<CF%@Stream 0x%x>{%@}"),
		                  (CFGetTypeID(cf) == _kCFReadStreamTypeID) ? CFSTR("Read") : CFSTR("Write"),
				  (uint32_t)(uintptr_t)stream, infoDesc);

  CFRelease(infoDesc);

  return desc;
}

static void
waitForOpen(__CFStream *stream)
{
 // TODO
}

static Boolean
_CFStreamOpen(__CFStream *stream)
{
  Boolean result = _CFStreamGetStatus(stream) == kCFStreamStatusNotOpen;
  Boolean done = true;
  if (result)
    {
      stream->_flags |= (1 << CALLING_CLIENT);
      _CFStreamSetStatusCode(stream, kCFStreamStatusOpening);
      if (stream->_callbacks->open)
        {
          result = stream->_callbacks->open(stream, &stream->_error, &done, stream->_info);
        }

      if (done)
        {
          if (result)
	    {
              if (__CFStreamGetStatus(stream) == kCFStreamStatusOpening)
	        {
                  _CFStreamSetStatusCode(stream, kCFStreamStatusOpen);
                }
              _CFStreamScheduleEvent(stream, kCFStreamEventOpenCompleted);
            }
	  else
	    {
              _CFStreamSetStatusCode(stream, kCFStreamStatusError);
              _CFStreamScheduleEvent(stream, kCFStreamEventErrorOccurred);
	      stream->_flags |= (1 << HAVE_CLOSED);
            }
        }
      stream->_flags &= ~(1 << CALLING_CLIENT);
    }

  return result;
}

static void
_CFStreamClose(__CFStream *stream)
{
  // TODO
}

static const CFRuntimeClass CFReadStreamClass =
{
  0,
  "CFReadStream",
  NULL,
  NULL,
  _CFStreamDeallocate,
  NULL,
  NULL,
  NULL,
  _CFStreamCopyDescription
};

static const CFRuntimeClass CFWriteStreamClass =
{
  0,
  "CFReadStream",
  NULL,
  NULL,
  _CFStreamDeallocate,
  NULL,
  NULL,
  NULL,
  _CFStreamCopyDescription
};

void CFStreamInitialize (void)
{
  _kCFReadStreamTypeID = _CFRuntimeRegisterClass (&CFReadStreamClass);
  _kCFWriteStreamTypeID = _CFRuntimeRegisterClass (&CFWriteStreamClass);
}

CFTypeID
CFWriteStreamGetTypeID (void)
{
  return _kCFWriteStreamTypeID;
}

CFTypeID
CFReadStreamGetTypeID (void)
{
  return _kCFReadStreamTypeID;
}

void
CFStreamCreateBoundPair (CFAllocatorRef alloc, CFReadStreamRef *readStream,
                         CFWriteStreamRef *writeStream,
                         CFIndex transferBufferSize)
{
  // FIXME
}

/*
void
CFStreamCreatePairWithPeerSocketSignature (CFAllocatorRef alloc,
                                           const CFSocketSignature *signature,
                                           CFReadStreamRef *readStream,
                                           CFWriteStreamRef *writeStream)
{
  // FIXME
}

void
CFStreamCreatePairWithSocket (CFAllocatorRef alloc, CFSocketNativeHandle sock,
                              CFReadStreamRef *readStream,
                              CFWriteStreamRef *writeStream)
{
  // FIXME
}
*/

void
CFStreamCreatePairWithSocketToHost (CFAllocatorRef alloc, CFStringRef host,
                                    UInt32 port, CFReadStreamRef *readStream,
                                    CFWriteStreamRef *writeStream)
{

}



Boolean
CFWriteStreamCanAcceptBytes (CFWriteStreamRef stream)
{
  return false;
}

void
CFWriteStreamClose (CFWriteStreamRef stream)
{
  CF_OBJC_FUNCDISPATCH0(_kCFWriteStreamTypeID, void, stream, "close");
  _CFStreamClose((__CFStream *)stream);
}

CFErrorRef
CFWriteStreamCopyError (CFWriteStreamRef stream)
{
  return NULL;
}

CFTypeRef
CFWriteStreamCopyProperty (CFWriteStreamRef stream, CFStringRef propertyName)
{
  return NULL;
}

CFWriteStreamRef
CFWriteStreamCreateWithAllocatedBuffers (CFAllocatorRef alloc,
                                         CFAllocatorRef bufferAllocator)
{
  return NULL;
}

CFWriteStreamRef
CFWriteStreamCreateWithBuffer (CFAllocatorRef alloc, UInt8 *buffer,
                               CFIndex bufferCapacity)
{
  return NULL;
}

CFWriteStreamRef
CFWriteStreamCreateWithFile (CFAllocatorRef alloc, CFURLRef fileURL)
{
  return NULL;
}

CFStreamError
CFWriteStreamGetError (CFWriteStreamRef stream)
{
  CF_OBJC_FUNCDISPATCH0(_kCFWriteStreamTypeID, CFStreamError, stream, "_cfStreamError");
  return ((__CFStream *)stream)->_error;
}

CFStreamStatus
CFWriteStreamGetStatus (CFWriteStreamRef stream)
{
  CF_OBJC_FUNCDISPATCH0(_kCFWriteStreamTypeID, CFStreamStatus, stream, "streamStatus");
  return _CFStreamGetStatus((__CFStream *)stream);
}

Boolean
CFWriteStreamOpen (CFWriteStreamRef stream)
{
  return false;
}

void
CFWriteStreamScheduleWithRunLoop (CFWriteStreamRef stream,
                                  CFRunLoopRef runLoop,
                                  CFStringRef runLoopMode)
{
  ;
}

Boolean
CFWriteStreamSetClient (CFWriteStreamRef stream, CFOptionFlags streamEvents,
                        CFWriteStreamClientCallBack clientCB,
                        CFStreamClientContext *clientContext)
{
  return false;
}

Boolean
CFWriteStreamSetProperty (CFWriteStreamRef stream, CFStringRef propertyName,
                          CFTypeRef propertyValue)
{
  return false;
}

void
CFWriteStreamUnscheduleFromRunLoop (CFWriteStreamRef stream,
                                    CFRunLoopRef runLoop,
                                    CFStringRef runLoopMode)
{

}

CFIndex
CFWriteStreamWrite (CFWriteStreamRef stream, const UInt8 *buffer,
                    CFIndex bufferLength)
{
  return 0;
}



void
CFReadStreamClose (CFReadStreamRef stream)
{
  CF_OBJC_FUNCDISPATCH0(_kCFReadStreamTypeID, void, stream, "close");
  _CFStreamClose((__CFStream *)stream);
}

CFErrorRef
CFReadStreamCopyError (CFReadStreamRef stream)
{
  return NULL;
}

CFTypeRef
CFReadStreamCopyProperty (CFReadStreamRef stream, CFStringRef propertyName)
{
  return NULL;
}

CFReadStreamRef
CFReadStreamCreateWithBytesNoCopy (CFAllocatorRef alloc, const UInt8 *bytes,
                                   CFIndex length,
                                   CFAllocatorRef bytesDeallocator)
{
  _CFReadDataStreamContext ctxt = {};
  ctxt._data = CFDataCreateWithBytesNoCopy(alloc, bytes, length, bytesDeallocator);
  __CFStream * stream = _CFStreamCreateWithConstantCallbacks(alloc, &ctxt, &readDataCallbacks, TRUE);
  CFRelease(ctxt._data);

  return (CFReadStreamRef)stream;
}

CFReadStreamRef
CFReadStreamCreateWithFile (CFAllocatorRef alloc, CFURLRef fileURL)
{
  return NULL;
}

const UInt8 *
CFReadStreamGetBuffer (CFReadStreamRef stream, CFIndex maxBytesToRead,
                       CFIndex *numBytesRead)
{
  return NULL;
}

CFStreamError
CFReadStreamGetError (CFReadStreamRef stream)
{
  CF_OBJC_FUNCDISPATCH0(_kCFReadStreamTypeID, CFStreamError, stream, "_cfStreamError");
  return ((__CFStream *)stream)->_error;
}

CFStreamStatus
CFReadStreamGetStatus (CFReadStreamRef stream)
{
  CF_OBJC_FUNCDISPATCH0(_kCFReadStreamTypeID, CFStreamStatus, stream, "streamStatus");
  return _CFStreamGetStatus((__CFStream *)stream);
}

Boolean
CFReadStreamHasBytesAvailable (CFReadStreamRef stream)
{
  CF_OBJC_FUNCDISPATCH0(_kCFReadStreamTypeID, Boolean, stream, "hasBytesAvailable");
  __CFStream *streamPriv = (__CFStream*)stream;
  CFStreamStatus status = _CFStreamGetStatus(streamPriv);
  Boolean result = false;
  if (status == kCFStreamStatusOpen || status == kCFStreamStatusReading)
    {
      if (streamPriv->_callbacks->canRead == NULL)
        {
          result = true;
        }
      else
        {
          streamPriv->_flags |= (1 << CALLING_CLIENT);
          result = streamPriv->_callbacks->canRead(stream, streamPriv->_info);
	  streamPriv->_flags &= ~(1 << CALLING_CLIENT);
        }
    }
  return result;
}

Boolean
CFReadStreamOpen (CFReadStreamRef stream)
{
  Boolean result = false;
  if (CF_IS_OBJC(_kCFReadStreamTypeID, stream))
    {
      CF_OBJC_VOIDCALLV(stream, "open");
      result = true;
    }
  else
    {
      result = _CFStreamOpen((__CFStream*)stream);
    }
  return result;
}

CFIndex
CFReadStreamRead (CFReadStreamRef stream, UInt8 *buffer, CFIndex bufferLength)
{
  CF_OBJC_FUNCDISPATCH2(_kCFReadStreamTypeID, CFIndex, stream, "read:maxLength:", buffer, bufferLength);
  __CFStream *streamPriv = (__CFStream*)stream;
  CFIndex bytesRead = -1;
  CFStreamStatus status = _CFStreamGetStatus(streamPriv);
  if (status == kCFStreamStatusOpening)
    {
      streamPriv->_flags |= (1 << CALLING_CLIENT);
      waitForOpen(streamPriv);
      streamPriv->_flags &= ~(1 << CALLING_CLIENT);
      status = _CFStreamGetStatus(streamPriv);
    }

  switch (status)
    {
      case kCFStreamStatusOpen:
      case kCFStreamStatusReading:
        {
          Boolean atEOF;
          streamPriv->_flags |= (1 << CALLING_CLIENT);
          if (streamPriv->_client)
	    {
              streamPriv->_client->whatToSignal &= ~kCFStreamEventHasBytesAvailable;
            }
          _CFStreamSetStatusCode(streamPriv, kCFStreamStatusReading);
          bytesRead = streamPriv->_callbacks->read(stream, buffer, bufferLength, &streamPriv->_error, &atEOF, streamPriv->_info);
          if (streamPriv->_error.error != 0)
	    {
              bytesRead = -1;
              _CFStreamSetStatusCode(streamPriv, kCFStreamStatusError);
              _CFStreamScheduleEvent(streamPriv, kCFStreamEventErrorOccurred);
            }
	  else if (atEOF)
	    {
              _CFStreamSetStatusCode(streamPriv, kCFStreamStatusAtEnd);
              _CFStreamScheduleEvent(streamPriv, kCFStreamEventEndEncountered);
            }
	  else
	    {
              _CFStreamSetStatusCode(streamPriv, kCFStreamStatusOpen);
            }
          streamPriv->_flags &= ~(1 << CALLING_CLIENT);
	}
	break;

      case kCFStreamStatusAtEnd:
        bytesRead = 0;
        break;

      default:
	break;
    }

  return bytesRead;
}

void
CFReadStreamScheduleWithRunLoop (CFReadStreamRef stream, CFRunLoopRef runLoop,
                                 CFStringRef runLoopMode)
{

}

Boolean
CFReadStreamSetClient (CFReadStreamRef stream, CFOptionFlags streamEvents,
                       CFReadStreamClientCallBack clientCB,
                       CFStreamClientContext *clientContext)
{
  return false;
}

Boolean
CFReadStreamSetProperty (CFReadStreamRef stream, CFStringRef propertyName,
                         CFTypeRef propertyValue)
{
  return false;
}

void
CFReadStreamUnscheduleFromRunLoop (CFReadStreamRef stream, CFRunLoopRef runLoop,
                                   CFStringRef runLoopMode)
{

}

