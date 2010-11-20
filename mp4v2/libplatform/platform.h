#ifndef MP4V2_PLATFORM_PLATFORM_H
#define MP4V2_PLATFORM_PLATFORM_H

/// @namespace mp4v2::platform (private) Platform abstraction.
/// <b>WARNING: THIS IS A PRIVATE NAMESPACE. NOT FOR PUBLIC CONSUMPTION.</b>
///
/// This namespace implements platform abstractions that are useful for
/// keeping the code base portable.

/// @namespace mp4v2::platform::io (private) I/O.
/// <b>WARNING: THIS IS A PRIVATE NAMESPACE. NOT FOR PUBLIC CONSUMPTION.</b>

/// @namespace mp4v2::platform::number (private) Number.
/// <b>WARNING: THIS IS A PRIVATE NAMESPACE. NOT FOR PUBLIC CONSUMPTION.</b>

/// @namespace mp4v2::platform::sys (private) System.
/// <b>WARNING: THIS IS A PRIVATE NAMESPACE. NOT FOR PUBLIC CONSUMPTION.</b>

///////////////////////////////////////////////////////////////////////////////

#if defined( _WIN32 )
#   include "libplatform/platform_win32.h"
#else
#   include "libplatform/platform_posix.h"
#endif

///////////////////////////////////////////////////////////////////////////////

#include "libplatform/warning.h"
#include "libplatform/endian.h"

#include "libplatform/io/File.h"
#include "libplatform/io/FileSystem.h"

#include "libplatform/number/random.h"
#include "libplatform/process/process.h"
#include "libplatform/prog/option.h"
#include "libplatform/sys/error.h"
#include "libplatform/time/time.h"

///////////////////////////////////////////////////////////////////////////////

#endif // MP4V2_PLATFORM_PLATFORM_H
