/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001 - 2005.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie,               dmackie@cisco.com
 *      Alix Marchandise-Franquet, alix@cisco.com
 *      Bill May,                  wmay@cisco.com
 *      Kona Blend,                kona8lend@gmail.com
 *      Robert Kausch,             robert.kausch@freac.org
 */
#ifndef MP4V2_FILE_PROP_H
#define MP4V2_FILE_PROP_H

/**************************************************************************//**
 *
 *  @defgroup mp4_file_prop MP4v2 File Properties
 *  @{
 *
 *****************************************************************************/

/* generic props */

/** Check for presence of an atom.
 *
 *  MP4HaveAtom checks for the presence of the atom passed in @p atomName. @p
 *  atomName can specify an atom path to check for atoms that are not top level
 *  atoms, e.g. "moov.udta.meta.ilst".
 *
 *  @param hFile handle of file for operation.
 *  @param atomName name of the atom to check for.
 *
 *  @return true (1) if the atom is present, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4HaveAtom(
    MP4FileHandle hFile,
    const char*   atomName );

/** Get the value of an integer property.
 *
 *  MP4GetIntegerProperty determines the value of the integer property
 *  identified by @p propName, e.g. "moov.iods.audioProfileLevelId". The value
 *  is stored in the variable pointed to by @p retVal.
 *
 *  @param hFile handle of file for operation.
 *  @param propName path to the property to get.
 *  @param retVal pointer to a variable to receive the return value.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4GetIntegerProperty(
    MP4FileHandle hFile,
    const char*   propName,
    uint64_t*     retVal );

/** Get the value of a float property.
 *
 *  MP4GetFloatProperty determines the value of the float property identified
 *  by @p propName, e.g. "moov.mvhd.rate". The value is stored in the variable
 *  pointed to by @p retVal.
 *
 *  @param hFile handle of file for operation.
 *  @param propName path to the property to get.
 *  @param retVal pointer to a variable to receive the return value.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4GetFloatProperty(
    MP4FileHandle hFile,
    const char*   propName,
    float*        retVal );

/** Get the value of a string property.
 *
 *  MP4GetStringProperty determines the value of the string property identified
 *  by @p propName, e.g. "ftyp.majorBrand". The value is stored in the variable
 *  pointed to by @p retVal.
 *
 *  @param hFile handle of file for operation.
 *  @param propName path to the property to get.
 *  @param retVal pointer to a variable to receive the return value.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4GetStringProperty(
    MP4FileHandle hFile,
    const char*   propName,
    const char**  retVal );

/** Get the value of a bytes property.
 *
 *  MP4GetBytesProperty determines the value of the bytes property identified
 *  by @p propName, e.g. "moov.udta.meta.metadata". The value is stored in a
 *  newly allocated buffer the location of which is assigned to the variable
 *  pointed to by ppValue. The caller is responsible for freeing the memory
 *  with MP4Free().
 *
 *  @param hFile handle of file for operation.
 *  @param propName path to the property to get.
 *  @param ppValue pointer to a variable to receive the memory location
 *      containing the property bytes.
 *  @param pValueSize pointer to a variable to receive the length of the
 *      property bytes value.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4GetBytesProperty(
    MP4FileHandle hFile,
    const char*   propName,
    uint8_t**     ppValue,
    uint32_t*     pValueSize );

/** Set the value of an integer property.
 *
 *  MP4SetIntegerProperty sets the value of the integer property identified by
 *  @p propName, e.g. "moov.iods.audioProfileLevelId".
 *
 *  @param hFile handle of file for operation.
 *  @param propName path to the property to set.
 *  @param value the new value of the property.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4SetIntegerProperty(
    MP4FileHandle hFile,
    const char*   propName,
    int64_t       value );

/** Set the value of a float property.
 *
 *  MP4SetFloatProperty sets the value of the float property identified by @p
 *  propName, e.g. "moov.mvhd.rate".
 *
 *  @param hFile handle of file for operation.
 *  @param propName path to the property to set.
 *  @param value the new value of the property.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4SetFloatProperty(
    MP4FileHandle hFile,
    const char*   propName,
    float         value );

/** Set the value of a string property.
 *
 *  MP4SetStringProperty sets the value of the string property identified by @p
 *  propName, e.g. "ftyp.majorBrand".
 *
 *  @param hFile handle of file for operation.
 *  @param propName path to the property to set.
 *  @param value the new value of the property.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4SetStringProperty(
    MP4FileHandle hFile,
    const char*   propName,
    const char*   value );

/** Set the value of a bytes property.
 *
 *  MP4SetBytesProperty sets the value of the bytes property identified by @p
 *  propName, e.g. "moov.udta.meta.metadata".
 *
 *  @param hFile handle of file for operation.
 *  @param propName path to the property to set.
 *  @param pValue pointer the bytes representing the new value of the property.
 *  @param valueSize the size of the bytes value pointed to by <b>pValue</b>.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4SetBytesProperty(
    MP4FileHandle  hFile,
    const char*    propName,
    const uint8_t* pValue,
    uint32_t       valueSize );

/* specific props */

