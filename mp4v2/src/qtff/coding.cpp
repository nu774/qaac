///////////////////////////////////////////////////////////////////////////////
//
//  The contents of this file are subject to the Mozilla Public License
//  Version 1.1 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.mozilla.org/MPL/
//
//  Software distributed under the License is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//  License for the specific language governing rights and limitations
//  under the License.
// 
//  The Original Code is MP4v2.
// 
//  The Initial Developer of the Original Code is Kona Blend.
//  Portions created by Kona Blend are Copyright (C) 2008.
//  All Rights Reserved.
//
//  Contributors:
//      Kona Blend, kona8lend@@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#include "impl.h"

namespace mp4v2 { namespace impl { namespace qtff {

///////////////////////////////////////////////////////////////////////////////

namespace {
    class StaticData
    {
    public:
        StaticData()
        {
            supportedCodings.insert( "avc1" );
            supportedCodings.insert( "mp4v" );
        }

        set<string> supportedCodings;
    };

    const StaticData STATIC_DATA;
}

///////////////////////////////////////////////////////////////////////////////

bool
findCoding( MP4FileHandle file, uint16_t trackIndex, MP4Atom*& coding )
{
    coding = NULL;
    MP4File& mp4 = *((MP4File*)file);

    if( trackIndex == numeric_limits<uint16_t>::max() ) {
        ostringstream xss;
        xss << "invalid track-index: " << trackIndex;
        throw new Exception( xss.str(), __FILE__, __LINE__, __FUNCTION__ );
    }

    ostringstream oss;
    oss << "moov.trak[" << trackIndex << "].mdia.hdlr";
    MP4Atom* hdlr = mp4.FindAtom( oss.str().c_str() );
    if( !hdlr )
        throw new Exception( "media handler not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4StringProperty* handlerType;
    if( !hdlr->FindProperty( "hdlr.handlerType", (MP4Property**)&handlerType ))
        throw new Exception( "media handler type-property not found", __FILE__, __LINE__, __FUNCTION__ );

    const string video = "vide";
    if( video != handlerType->GetValue() )
        throw new Exception( "video-track required", __FILE__, __LINE__, __FUNCTION__ );

    oss.str( "" );
    oss.clear();
    oss << "moov.trak[" << trackIndex << "].mdia.minf.stbl.stsd";
    MP4Atom* stsd = mp4.FindAtom( oss.str().c_str() );
    if( !stsd )
        throw new Exception( "media handler type-property not found", __FILE__, __LINE__, __FUNCTION__ );

    // find first atom which is a supported coding
    const uint32_t atomc = stsd->GetNumberOfChildAtoms();
    for( uint32_t i = 0; i < atomc; i++ ) {
        MP4Atom* atom = stsd->GetChildAtom( i );
        if( STATIC_DATA.supportedCodings.find( atom->GetType() ) == STATIC_DATA.supportedCodings.end() )
            continue;
        coding = atom;
    }

    return coding == NULL;
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::impl::qtff
