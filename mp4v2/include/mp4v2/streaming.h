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
 *      Robert Kausch,             robert.kausch@freac.org
 */
#ifndef MP4V2_STREAMING_H
#define MP4V2_STREAMING_H

/**************************************************************************//**
 *
 *  @defgroup mp4_hint MP4v2 Streaming
 *  @{
 *
 *****************************************************************************/

/** Get the RTP payload parameters of the hint track.
 *
 *  MP4GetHintTrackRtpPayload gets the RTP payload parameters for the hint
 *  track. The RTP payload is the set of rules by which media samples are
 *  packed into RTP packets. This call is typically used in constructing the
 *  SDP media level description for the hint track.
 *
 *  The payloadName identifies which RTP payload is being used for the RTP
 *  packets created from the hint track. This value is sent to the receiver in
 *  the SDP description. For example, MP3 audio sent according to the rules in
 *  IETF RFC 2250 uses the name "MPA" for the RTP payload.
 *
 *  The payloadNumber is a shorter form of the payloadName. This value is
 *  associated with the payload name in the SDP description and then sent in
 *  every RTP packet. Payload numbers 1 thru 95 are statically assigned in IETF
 *  RFC 1890, numbers 96 thru 127 are dynamically assigned within a session.
 *
 *  The maxPayloadSize specifies the maximum number of bytes that should be
 *  placed in the RTP payload section of the RTP packets.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param ppPayloadName specifies a pointer to the variable to receive the
 *      string RTP payload name.
 *  @param pPayloadNumber specifies a pointer to the variable to receive the
 *      RTP payload number.
 *  @param pMaxPayloadSize specifies a pointer to the variable to receive the
 *      maximum RTP payload size in bytes.
 *  @param ppEncodingParams
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4SetHintTrackRtpPayload()
 */
MP4V2_EXPORT
bool MP4GetHintTrackRtpPayload(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    char**        ppPayloadName DEFAULT(NULL),
    uint8_t*      pPayloadNumber DEFAULT(NULL),
    uint16_t*     pMaxPayloadSize DEFAULT(NULL),
    char**        ppEncodingParams DEFAULT(NULL) );

#define MP4_SET_DYNAMIC_PAYLOAD 0xff

/** Set the RTP payload parameters of the hint track.
 *
 *  MP4SetHintTrackRtpPayload sets the RTP payload parameters for the hint
 *  track. The RTP payload is the set of rules by which media samples are
 *  packed into RTP packets.
 *
 *  The payload name identifies which RTP payload is being used for the RTP
 *  packets created from the hint track. This value is sent to the receiver in
 *  the SDP description. For example, MP3 audio sent according to the rules in
 *  IETF RFC 2250 uses the name "MPA" for the RTP payload.
 *
 *  The payload number is a shorter form of the payload name. This value is
 *  associated with the payload name in the SDP description and then sent in
 *  every RTP packet. Payload numbers 1 thru 95 are statically assigned in IETF
 *  RFC 1890, numbers 96 thru 127 are dynamically assigned within a session. If
 *  the RTP payload in use is one of the statically assigned ones, you should
 *  pass this value to the library. If you need a dynamic payload number
 *  assigned, pass the define value MP4_SET_DYNAMIC_PAYLOAD for this parameter
 *  and the library will choose an valid available number and return this
 *  value.
 *
 *  The maxPayloadSize specifies the maximum number of bytes that should be
 *  placed in the RTP payload section of the RTP packets. It is desirable that
 *  RTP packets not exceed the maximum transmission unit (MTU) of the IP
 *  network they travel over since otherwise the packets must be fragmented at
 *  the IP level which consumes router resources and can lead to less robust
 *  behavior in the face of packet loss.
 *
 *  The default value for maxPayloadSize is 1460, which is the MTU for an
 *  Ethernet or similar network minus the standard sizes of the IP, UDP, and
 *  RTP headers (1500 - 20 - 8 - 12 = 1460).
 *
 *  If you anticipate streaming over IP networks with smaller MTU sizes, or
 *  that extensions to the network headers might be used, a more conservative
 *  value should be chosen. The minimum MTU for an IP network is 576 bytes.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param pPayloadName specifies the RTP payload name.
 *  @param pPayloadNumber specifies a pointer to the RTP payload number.
 *  @param maxPayloadSize specifies the maximum RTP payload size in bytes.
 *  @param encode_params
 *  @param include_rtp_map specifies if the a=rtpmap statement is included.
 *  @param include_mpeg4_esid specifies if the a=mpeg4-esid statement is
 *      included.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetHintTrackRtpPayload()
 */
