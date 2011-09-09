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

#ifndef MP4V2_IMPL_QTFF_PICTUREAPSECTRATIOBOX_H
#define MP4V2_IMPL_QTFF_PICTUREAPSECTRATIOBOX_H

namespace mp4v2 { namespace impl { namespace qtff {
    using namespace std;

///////////////////////////////////////////////////////////////////////////////

/// Functional class for pasp-box (Picture Aspect Ratio Box) support.
///
/// A pasp-box is expected to be contained in a video track which is one of
/// the following coding types:
///     @li avc1
///     @li mp4v
///
/// This implementation assumes a maximum count of <b>1</b> for
/// VideoSampleEntry of the supported codings.
///
class MP4V2_EXPORT PictureAspectRatioBox
{
public:
    /// Data object for pasp-box item.
    /// This object correlates to one pasp-box (Picture Aspect Ratio Box).
    class MP4V2_EXPORT Item
    {
    public:
        Item ();

        /// reset to state of newly constructed object.
        void reset();

        // convert from string CSV format.
        void convertFromCSV( const string& csv );

        // convert to string CSV format.
        string convertToCSV() const;

        // convert to string CSV format with buffer.
        string& convertToCSV( string& buffer ) const;

    public:
        /// an unsigned 32-bit integer specifying the vertical spacing of pixels.
        uint32_t hSpacing;

        /// an unsigned 32-bit integer specifying the horizontal spacing of pixels.
        uint32_t vSpacing;
    };

    class MP4V2_EXPORT IndexedItem {
    public:
        IndexedItem();

        uint16_t trackIndex;
        uint16_t trackId;
        Item     item;
    };

    typedef vector<IndexedItem> ItemList;

    static bool list( MP4FileHandle file, ItemList& itemList );

    /// Add pasp-box by track-index.
    ///
    /// This function adds a pasp-box to <b>trackId</b> of <b>file</b>.
    /// The track must be a video-track and match one of the supporting
    /// codings.
    ///
    /// @param file on which to operate.
    /// @param trackIndex on which to operate.
    /// @param item pasp-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool add( MP4FileHandle file, uint16_t trackIndex, const Item& item );

    /// Add pasp-box by track-id.
    ///
    /// This function adds a pasp-box to <b>trackId</b> of <b>file</b>.
    /// The track must be a video-track and match one of the supporting
    /// codings.
    ///
    /// @param file on which to operate.
    /// @param trackId on which to operate.
    /// @param item pasp-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool add( MP4FileHandle file, MP4TrackId trackId, const Item& item );

    /// Store pasp-box (Color Parameter Box) properties by track-index.
    ///
    /// This function sets the properties of a <b>pasp-box</b>
    /// (Color Parameter Box).
    ///
    /// @param file on which to operate.
    /// @param trackIndex on which to operate.
    /// @param item pasp-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool set( MP4FileHandle file, uint16_t trackIndex, const Item& item );

    /// Store pasp-box (Color Parameter Box) properties by track-id.
    ///
    /// This function sets the properties of a <b>pasp-box</b>
    /// (Color Parameter Box).
    ///
    /// @param file on which to operate.
    /// @param trackId on which to operate.
    /// @param item pasp-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool set( MP4FileHandle file, MP4TrackId trackId, const Item& item );

    /// Fetch pasp-box (Color Parameter Box) properties by track-index.
    ///
    /// This function gets the properties of a <b>pasp-box</b>
    /// (Color Parameter Box).
    ///
    /// @param file on which to operate.
    /// @param trackIndex on which to operate.
    /// @param item pasp-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool get( MP4FileHandle file, uint16_t trackIndex, Item& item );

    /// Fetch pasp-box (Color Parameter Box) properties by track-id.
    ///
    /// This function gets the properties of a <b>pasp-box</b>
    /// (Color Parameter Box).
    ///
    /// @param file on which to operate.
    /// @param trackId on which to operate.
    /// @param item pasp-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool get( MP4FileHandle file, MP4TrackId trackId, Item& item );

    /// Remove pasp-box (Color Parameter Box) by track-index.
    ///
    /// @param file on which to operate.
    /// @param trackIndex on which to operate.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool remove( MP4FileHandle file, uint16_t trackIndex );

    /// Remove pasp-box (Color Parameter Box) by track-id.
    ///
    /// @param file on which to operate.
    /// @param trackId on which to operate.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool remove( MP4FileHandle file, MP4TrackId trackId );
};

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::impl::qtff

#endif // MP4V2_IMPL_QTTF_PICTUREAPSECTRATIOBOX_H
