#ifndef MP4V2_IMPL_TEXT_H
#define MP4V2_IMPL_TEXT_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

struct MP4V2_EXPORT LessIgnoreCase : less<string>
{
    bool operator()( const string&, const string& ) const;
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_TEXT_H
