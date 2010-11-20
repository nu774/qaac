#ifndef MP4V2_PLATFORM_NUMBER_RANDOM_H
#define MP4V2_PLATFORM_NUMBER_RANDOM_H

namespace mp4v2 { namespace platform { namespace number {

///////////////////////////////////////////////////////////////////////////////

/// Generate 32-bit pseudo-random number.
MP4V2_EXPORT uint32_t random32();

/// Seed pseudo-random number generator.
MP4V2_EXPORT void srandom( uint32_t );

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::number

#endif // MP4V2_PLATFORM_NUMBER_RANDOM_H