MP4V2_EXPORT
bool MP4SetHintTrackRtpPayload(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    const char*   pPayloadName,
    uint8_t*      pPayloadNumber,
    uint16_t      maxPayloadSize DEFAULT(0),
    const char *  encode_params DEFAULT(NULL),
    bool          include_rtp_map DEFAULT(true),
    bool          include_mpeg4_esid DEFAULT(true) );

/** Get the SDP session level description of the file.
 *
 *  MP4GetSessionSdp returns the SDP (IETF RFC 2327) session level fragment for
 *  the file. This is used by a streaming server to create a complete SDP
 *  description of the multimedia session represented by the file.
 *
 *  The mp4broadcaster test program provided with the MP4v2 library gives an
 *  example of using this call to create the complete SDP description.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *
 *  @return The SDP session level description fragment of the mp4 file.
 *
 *  @see MP4SetSessionSdp()
 *  @see MP4AppendSessionSdp()
 *  @see MP4GetHintTrackSdp()
 */
MP4V2_EXPORT
const char* MP4GetSessionSdp(
    MP4FileHandle hFile );

/** Set the SDP session level description of the file.
 *
 *  MP4SetSessionSdp sets the SDP (IETF RFC 2327) session level fragment for
 *  the file. This is used by a streaming server to create a complete SDP
 *  description of the multimedia session represented by the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param sdpString specifies the new value of the session SDP string.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetSessionSdp()
 *  @see MP4AppendSessionSdp()
 */
MP4V2_EXPORT
bool MP4SetSessionSdp(
    MP4FileHandle hFile,
    const char*   sdpString );

/** Add to the SDP session level description of the file.
 *
 *  MP4AppendSessionSdp appends the specified string to the SDP (IETF RFC 2327)
 *  session level fragment for the file. This is used by a streaming server to
 *  create a complete SDP description of the multimedia session represented by
 *  the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param sdpString specifies the addition to the session SDP string.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetSessionSdp()
 *  @see MP4SetSessionSdp()
 */
MP4V2_EXPORT
bool MP4AppendSessionSdp(
    MP4FileHandle hFile,
    const char*   sdpString );

/** Get the SDP media level description associated with a hint track.
 *
 *  MP4GetHintTrackSdp returns the SDP (IETF RFC 2327) media level fragment
 *  associated with the hint track. This is used by a streaming server to
 *  create a complete SDP description of the multimedia session represented by
 *  the file.
 *
 *  The mp4broadcaster test program provided with the MP4 library gives an
 *  example of using this call to create the complete SDP description.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *
 *  @return SDP media level description fragment associated with the hint
 *      track.
 *
 *  @see MP4SetHintTrackSdp()
 *  @see MP4AppendHintTrackSdp()
 *  @see MP4GetSessionSdp()
 */
MP4V2_EXPORT
const char* MP4GetHintTrackSdp(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId );

/** Set the SDP media level description of the hint track.
 *
 *  MP4SetHintTrackSdp sets the SDP (IETF RFC 2327) media level fragment for
 *  the hint track. This is used by a streaming server to create a complete SDP
 *  description of the multimedia session represented by the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param sdpString specifies the new value of the hint track SDP string.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetHintTrackSdp()
 *  @see MP4AppendHintTrackSdp()
 */
MP4V2_EXPORT
bool MP4SetHintTrackSdp(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    const char*   sdpString );

/** Add to the SDP media level description of the hint track.
 *
 *  MP4AppendHintTrackSdp appends the specified string to the SDP (IETF RFC
 *  2327) media level fragment for the hint track. This is used by a streaming
 *  server to create a complete SDP description of the multimedia session
 *  represented by the file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param sdpString specifies the addition to the hint track SDP string.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetHintTrackSdp()
 *  @see MP4AppendHintTrackSdp()
 */
MP4V2_EXPORT
bool MP4AppendHintTrackSdp(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    const char*   sdpString );

/** Get the reference track id for a hint track.
 *
 *  MP4GetHintTrackReferenceTrackId gets the track id of the reference media
 *  track associated with the specified hint track.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *
 *  @return Upon success, the track id of the reference media track. Upon an
 *      error, MP4_INVALID_TRACK_ID.
 *
 *  @see MP4AddHintTrack()
 */
MP4V2_EXPORT
MP4TrackId MP4GetHintTrackReferenceTrackId(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId );

