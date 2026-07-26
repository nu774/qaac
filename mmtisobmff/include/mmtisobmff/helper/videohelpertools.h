/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2016 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1. INTRODUCTION

The "Fraunhofer FDK MPEG-H Software" is software that implements the ISO/MPEG
MPEG-H 3D Audio standard for digital audio or related system features. Patent
licenses for necessary patent claims for the Fraunhofer FDK MPEG-H Software
(including those of Fraunhofer), for the use in commercial products and
services, may be obtained from the respective patent owners individually and/or
from Via LA (www.via-la.com).

Fraunhofer supports the development of MPEG-H products and services by offering
additional software, documentation, and technical advice. In addition, it
operates the MPEG-H Trademark Program to ease interoperability testing of end-
products. Please visit www.mpegh.com for more information.

2. COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

* You must retain the complete text of this software license in redistributions
of the Fraunhofer FDK MPEG-H Software or your modifications thereto in source
code form.

* You must retain the complete text of this software license in the
documentation and/or other materials provided with redistributions of
the Fraunhofer FDK MPEG-H Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of
the Fraunhofer FDK MPEG-H Software and your modifications thereto to recipients
of copies in binary form.

* The name of Fraunhofer may not be used to endorse or promote products derived
from the Fraunhofer FDK MPEG-H Software without prior written permission.

* You may not charge copyright license fees for anyone to use, copy or
distribute the Fraunhofer FDK MPEG-H Software or your modifications thereto.

* Your modified versions of the Fraunhofer FDK MPEG-H Software must carry
prominent notices stating that you changed the software and the date of any
change. For modified versions of the Fraunhofer FDK MPEG-H Software, the term
"Fraunhofer FDK MPEG-H Software" must be replaced by the term "Third-Party
Modified Version of the Fraunhofer FDK MPEG-H Software".

3. No PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software. You may use this Fraunhofer FDK MPEG-H Software or modifications
thereto only for purposes that are authorized by appropriate patent licenses.

4. DISCLAIMER

This Fraunhofer FDK MPEG-H Software is provided by Fraunhofer on behalf of the
copyright holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5. CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - MPEG-H FDK
Am Wolfsmantel 33
91058 Erlangen, Germany
www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
-----------------------------------------------------------------------------*/

/*!
 * @file videohelpertools.h
 * @brief Helper tools for video workflows
 *
 * Helper tools that allow AnnexB conversion, filling of config record, etc.
 */

#pragma once

// System includes
#include <memory>
#include <deque>

// Internal includes
#include "mmtisobmff/types.h"
#include "mmtisobmff/configdescriptor/avc_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/hevc_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/vvc_decoderconfigrecord.h"