/** Get the duration of the movie (file).
 *
 *  MP4GetDuration returns the maximum duration of all the tracks in the
 *  specified mp4 file. 
 *
 *  Caveat: the duration is the movie (file) time scale units.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *
 *  @return The duration of the movie (file) in movie (file) time scale units.
 *
 *  @see MP4GetTimeScale()
 *  @see MP4ConvertFromMovieDuration()
 */
MP4V2_EXPORT
MP4Duration MP4GetDuration(
    MP4FileHandle hFile );

/** Get the time scale of the movie (file).
 *
 *  MP4GetTimeScale returns the time scale in units of ticks per second for
 *  the mp4 file. Caveat: tracks may use the same time scale as the movie
 *  or may use their own time scale.
 *
 *  @param hFile handle of file for operation.
 *
 *  @return timescale (ticks per second) of the mp4 file.
 */
MP4V2_EXPORT
uint32_t MP4GetTimeScale(
    MP4FileHandle hFile );

/** Set the time scale of the movie (file).
 *
 *  MP4SetTimeScale sets the time scale of the mp4 file. The time scale is
 *  in the number of clock ticks per second. Caveat:  tracks may use the
 *  same time scale as the movie or may use their own time scale.
 *
 *  @param hFile handle of file for operation.
 *  @param value desired timescale for the movie.
 *
 *  @return On success, true. On failure, false.
 */
MP4V2_EXPORT
bool MP4SetTimeScale(
    MP4FileHandle hFile,
    uint32_t      value );

/** Change the general timescale of file hFile.
 *
 *  This function changes the general timescale of the file <b>hFile</b>
 *  to the new timescale <b>value</b> by recalculating all values that depend
 *  on the timescale in "moov.mvhd".
 *
 *  If the timescale is already equal to value nothing is done.
 *
 *  @param hFile handle of file to change.
 *  @param value the new timescale.
 */
MP4V2_EXPORT
void MP4ChangeMovieTimeScale(
    MP4FileHandle hFile,
    uint32_t      value );

/** Gets the minimum MPEG-4 object descriptor profile and level required to
 *  render the contents of the file.
 *
 *  MP4GetODProfileLevel returns the minimum profile/level of MPEG-4 object
 *  descriptor support necessary to render the contents of the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *
 *  @return The current object descriptor profile/level for the file. See
 *      MP4SetODProfileLevel() for known values.
 *
 *  @see MP4SetODProfileLevel()
 */
MP4V2_EXPORT
uint8_t MP4GetODProfileLevel(
    MP4FileHandle hFile );

/** Sets the minimum MPEG-4 object descriptor profile and level required to
 *  render the contents of the file.
 *
 *  MP4SetODProfileLevel sets the minimum profile/level of MPEG-4 object
 *  descriptor support necessary to render the contents of the file.
 * 
 *  ISO/IEC 14496-1:2001 MPEG-4 Systems defines the following values:
 *
 *  Value     | Meaning
 *  ----------|-----------------------------
 *  0x00      | Reserved
 *  0x01-0x7F | Reserved
 *  0x80-0xFD | User private
 *  0xFE      | No object descriptor profile specified
 *  0xFF      | No object descriptor required
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param value specifies the profile/level to set.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetODProfileLevel()
 */