/** Read an RTP hint.
 *
 *  MP4ReadRtpHint reads the specified hint sample from the specified hint
 *  track and enables subsequent calls to MP4ReadRtpPacket() to read the
 *  individual RTP packets associated with this hint. If desired, the number of
 *  RTP packets associated with this hint is returned.
 *
 *  Note that a hint track sample is just like any other track sample. I.e.
 *  MP4ReadSample(), MP4GetSampleSize(), MP4GetSampleTime(), etc. are all
 *  valid. The RTP specific functions are provided to interpret the information
 *  contain in the hint track samples that give instructions on how to form the
 *  actual RTP packets.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param hintSampleId specifies which hint sample is to be read. Caveat: the
 *      first sample has id 1 not 0.
 *  @param pNumPackets Pointer to variable that will be hold the number of
 *      packets in the hint.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4ReadRtpPacket()
 */
MP4V2_EXPORT
bool MP4ReadRtpHint(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    MP4SampleId   hintSampleId,
    uint16_t*     pNumPackets DEFAULT(NULL) );

/** Get the number of packets in an RTP hint.
 *
 *  MP4GetRtpHintNumberOfPackets returns the number of packets contained in the
 *  current RTP hint as established by a call to MP4ReadRtpHint().
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *
 *  @return Upon success, the number of packets in the current RTP hint. Upon
 *      an error, 0.
 *
 *  @see MP4ReadRtpHint()
 */
MP4V2_EXPORT
uint16_t MP4GetRtpHintNumberOfPackets(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId );

/** Get the B frame flag of an RTP packet.
 *
 *  MP4GetRtpPacketBFrame returns the state of the B Frame flag of an RTP
 *  packet. See MP4AddRtpHint() for a description of this flag.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param packetIndex specifies the packet to which the operation applies.
 *
 *  @return Upon success, the state of the B frame flag for the specified
 *      packet. Upon an error, -1.
 *
 *  @see MP4AddRtpHint()
 *  @see MP4ReadRtpPacket()
 */
MP4V2_EXPORT
int8_t MP4GetRtpPacketBFrame(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    uint16_t      packetIndex );

/** Get the transmit offset of an RTP packet.
 *
 *  MP4GetRtpPacketTransmitOffset returns the transmit offset of an RTP packet.
 *  This offset may be set by some hinters to smooth out the packet
 *  transmission times and reduce network burstiness. A transmitter would need
 *  to apply this offset to the calculated transmission time based on the hint
 *  start time.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param packetIndex specifies the packet to which the operation applies.
 *
 *  @return The transmit offset for the specified packet in the hint track
 *      timescale.
 *
 *  @see MP4AddRtpHint()
 *  @see MP4ReadRtpPacket()
 */
MP4V2_EXPORT
int32_t MP4GetRtpPacketTransmitOffset(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    uint16_t      packetIndex );

/** Read an RTP packet.
 *
 *  MP4ReadRtpPacket reads the specified packet from the current hint sample,
 *  as previously read by MP4ReadRtpHint().
 *
 *  The argument ppBytes allows for two possible approaches for buffering:
 *
 *  @li If the calling application wishes to handle its own buffering it can
 *      set *ppBytes to the buffer it wishes to use. The calling application is
 *      responsible for ensuring that the buffer is large enough to hold the
 *      packet. This can be done by using MP4GetRtpPayload() to retrieve the
 *      maximum packet payload size and hence how large the receiving buffer
 *      must be. Caveat: the value returned by MP4GetRtpPayload() is the
 *      maximum payload size, if the RTP packet header is going to be included
 *      by the library this value should be incremented by 12.
 *
 *  @li If the value of *ppBytes is NULL, then an appropriately sized buffer is
 *      automatically allocated for the sample data and *ppBytes set to this
 *      pointer. The calling application is responsible for freeing this memory
 *      with MP4Free().
 *
 *  The application is expected to provide the value of the RTP SSRC identifier
 *  which uniquely identifies the originator of the media stream. For most
 *  applications, a single random value can be provided. The value should be
 *  the same for all packets for the duration of the RTP transmission. If the
 *  parameter @p includeHeader is false, then this value has no effect.
 *
 *  By default the library constructs the standard 12 byte RTP header from the
 *  information in the hint sample, and the specified SSRC. It then
 *  concatenates the RTP header with the packet payload, that is the RTP
 *  payload specific header and the media data for the packet. The @p
 *  includeHeader and @p includePayload parameters allow control over these
 *  steps, so that either just the packet payloads or just the RTP headers can
 *  be returned. A potential use of this feature is if the calling application
 *  wishes to construct an extended RTP header with non-standard options.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param packetIndex specifies which packet is to be read. Valid values range
 *      from zero to the number of packets in this hint minus one.
 *  @param ppBytes Pointer to the pointer to the packet data. See the function
 *      description for details on this argument. 
 *  @param pNumBytes Pointer to variable that will be hold the size in bytes of
 *      the packet.
 *  @param ssrc specifies the RTP SSRC to be used when constructing the RTP
 *      packet header.
 *  @param includeHeader specifies whether the library should include the
 *      standard 12 byte RTP header to the returned packet. The header is
 *      constructed from the information in the hint sample and the specified
 *      ssrc.
 *  @param includePayload specifies whether the library should include the
 *      packet payload (RTP payload header and media data) in the returned
 *      packet.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4ReadRtpHint()
 */
