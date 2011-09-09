#ifndef MP4V2_IMPL_UTIL_H
#define MP4V2_IMPL_UTIL_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

inline int8_t max( int8_t a, int8_t b ) {
    return ( a > b ) ? a : b;
}

inline int16_t max( int16_t a, int16_t b ) {
    return ( a > b ) ? a : b;
}

inline int32_t max( int32_t a, int32_t b ) {
    return ( a > b ) ? a : b;
}

inline int64_t max( int64_t a, int64_t b ) {
    return ( a > b ) ? a : b;
}

///////////////////////////////////////////////////////////////////////////////

inline uint8_t max( uint8_t  a, uint8_t b ) {
    return ( a > b ) ? a : b;
}

inline uint16_t max( uint16_t a, uint16_t b ) {
    return ( a > b ) ? a : b;
}

inline uint32_t max( uint32_t a, uint32_t b ) {
    return ( a > b ) ? a : b;
}

inline uint64_t max( uint64_t a, uint64_t b ) {
    return ( a > b ) ? a : b;
}

///////////////////////////////////////////////////////////////////////////////

inline int8_t min( int8_t  a, int8_t b ) {
    return ( a < b ) ? a : b;
}

inline int16_t min( int16_t a, int16_t b ) {
    return ( a < b ) ? a : b;
}

inline int32_t min( int32_t a, int32_t b ) {
    return ( a < b ) ? a : b;
}

inline int64_t min( int64_t a, int64_t b ) {
    return ( a < b ) ? a : b;
}

///////////////////////////////////////////////////////////////////////////////

inline uint8_t min( uint8_t  a, uint8_t b ) {
    return ( a < b ) ? a : b;
}

inline uint16_t min( uint16_t a, uint16_t b ) {
    return ( a < b ) ? a : b;
}

inline uint32_t min( uint32_t a, uint32_t b ) {
    return ( a < b ) ? a : b;
}

inline uint64_t min( uint64_t a, uint64_t b ) {
    return ( a < b ) ? a : b;
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_UTIL_H