MP4V2_EXPORT
bool MP4SetODProfileLevel(
    MP4FileHandle hFile,
    uint8_t       value );

/** Gets the minimum MPEG-4 scene graph profile and level required to render
 *  the contents of the file.
 *
 *  MP4GetSceneProfileLevel returns the minimum profile/level of MPEG-4 scene
 *  graph support necessary to render the contents of the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *
 *  @return The current scene graph profile/level for the file. See
 *      MP4SetSceneProfileLevel() for known values.
 *
 *  @see MP4SetSceneProfileLevel()
 */
MP4V2_EXPORT
uint8_t MP4GetSceneProfileLevel(
    MP4FileHandle hFile );

/** Sets the minimum MPEG-4 scene graph profile and level required to render
 *  the contents of the file.
 *
 *  MP4SetSceneProfileLevel sets the minimum profile/level of MPEG-4 scene
 *  graph support necessary to render the contents of the file.
 * 
 *  ISO/IEC 14496-1:2001 MPEG-4 Systems defines the following values:
 *
 *  Value     | Meaning
 *  ----------|-----------------------------
 *  0x00      | Reserved
 *  0x01      | Simple 2D Profile @@ Level 1
 *  0x02-0x7F | Reserved
 *  0x80-0xFD | User private
 *  0xFE      | No scene graph profile specified
 *  0xFF      | No scene graph required
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param value specifies the profile/level to set.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetSceneProfileLevel()
 */
MP4V2_EXPORT
bool MP4SetSceneProfileLevel(
    MP4FileHandle hFile,
    uint8_t       value );

/** Gets the minimum MPEG-4 video profile and level required to render the
 *  contents of the file.
 *
 *  MP4GetVideoProfileLevel returns the minimum profile/level of MPEG-4 video
 *  support necessary to render the contents of the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the profile/level is
 *      requested.
 *
 *  @return The current video profile/level for the file/track. See
 *      MP4SetVideoProfileLevel() for known values.
 *
 *  @see MP4SetVideoProfileLevel()
 */
MP4V2_EXPORT
uint8_t MP4GetVideoProfileLevel(
    MP4FileHandle hFile,
    MP4TrackId    trackId DEFAULT(MP4_INVALID_TRACK_ID) );

/** Sets the minimum MPEG-4 video profile and level required to render the
 *  contents of the file.
 *
 *  MP4SetVideoProfileLevel sets the minimum profile/level of MPEG-4 video
 *  support necessary to render the contents of the file.
 * 
 *  ISO/IEC 14496-1:2001 MPEG-4 Systems defines the following values:
 *
 *  Value     | Meaning
 *  ----------|-----------------------------------
 *  0x00      | Reserved
 *  0x01      | Simple Profile @@ Level 3
 *  0x02      | Simple Profile @@ Level 2
 *  0x03      | Simple Profile @@ Level 1
 *  0x04      | Simple Scalable Profile @@ Level 2
 *  0x05      | Simple Scalable Profile @@ Level 1
 *  0x06      | Core Profile @@ Level 2
 *  0x07      | Core Profile @@ Level 1
 *  0x08      | Main Profile @@ Level 4
 *  0x09      | Main Profile @@ Level 3
 *  0x0A      | Main Profile @@ Level 2
 *  0x0B      | N-Bit Profile @@ Level 2
 *  0x0C      | Hybrid Profile @@ Level 2
 *  0x0D      | Hybrid Profile @@ Level 1
 *  0x0E      | Basic Animated Texture @@ Level 2
 *  0x0F      | Basic Animated Texture @@ Level 1
 *  0x10      | Scalable Texture @@ Level 3
 *  0x11      | Scalable Texture @@ Level 2
 *  0x12      | Scalable Texture @@ Level 1
 *  0x13      | Simple Face Animation @@ Level 2
 *  0x14      | Simple Face Animation @@ Level 1
 *  0x15-0x7F | Reserved
 *  0x80-0xFD | User private
 *  0xFE      | No video profile specified
 *  0xFF      | No video required
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param value specifies the profile/level to set.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetVideoProfileLevel()
 */