namespace mmt {
namespace isobmff {
struct CIsobmffReader;

namespace tools {
/*! \addtogroup VideoHelperToolsReading Collection of video focused helpers used in MP4 file reading
 * workflows
 *  @{
 */

/*!
 * @brief Function to fill the @ref SAvcSample
 *
 * The @ref SAvcSample structure allows an AVC specific view on the underlying generic @ref CSample.
 * If using a generic (video) track reader, the resulting @ref CSample can be converted into an @ref
 * SAvcSample. This is not required when using an AVC specific reader or when the sample is only
 * copied into another track.
 *
 * This process requires the length of the prefixed sizes before every NALU in the @ref CSample
 * structure. This data is stored as a 'value - 1' version of it called 'lengthSizeMinusOne' in the
 * @ref config::CAvcDecoderConfigRecord.
 *
 * @param avcSample Codec specific sample implementation to be filled with NALU markers.
 * @param configRecord AVC config record to get length of the NALU size prefixes from.
 *
 * @code
 * // 1) Get sample from a generic track reader
 * CSample sample;
 * genericTrackReader.nextSample(sample)
 *
 * // 2) Get config record from a generic track reader
 * ilo::ByteBuffer configRecordBuffer = genericTrackReader.decoderConfigRecord();
 * config::CAvcDecoderConfigRecord configRecord{configRecordBuffer.begin(),
 * configRecordBuffer.end()}
 *
 * // 3) Create empty SAvcSample and copy CSample into it
 * SAvcSample avcSample;
 * avcSample.sample = sample;
 *
 * // 4) Use the helper function to parse the NALUs from the CSample and
 * // mark them in the SAvcSample to get a valid sample.
 * parseVideoSampleNalus(avcSample, configRecord);
 *
 * @endcode
 */
void parseVideoSampleNalus(SAvcSample& avcSample,
                           const config::CAvcDecoderConfigRecord& configRecord);
/*!
 * @brief Function to fill the @ref SHevcSample
 *
 * The @ref SHevcSample structure allows an HEVC specific view on the underlying generic @ref
 * CSample. If using a generic (video) track reader, the resulting @ref CSample can be converted
 * into an @ref SHevcSample. This is not required when using an HEVC specific reader or when the
 * sample is only copied into another track.
 *
 * This process requires the length of the prefixed sizes used before every NALU in the @ref CSample
 * structure. This data is stored as a 'value - 1' version of it called 'lengthSizeMinusOne' in the
 * @ref config::CHevcDecoderConfigRecord.
 *
 * @param hevcSample Codec specific sample implementation to be filled with NALU markers.
 * @param configRecord HEVC config record to get length of the NALU size prefixes from.
 *
 * @code
 * // 1) Get sample from a generic track reader
 * CSample sample;
 * genericTrackReader.nextSample(sample)
 *
 * // 2) Get config record from a generic track reader
 * ilo::ByteBuffer configRecordBuffer = genericTrackReader.decoderConfigRecord();
 * config::CHevcDecoderConfigRecord configRecord{configRecordBuffer.begin(),
 * configRecordBuffer.end()}
 *
 * // 3) Create empty SHevcSample and copy CSample into it
 * SHevcSample hevcSample;
 * hevcSample.sample = sample;
 *
 * // 4) Use the helper function to parse the NALUs from the CSample and
 * // mark them in the SHevcSample to get a valid sample.
 * parseVideoSampleNalus(hevcSample, configRecord);
 *
 * @endcode
 */
void parseVideoSampleNalus(SHevcSample& hevcSample,
                           const config::CHevcDecoderConfigRecord& configRecord);
/*!
 * @brief Function to fill the @ref SVvcSample
 *
 * The @ref SVvcSample structure allows a VVC specific view on the underlying generic @ref CSample.
 * If using a generic (video) track reader, the resulting @ref CSample can be converted into an @ref
 * SVvcSample. This is not required when using a VVC specific reader or when the sample is only
 * copied into another track.
 *
 * This process requires the length of the prefixed sizes used before every NALU in the @ref CSample
 * structure. This data is stored as a 'value - 1' version of it called 'lengthSizeMinusOne' in the
 * @ref config::CVvcDecoderConfigRecord.
 *
 * @param vvcSample Codec specific sample implementation to be filled with NALU markers.
 * @param configRecord VVC config record to get length of the NALU size prefixes from.
 *
 * @code
 * // 1) Get sample from a generic track reader
 * CSample sample;
 * genericTrackReader.nextSample(sample)
 *
 * // 2) Get config record from a generic track reader
 * ilo::ByteBuffer configRecordBuffer = genericTrackReader.decoderConfigRecord();
 * config::CVvcDecoderConfigRecord configRecord{configRecordBuffer.begin(),
 * configRecordBuffer.end()}
 *
 * // 3) Create empty SVvcSample and copy CSample into it
 * SVvcSample vvcSample;
 * vvcSample.sample = sample;
 *
 * // 4) Use the helper function to parse the NALUs from the CSample and
 * // mark them in the SVvcSample to get a valid sample.
 * parseVideoSampleNalus(vvcSample, configRecord);
 *
 * @endcode
 */
void parseVideoSampleNalus(SVvcSample& vvcSample,
                           const config::CVvcDecoderConfigRecord& configRecord);

/*!
 * @brief Function to convert an @ref SAvcSample to an AnnexB version of it
 *
 * Useful for video decoders that do not support the isobmff sample format, but only AnnexB streams.
 * During the process, the prefixed sizes are removed and replaced with AnnexB start codes. These
 * changes are performed on the underlying @ref CSample and the NALU markers are also updated
 * accordingly.
 *
 * @note If the used decoder does not require feeding a single NALU at a time, the payload of the
 * underlying @ref CSample can be fed to it directly.
 * @note The converted NALUs shall not be concatenated to form a standalone AnnexB stream without
 * ensuring required AccessUnitDelimiters (AUD NALUs) are present and any required emulation
 * prevention according to AnnexB is added. This is not handled by this function.
 */
void convertVideoSampleToAnnexBNalus(const SAvcSample& avcSample, SAvcSample& avcAnnexbSample);
/*!
 * @brief Function to convert an @ref SHevcSample to an AnnexB version of it
 *
 * Useful for video decoders that do not support the isobmff sample format, but only AnnexB streams.
 * During the process, the prefixed sizes are removed and replaced with AnnexB start codes. These
 * changes are performed on the underlying @ref CSample and the NALU markers are also updated
 * accordingly.
 *
 * @note If the used decoder does not require feeding a single NALU at a time, the payload of the
 * underlying @ref CSample can be fed to it directly.
 * @note The converted NALUs shall not be concatenated to form a standalone AnnexB stream without
 * ensuring required AccessUnitDelimiters (AUD NALUs) are present and any required emulation
 * prevention according to AnnexB is added. This is not handled by this function.
 */
void convertVideoSampleToAnnexBNalus(const SHevcSample& hevcSample, SHevcSample& hevcAnnexbSample);
/*!
 * @brief Function to convert an @ref SVvcSample to an AnnexB version of it
 *
 * Useful for video decoders that do not support the isobmff sample format, but only AnnexB streams.
 * During the process, the prefixed sizes are removed and replaced with AnnexB start codes. These
 * changes are performed on the underlying @ref CSample and the NALU markers are also updated
 * accordingly.
 *
 * @note If the used decoder does not require feeding a single NALU at a time, the payload of the
 * underlying @ref CSample can be fed to it directly.
 * @note The converted NALUs shall not be concatenated to form a standalone AnnexB stream without
 * ensuring required AccessUnitDelimiters (AUD NALUs) are present and any required emulation
 * prevention according to AnnexB is added. This is not handled by this function.
 */
void convertVideoSampleToAnnexBNalus(const SVvcSample& vvcSample, SVvcSample& vvcAnnexbSample);

/*!
 * @brief Function to extract non-VCL NALUs from the AVC config record and convert them to AnnexB
 *
 * Non-VCL NALUs are NALUs that do not contain coded picture data, but only metadata required to
 * decode the pictures in this MP4 track. Depending on the video coding flavor, those NALUs (like
 * SPS, PPS, etc.) are stored separately from the picture NALUs (VCL) in a config record.
 *
 * Non-VCL NALUs are usually fed into a decoder via a special interface. If the decoder only
 * supports AnnexB input, the data needs to be extracted and converted first.
 *
 * This function will extract all non-VCL NALUs from the config record and convert each of them to
 * AnnexB. The result is then stored in an @ref SAvcSample structure.
 *
 * @param configRecord AVC config record the get non-VCL NALUs from.
 * @param avcAnnexbSample AnnexB converted non-VCL NALUs.
 *
 * @note If the decoder does not have a special interface to accept non-VCL NALUs, they must be
 * muxed into the regular stream of VCL NALUs. For this, these non-VCL NALUs should be prepended
 * before every SyncSample or StreamAccessPoint (SAP).
 */
void convertNonVclNalusToAnnexBNalus(const config::CAvcDecoderConfigRecord& configRecord,
                                     SAvcSample& avcAnnexbSample);
/*!
 * @brief Function to extract non-VCL NALUs from the HEVC config record and convert them to AnnexB
 *
 * Non-VCL NALUs are NALUs that do not contain coded picture data, but further metadata required to
 * decode the pictures in this MP4 track. Depending on the video coding flavor, those NALUs (like
 * SPS, PPS, etc.) are stored separately from the picture NALUs (VCL) in a config record.
 *
 * Non-VCL NALUs are usually fed into a decoder via a special interface. If the decoder only
 * supports AnnexB input, the data needs to be extracted and converted first.
 *
 * This function will extract all non-VCL NALUs from the config record and convert each of them to
 * AnnexB. The result is then stored in an @ref SHevcSample structure.
 *
 * @param configRecord HEVC config record the get non-VCL NALUs from.
 * @param hevcAnnexbSample AnnexB converted non-VCL NALUs.
 *
 * @note If the decoder does not have a special interface to accept non-VCL NALUs, they must be
 * muxed into the regular stream of VCL NALUs. For this, these non-VCL NALUs should be prepended
 * before every SyncSample or StreamAccessPoint (SAP).
 */
void convertNonVclNalusToAnnexBNalus(const config::CHevcDecoderConfigRecord& configRecord,
                                     SHevcSample& hevcAnnexbSample);
/*!
 * @brief Function to extract non-VCL NALUs from the VVC config record and convert them to AnnexB
 *
 * Non-VCL NALUs are NALUs that do not contain coded picture data, but further metadata required to
 * decode the pictures in this MP4 track. Depending on the video coding flavor, those NALUs (like
 * SPS, PPS, etc.) are stored separately from the picture NALUs (VCL) in a config record.
 *
 * Non-VCL NALUs are usually fed into a decoder via a special interface. If the decoder only
 * supports AnnexB input, the data needs to be extracted and converted first.
 *
 * This function will extract all non-VCL NALUs from the config record and convert each of them to
 * AnnexB. The result is then stored in an @ref SVvcSample structure.
 *
 * @param configRecord VVC config record the get non-VCL NALUs from.
 * @param vvcAnnexbSample AnnexB converted non-VCL NALUs.
 *
 * @note If the decoder does not have a special interface to accept non-VCL NALUs, they must be
 * muxed into the regular stream of VCL NALUs. For this, these non-VCL NALUs should be prepended
 * before every SyncSample or StreamAccessPoint (SAP).
 */
void convertNonVclNalusToAnnexBNalus(const config::CVvcDecoderConfigRecord& configRecord,
                                     SVvcSample& vvcAnnexbSample);

/**@}*/

/*! \addtogroup VideoHelperToolsWriting Collection of video focused helpers used in MP4 file writing
 * workflows
 *  @{
 */

/*!
 * @brief Function to convert generic @ref SVideoNalus to a @ref SNaluSample based format.
 *
 * Generic sample format converter. Can be used to convert any supported @ref SVideoNalus
 * based format (like @ref SAvcNalus) into a matching @ref SNaluSample based format
 * (like @ref SAvcSample).
 *
 * Useful, if an encoder does not directly output isobmff formatted samples. The idea is to
 * use an @ref SVideoNalus based format and fill in the NALUs and metadata from the encoder.
 *
 * This function will then take care of creating an isobmff compatible sample by removing
 * any potential AnnexB start code and prefixing every NALU with a size field.
 *
 * @param videoNalus Video NALUs belonging to one picture.
 * @param lengthPrefixSize Length of the size prefix in bytes that will be written before each NALU
 * (Valid values are 1, 2 and 4).
 * @param naluSample Converted sample.
 *
 * @note This function will not remove any potential NALUs that are meant for global storage
 * in the config record and filtering must be applied beforehand according to the standard.
 * @warning This function is not required for use with @ref CTrackWriter functions, since the video
 * ones support both formats.
 */
void convertGeneralVideoNalusToVideoSample(const SVideoNalus& videoNalus, uint8_t lengthPrefixSize,
                                           SNaluSample& naluSample);

/*!
 * @brief Function to convert a byte buffer with AnnexB video samples (belonging to one picture)
 * into an isobmff @ref SNaluSample
 *
 * Useful, if an encoder does not directly output isobmff formatted samples, but (for example) only
 * AnnexB formatted buffers containing all NALUs belonging to one picture.
 *
 * The function will separate the NALUs, remove the AnnexB start codes and prefix each NALU with a
 * size (as required for isobmff storage). The converted NALUs are then stored together with the
 * provided metadata in an @ref SNaluSample format that is compatible with @ref CTrackWriter
 * functions.
 *
 * @param annexbBuffer Buffer consisting of NALUs belonging to one picture with AnnexB start codes
 * prefixed before each NALU.
 * @param metaData Sample metadata.
 * @param lengthPrefixSize Length of the size prefix in bytes that will be written before each NALU
 * (Valid values are 1, 2 and 4).
 * @param naluSample @ref SNaluSample to be filled with the converted sample data. It is required to
 * use a codec specific version of @ref SNaluSample for use with @ref CTrackWriter functions.
 *
 * @note The lengthPrefixSize has to be same for all samples of a track and needs to be stored
 * correctly in the appropriate codec specific config record.
 * @note This functions does not parse a standalone AnnexB stream with emulation prevention and AUD
 * NALUs. The buffer must solely be composed of NALUs belonging to one picture with each NALU being
 * prefixed with an AnnexB start code.
 * @see Codec specific versions of @ref CTrackWriter
 */
void convertAnnexbByteBufferToVideoSample(const ilo::ByteBuffer& annexbBuffer,
                                          const SVideoNalus::SMetaData& metaData,
                                          uint8_t lengthPrefixSize, SNaluSample& naluSample);

/*!
 * @brief Function to convert a byte buffer with AnnexB video samples (belonging to one picture)
 * into a byte buffer with pre-fixed lengths.
 *
 * Useful, if an encoder does not directly output isobmff formatted samples, but (for example) only
 * AnnexB formatted buffers containing all NALUs belonging to one picture.
 *
 * The function will separate the NALUs, remove the AnnexB start codes and prefix each NALU with a
 * size (as required for isobmff storage). This low-level function does not directly output a
 * structure that can be written with a track writer, but can be used to create a suitable payload
 * format for a @ref CSample.
 *
 * @param annexbBuffer Buffer consisting of NALUs belonging to one picture with AnnexB start codes
 * prefixed before each NALU.
 * @param lengthPrefixSize Length of the size prefix in bytes that will be written before each NALU
 * (Valid values are 1, 2 and 4).
 * @param sampleBuffer Buffer the converted NALUs are written to.
 *
 * @note The lengthPrefixSize has to be same for all samples of a track and needs to be stored
 * correctly in the appropriate codec specific config record.
 * @note This functions does not parse a standalone AnnexB stream with emulation prevention and AUD
 * NALUs. The buffer must solely be composed of NALUs belonging to one picture with each NALU being
 * prefixed with an AnnexB start code.
 * @see Codec specific versions of @ref CTrackWriter.
 */
void convertAnnexbByteBufferToVideoSampleBuffer(const ilo::ByteBuffer& annexbBuffer,
                                                uint8_t lengthPrefixSize,
                                                ilo::ByteBuffer& sampleBuffer);

/*!
 * @brief Function to fill an AVC decoder config record with non-VCL NALUs
 *
 * Function will convert (if necessary) and copy known non-VCL NALUs from the input into the
 * appropriate section of @ref config::CAvcDecoderConfigRecord.
 *
 * @param nonVclNalus NALU buffer to extract and copy non-VCL NALUs from.
 * @param configRecord AVC config record to copy non-VCL NALUs to.
 */
void fillNonVclNalusIntoConfigRecord(const SAvcNonVclNalus& nonVclNalus,
                                     config::CAvcDecoderConfigRecord& configRecord);
/*!
 * @brief Function to fill an HEVC decoder config record with non-VCL NALUs
 *
 * Function will convert (if necessary) and copy known non-VCL NALUs from the input into the
 * appropriate section of @ref config::CHevcDecoderConfigRecord.
 *
 * @param nonVclNalus NALU buffer to extract and copy non-VCL NALUs from.
 * @param configRecord HEVC config record to copy non-VCL NALUs to.
 * @param allArrayComplete Needs to be set to 1, if VPS, SPS and PPS are present and no in-band
 * updates are provided. Needs to be set to 0, if one of VPS, SPS or PPS are missing (partially or
 * completely).
 */
void fillNonVclNalusIntoConfigRecord(const SHevcNonVclNalus& nonVclNalus,
                                     config::CHevcDecoderConfigRecord& configRecord,
                                     bool allArrayComplete);
/*!
 * @brief Function to fill a VVC decoder config record with non-VCL NALUs
 *
 * Function will convert (if necessary) and copy known non-VCL NALUs from the input into the
 * appropriate section of @ref config::CVvcDecoderConfigRecord.
 *
 * @param nonVclNalus NALU buffer to extract and copy non-VCL NALUs from.
 * @param configRecord VVC config record to copy non-VCL NALUs to.
 * @param allArrayComplete Needs to be set to 1, if SPS and PPS are present and no in-band updates
 * are provided. If VPS is present it also must be complete without in-band updates. Needs to be set
 * to 0, if one of SPS or PPS are in-band (partially or complete) or VPS is in-band (partially or
 * complete).
 */
void fillNonVclNalusIntoConfigRecord(const SVvcNonVclNalus& nonVclNalus,
                                     config::CVvcDecoderConfigRecord& configRecord,
                                     bool allArrayComplete);

/**@}*/

}  // namespace tools
}  // namespace isobmff
}  // namespace mmt
