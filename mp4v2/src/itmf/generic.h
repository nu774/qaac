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
//      KonaBlend, kona8lend@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MP4V2_IMPL_ITMF_GENERIC_H
#define MP4V2_IMPL_ITMF_GENERIC_H

namespace mp4v2 { namespace impl { namespace itmf {

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItem*
genericItemAlloc( const string& code, uint32_t numData );

void
genericItemFree( MP4ItmfItem* item );

void
genericItemListFree( MP4ItmfItemList* list );

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItemList*
genericGetItems( MP4File& file );

MP4ItmfItemList*
genericGetItemsByCode( MP4File& file, const string& code );

MP4ItmfItemList*
genericGetItemsByMeaning( MP4File& file, const string& meaning, const string& name );

///////////////////////////////////////////////////////////////////////////////

bool
genericAddItem( MP4File& file, const MP4ItmfItem* item );

bool
genericSetItem( MP4File& file, const MP4ItmfItem* item );

bool
genericRemoveItem( MP4File& file, const MP4ItmfItem* item );

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::impl::itmf

#endif // MP4V2_IMPL_ITMF_GENERIC_H