MP4V2_EXPORT
void MP4SetVideoProfileLevel(
    MP4FileHandle hFile,
    uint8_t       value );

/** Gets the minimum MPEG-4 audio profile and level required to render the
 *  contents of the file.
 *
 *  MP4GetAudioProfileLevel returns the minimum profile/level of MPEG-4 audio
 *  support necessary to render the contents of the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *
 *  @return The current audio profile/level for the file. See
 *      MP4SetAudioProfileLevel() for known values.
 *
 *  @see MP4SetAudioProfileLevel()
 */
MP4V2_EXPORT
uint8_t MP4GetAudioProfileLevel(
    MP4FileHandle hFile );

/** Sets the minimum MPEG-4 audio profile and level required to render the
 *  contents of the file.
 *
 *  MP4SetAudioProfileLevel sets the minimum profile/level of MPEG-4 audio
 *  support necessary to render the contents of the file.
 * 
 *  ISO/IEC 14496-1:2001 MPEG-4 Systems defines the following values:
 *
 *  Value     | Meaning
 *  ----------|-----------------------------
 *  0x00      | Reserved
 *  0x01      | Main Profile @@ Level 1
 *  0x02      | Main Profile @@ Level 2
 *  0x03      | Main Profile @@ Level 3
 *  0x04      | Main Profile @@ Level 4
 *  0x05      | Scalable Profile @@ Level 1
 *  0x06      | Scalable Profile @@ Level 2
 *  0x07      | Scalable Profile @@ Level 3
 *  0x08      | Scalable Profile @@ Level 4
 *  0x09      | Speech Profile @@ Level 1
 *  0x0A      | Speech Profile @@ Level 2
 *  0x0B      | Synthesis Profile @@ Level 1
 *  0x0C      | Synthesis Profile @@ Level 2
 *  0x0D      | Synthesis Profile @@ Level 3
 *  0x0E-0x7F | Reserved
 *  0x80-0xFD | User private
 *  0xFE      | No audio profile specified
 *  0xFF      | No audio required
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param value specifies the profile/level to set.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetAudioProfileLevel()
 */
MP4V2_EXPORT
void MP4SetAudioProfileLevel(
    MP4FileHandle hFile,
    uint8_t       value );

/** Gets the minimum MPEG-4 graphics profile and level required to render the
 *  contents of the file.
 *
 *  MP4GetGraphicsProfileLevel returns the minimum profile/level of MPEG-4
 *  graphics support necessary to render the contents of the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *
 *  @return The current graphics profile/level for the file. See
 *      MP4SetGraphicsProfileLevel() for known values.
 *
 *  @see MP4SetGraphicsProfileLevel()
 */
MP4V2_EXPORT
uint8_t MP4GetGraphicsProfileLevel(
    MP4FileHandle hFile );

/** Sets the minimum MPEG-4 graphics profile and level required to render the
 *  contents of the file.
 *
 *  MP4SetGraphicsProfileLevel sets the minimum profile/level of MPEG-4
 *  graphics support necessary to render the contents of the file.
 * 
 *  ISO/IEC 14496-1:2001 MPEG-4 Systems defines the following values:
 *
 *  Value     | Meaning
 *  ----------|-----------------------------
 *  0x00      | Reserved
 *  0x01      | Simple 2D Profile @@ Level 1
 *  0x02-0x7F | Reserved
 *  0x80-0xFD | User private
 *  0xFE      | No graphics profile specified
 *  0xFF      | No graphics required
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param value specifies the profile/level to set.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetGraphicsProfileLevel()
 */
MP4V2_EXPORT
bool MP4SetGraphicsProfileLevel(
    MP4FileHandle hFile,
    uint8_t       value );

/** @} ***********************************************************************/

#endif /* MP4V2_FILE_PROP_H */
