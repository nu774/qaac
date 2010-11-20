#ifndef MP4V2_PLATFORM_TIME_TIME_H
#define MP4V2_PLATFORM_TIME_TIME_H

/// @namespace mp4v2::platform::time (private) Time.
/// <b>WARNING: THIS IS A PRIVATE NAMESPACE. NOT FOR PUBLIC CONSUMPTION.</b>
namespace mp4v2 { namespace platform { namespace time {

//! type used to represent milliseconds
typedef int64_t milliseconds_t;

//! type used to represent seconds
typedef int64_t seconds_t;

///////////////////////////////////////////////////////////////////////////////
//!
//! Get local-time in milliseconds.
//!
//! getLocalTimeMilliseconds obtains the system's notion of current Greenwich
//! time, adjusted according to the current timezone of the host system.
//! The time is expressed as an absolute value since midnight (0 hour),
//! January 1, 1970. This is commonly referred to as the "epoch".
//!
//! @return local-time in milliseconds elapsed since the epoch.
//!
///////////////////////////////////////////////////////////////////////////////
MP4V2_EXPORT milliseconds_t getLocalTimeMilliseconds();

///////////////////////////////////////////////////////////////////////////////
//!
//! Get local-time in seconds.
//!
//! getLocalTimeMilliseconds obtains the system's notion of current Greenwich
//! time, adjusted according to the current timezone of the host system.
//! The time is expressed as an absolute value since midnight (0 hour),
//! January 1, 1970. This is commonly referred to as the "epoch".
//!
//! @return local-time in seconds elapsed since the epoch.
//!
///////////////////////////////////////////////////////////////////////////////
MP4V2_EXPORT seconds_t getLocalTimeSeconds();

///////////////////////////////////////////////////////////////////////////////
//! @}
///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::time

#endif // MP4V2_PLATFORM_TIME_TIME_H
