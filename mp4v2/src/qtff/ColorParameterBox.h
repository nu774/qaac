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

#ifndef MP4V2_IMPL_QTFF_COLORPARAMETERBOX_H
#define MP4V2_IMPL_QTFF_COLORPARAMETERBOX_H

namespace mp4v2 { namespace impl { namespace qtff {

///////////////////////////////////////////////////////////////////////////////

/// Functional class for colr-box (Color Parameter Box) support.
///
/// A colr-box is expected to be contained in a video track which is one of
/// the following coding types:
///     @li avc1
///     @li mp4v
///
/// This implementation assumes a maximum count of <b>1</b> for
/// VideoSampleEntry of the supported codings.
///
/// This implementation supports parameter-type 'nclc' only.
///
class MP4V2_EXPORT ColorParameterBox
{
public:
    /// Data object for colr-box item.
    /// This object correlates to one colr-box (Color Parameter Box).
    ///
    class MP4V2_EXPORT Item
    {
    public:
        Item();

        /// reset to state of newly constructed object.
        void reset();

        // convert from string CSV format.
        void convertFromCSV( const string& csv );

        // convert to string CSV format.
        string convertToCSV() const;

        // convert to string CSV format with buffer.
        string& convertToCSV( string& buffer ) const;

    public:
        /// a 16-bit unsigned integer index.
        /// Specifies an index into a table specifying the CIE 1931 xy
        /// chromaticity coordinates of the white point and the red, green, and
        /// blue primaries. The table of primaries specifies the white point and
        /// the red, green, and blue primary color points for a video system.
        uint16_t primariesIndex;

        /// a 16-bit unsigned integer index.
        /// Specifies an an index into a table specifying the nonlinear transfer
        /// function coefficients used to translate between RGB color space values
        /// and Y′CbCr values. The table of transfer function coefficients
        /// specifies the nonlinear function coefficients used to translate
        /// between the stored Y′CbCr values and a video capture or display
        /// system.
        uint16_t transferFunctionIndex;

        /// a 16-bit unsigned integer index.
        /// Specifies an index into a table specifying the transformation matrix
        /// coefficients used to translate between RGB color space values and
        /// Y′CbCr values. The table of matrixes specifies the matrix used during
        /// the translation.
        uint16_t matrixIndex;
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

    /// Add colr-box by track-index.
    ///
    /// This function adds a colr-box to <b>trackId</b> of <b>file</b>.
    /// The track must be a video-track and match one of the supporting
    /// codings.
    ///
    /// @param file on which to operate.
    /// @param trackIndex on which to operate.
    /// @param item colr-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool add( MP4FileHandle file, uint16_t trackIndex, const Item& item );

    /// Add colr-box by track-id.
    ///
    /// This function adds a colr-box to <b>trackId</b> of <b>file</b>.
    /// The track must be a video-track and match one of the supporting
    /// codings.
    ///
    /// @param file on which to operate.
    /// @param trackId on which to operate.
    /// @param item colr-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool add( MP4FileHandle file, MP4TrackId trackId, const Item& item );

    /// Store colr-box (Color Parameter Box) properties by track-index.
    ///
    /// This function sets the properties of a <b>colr-box</b>
    /// (Color Parameter Box).
    ///
    /// @param file on which to operate.
    /// @param trackIndex on which to operate.
    /// @param item colr-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool set( MP4FileHandle file, uint16_t trackIndex, const Item& item );

    /// Store colr-box (Color Parameter Box) properties by track-id.
    ///
    /// This function sets the properties of a <b>colr-box</b>
    /// (Color Parameter Box).
    ///
    /// @param file on which to operate.
    /// @param trackId on which to operate.
    /// @param item colr-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool set( MP4FileHandle file, MP4TrackId trackId, const Item& item );

    /// Fetch colr-box (Color Parameter Box) properties by track-index.
    ///
    /// This function gets the properties of a <b>colr-box</b>
    /// (Color Parameter Box).
    ///
    /// @param file on which to operate.
    /// @param trackIndex on which to operate.
    /// @param item colr-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool get( MP4FileHandle file, uint16_t trackIndex, Item& item );

    /// Fetch colr-box (Color Parameter Box) properties by track-id.
    ///
    /// This function gets the properties of a <b>colr-box</b>
    /// (Color Parameter Box).
    ///
    /// @param file on which to operate.
    /// @param trackId on which to operate.
    /// @param item colr-box properties to set.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool get( MP4FileHandle file, MP4TrackId trackId, Item& item );

    /// Remove colr-box (Color Parameter Box) by track-index.
    ///
    /// @param file on which to operate.
    /// @param trackIndex on which to operate.
    ///
    /// @return <b>true</b> on failure, <b>false</b> on success.
    ///
    static bool remove( MP4FileHandle file, uint16_t trackIndex );

    /// Remove colr-box (Color Parameter Box) by track-id.
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

#endif // MP4V2_IMPL_QTTF_COLORPARAMETERBOX_H