MP4V2_EXPORT
bool MP4ReadRtpPacket(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    uint16_t      packetIndex,
    uint8_t**     ppBytes,
    uint32_t*     pNumBytes,
    uint32_t      ssrc DEFAULT(0),
    bool          includeHeader DEFAULT(true),
    bool          includePayload DEFAULT(true) );

/** Get the RTP start time of a hint track.
 *
 *  MP4GetRtpTimestampStart returns the RTP timestamp start of the specified
 *  hint track. Typically this is a random value that is chosen when the first
 *  RTP packet is constructed by the MP4 library. However the value can be set
 *  explicitly for the hint track and stored. Typically this is used if it is
 *  desired that timestamps start at zero.
 *
 *  An application will need this value in order to construct RTCP Sender
 *  Reports that relate the hint track time to an real time clock. The
 *  mp4broadcaster test program provided with the MP4 library gives an example
 *  of this.
 *
 *  See IETF RFC 1889 for details regarding RTP timestamps and RTCP.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *
 *  @return Upon success, the RTP start time in the RTP time scale which is
 *      identical to the hint track time scale. Upon an error,
 *      MP4_INVALID_TIMESTAMP.
 *
 *  @see MP4SetRtpTimestampStart()
 */
MP4V2_EXPORT
MP4Timestamp MP4GetRtpTimestampStart(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId );

/** Get the RTP start time of a hint track.
 *
 *  MP4SetRtpTimestampStart sets the RTP timestamp start of the specified hint
 *  track. Typically this is a random value that is chosen when the first RTP
 *  packet is constructed by the MP4 library. However the value can be set
 *  explicitly for the hint track and stored. Typically this is used if it is
 *  desired that timestamps start at zero.
 *
 *  See IETF RFC 1889 for details regarding RTP timestamps and RTCP.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param rtpStart specifies the desired RTP start timestamp for the hint
 *      track.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetRtpTimestampStart()
 */
MP4V2_EXPORT
bool MP4SetRtpTimestampStart(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    MP4Timestamp  rtpStart );

/** Add an RTP hint.
 *
 *  MP4AddRtpHint creates a new hint sample for the specified hint track and
 *  enables subsequent calls to MP4AddRtpPacket() to create the RTP packets
 *  associated with this hint. After all the RTP packets for the hint have been
 *  created, MP4WriteRtpHint() should be called to write the hint to the track.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4AddRtpPacket()
 *  @see MP4WriteRtpHint()
 */
MP4V2_EXPORT
bool MP4AddRtpHint(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId );

/** Add an RTP video specific hint.
 *
 *  MP4AddRtpVideoHint is an extended version of MP4AddRtpHint specifically to
 *  handle MPEG video frames. 
 *
 *  The isBFrame parameter allows the packets in the RTP hint to be marked as
 *  belonging to a video B frame. This can be useful to a streaming server if
 *  packets must be dropped due to system load or network congestion. No other
 *  video frames are dependent on the contents of B frames, so they are least
 *  damaging type of frames to drop.
 *
 *  The timestampOffset parameter allows an offset to be added to the RTP
 *  timestamp of the packets in the RTP hint. This is necessary for MPEG video
 *  that contains B frames since the video frames are transmitted out of order
 *  with respect to when they should be rendered. I.e. I and P frames are
 *  transmitted before any B frames that depend on them. The RTP timestamp must
 *  represent the rendering time of the data in the packets hence an offset
 *  must be added.
 *
 *  Note: The timestampOffset is equivalent to the sample rendering offset of a
 *  video media track. See MP4GetSampleRenderingOffset().
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param isBFrame specifies if this hint will contain packets for a video B
 *      frame.
 *  @param timestampOffset specifies a positive offset to add to the RTP
 *      timestamp for this hint. Caveat: the value is in the hint track
 *      timescale.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4AddRtpHint()
 *  @see MP4AddRtpPacket()
 *  @see MP4WriteRtpHint()
 */
MP4V2_EXPORT
bool MP4AddRtpVideoHint(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    bool          isBFrame DEFAULT(false),
    uint32_t      timestampOffset DEFAULT(0) );

/** Add an RTP packet.
 *
 *  MP4AddRtpPacket creates a new RTP packet for the currently pending RTP hint
 *  sample for the specified hint track. It also enables subsequent calls to
 *  MP4AddRtpImmediateData() and MP4AddRtpSampleData to add data to the RTP
 *  packets. After all the RTP packets for the hint have been created,
 *  MP4WriteRtpHint() should be called to write the hint to the track.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param setMBit specifies the value of the RTP packet header marker bit for
 *      this packet. The value depends on the rules of the RTP payload used for
 *      this hint track.
 *  @param transmitOffset specifies an offset to apply to the normal
 *      transmission time of this packet. The purpose of this offset is to
 *      allow smoothing of packet transmission over the duration of the hint.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4AddRtpHint()
 *  @see MP4AddRtpImmediateData()
 *  @see MP4AddRtpSampleData()
 *  @see MP4WriteRtpHint()
 */
MP4V2_EXPORT
bool MP4AddRtpPacket(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    bool          setMBit DEFAULT(false),
    int32_t       transmitOffset DEFAULT(0) );

/** Add immediate data to an RTP packet.
 *
 *  MP4AddRtpImmediateData adds immediate data to the current pending RTP
 *  packet. Typically, this is used to add RTP payload specific headers to RTP
 *  packets. Note that the size of a block of immediate data is limited to 14
 *  bytes. But multiple immediate data blocks can be added if more space is
 *  needed.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param pBytes specifies a pointer to the immediate data that should be
 *      included in the current RTP packet.
 *  @param numBytes specifies the length in bytes of the immediate data that
 *      should be included in the current RTP packet.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4AddRtpPacket()
 *  @see MP4AddRtpSampleData()
 */
MP4V2_EXPORT
bool MP4AddRtpImmediateData(
    MP4FileHandle  hFile,
    MP4TrackId     hintTrackId,
    const uint8_t* pBytes,
    uint32_t       numBytes );

/** Add media sample data to an RTP packet.
 *
 *  MP4AddRtpSampleData adds a reference in the current pending RTP packet to
 *  the media data in the specified media sample of the reference media track.
 *  Note this is a reference, not a copy, of the media data.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param sampleId specifies the reference media sample id from which the
 *      media data should be taken.
 *  @param dataOffset specifies the byte offset in the specified media sample
 *      where data should be taken from for the current RTP packet.
 *  @param dataLength specifies the length in bytes of the media data that
 *      should be included in the current RTP packet.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4AddRtpPacket()
 *  @see MP4AddRtpImmediateData()
 */
MP4V2_EXPORT
bool MP4AddRtpSampleData(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    MP4SampleId   sampleId,
    uint32_t      dataOffset,
    uint32_t      dataLength );

/** Add ES configuration information to an RTP hint.
 *
 *  MP4AddRtpESConfigurationPacket adds a packet to the current RTP hint that
 *  contains a copy of the elementary stream configuration information of the
 *  reference media track. Some RTP payloads require this information to be
 *  transmitted at the start of streaming or periodically during streaming.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4SetTrackESConfiguration()
 */
MP4V2_EXPORT
bool MP4AddRtpESConfigurationPacket(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId );

/** Write an RTP hint.
 *
 *  MP4WriteRtpHint writes the current pending hint created with
 *  MP4AddRtpHint() to the specified hint track.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param hintTrackId specifies the hint track to which the operation applies.
 *  @param duration specifies the duration for this hint sample. Typically this
 *      is the same duration as for the corresponding sample in the reference
 *      media track. Caveat: The duration should be in the hint track timescale
 *      units, again typically the same as the reference media track.
 *  @param isSyncSample specifies the sync/random access flag for this sample.
 *      Typically this is the same as for the corresponding sample in the
 *      reference media track.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4AddRtpHint()
 */
MP4V2_EXPORT
bool MP4WriteRtpHint(
    MP4FileHandle hFile,
    MP4TrackId    hintTrackId,
    MP4Duration   duration,
    bool          isSyncSample DEFAULT(true) );

/** @} ***********************************************************************/

#endif /* MP4V2_STREAMING_H */
