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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

/* rtp hint track operations */

MP4RtpHintTrack::MP4RtpHintTrack(MP4File& file, MP4Atom& trakAtom)
        : MP4Track(file, trakAtom)
{
    m_pRefTrack = NULL;

    m_pRtpMapProperty = NULL;
    m_pPayloadNumberProperty = NULL;
    m_pMaxPacketSizeProperty = NULL;
    m_pSnroProperty = NULL;
    m_pTsroProperty = NULL;

    m_pReadHint = NULL;
    m_pReadHintSample = NULL;
    m_readHintSampleSize = 0;

    m_pWriteHint = NULL;
    m_writeHintId = MP4_INVALID_SAMPLE_ID;
    m_writePacketId = 0;

    m_pTrpy = NULL;
    m_pNump = NULL;
    m_pTpyl = NULL;
    m_pMaxr = NULL;
    m_pDmed = NULL;
    m_pDimm = NULL;
    m_pPmax = NULL;
    m_pDmax = NULL;

    m_pMaxPdu = NULL;
    m_pAvgPdu = NULL;
    m_pMaxBitRate = NULL;
    m_pAvgBitRate = NULL;

    m_thisSec = 0;
    m_bytesThisSec = 0;
    m_bytesThisHint = 0;
    m_bytesThisPacket = 0;
}

MP4RtpHintTrack::~MP4RtpHintTrack()
{
    delete m_pReadHint; m_pReadHint = NULL;
    MP4Free( m_pReadHintSample ); m_pReadHintSample = NULL;
    delete m_pWriteHint; m_pWriteHint = NULL;
}

void MP4RtpHintTrack::InitRefTrack()
{
    if (m_pRefTrack == NULL) {
        MP4Integer32Property* pRefTrackIdProperty = NULL;
        (void)m_trakAtom.FindProperty(
            "trak.tref.hint.entries[0].trackId",
            (MP4Property**)&pRefTrackIdProperty);
        ASSERT(pRefTrackIdProperty);

        m_pRefTrack = m_File.GetTrack(pRefTrackIdProperty->GetValue());
    }
}

void MP4RtpHintTrack::InitRtpStart()
{
    number::srandom( time::getLocalTimeMilliseconds() );

    (void)m_trakAtom.FindProperty(
        "trak.udta.hnti.rtp .snro.offset",
        (MP4Property**)&m_pSnroProperty);

    if (m_pSnroProperty) {
        m_rtpSequenceStart = m_pSnroProperty->GetValue();
    } else {
        m_rtpSequenceStart = number::random32();
    }

    (void)m_trakAtom.FindProperty(
        "trak.udta.hnti.rtp .tsro.offset",
        (MP4Property**)&m_pTsroProperty);

    if (m_pTsroProperty) {
        m_rtpTimestampStart = m_pTsroProperty->GetValue();
    } else {
        m_rtpTimestampStart = number::random32();
    }
}

void MP4RtpHintTrack::ReadHint(
    MP4SampleId hintSampleId,
    uint16_t* pNumPackets)
{
    if (m_pRefTrack == NULL) {
        InitRefTrack();
        InitRtpStart();
    }

    // dispose of any old hint
    delete m_pReadHint; m_pReadHint = NULL;
    MP4Free( m_pReadHintSample ); m_pReadHintSample = NULL;
    m_readHintSampleSize = 0;

    // read the desired hint sample into memory
    ReadSample(
        hintSampleId,
        &m_pReadHintSample,
        &m_readHintSampleSize,
        &m_readHintTimestamp);

    m_File.EnableMemoryBuffer(m_pReadHintSample, m_readHintSampleSize);

    m_pReadHint = new MP4RtpHint(*this);
    m_pReadHint->Read(m_File);

    m_File.DisableMemoryBuffer();

    if (pNumPackets) {
        *pNumPackets = GetHintNumberOfPackets();
    }
}

uint16_t MP4RtpHintTrack::GetHintNumberOfPackets()
{
    if (m_pReadHint == NULL) {
        throw new Exception("no hint has been read",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    return m_pReadHint->GetNumberOfPackets();
}

bool MP4RtpHintTrack::GetPacketBFrame(uint16_t packetIndex)
{
    if (m_pReadHint == NULL) {
        throw new Exception("no hint has been read",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    MP4RtpPacket* pPacket =
        m_pReadHint->GetPacket(packetIndex);

    return pPacket->IsBFrame();
}

uint16_t MP4RtpHintTrack::GetPacketTransmitOffset(uint16_t packetIndex)
{
    if (m_pReadHint == NULL) {
        throw new Exception("no hint has been read",
                            __FILE__, __LINE__, __FUNCTION__);
     }

    MP4RtpPacket* pPacket =
        m_pReadHint->GetPacket(packetIndex);

    return pPacket->GetTransmitOffset();
}

void MP4RtpHintTrack::ReadPacket(
    uint16_t packetIndex,
    uint8_t** ppBytes,
    uint32_t* pNumBytes,
    uint32_t ssrc,
    bool addHeader,
    bool addPayload)
{
    if (m_pReadHint == NULL) {
        throw new Exception("no hint has been read",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    if (!addHeader && !addPayload) {
        throw new Exception("no data requested",
                             __FILE__, __LINE__, __FUNCTION__);
    }

    MP4RtpPacket* pPacket =
        m_pReadHint->GetPacket(packetIndex);

    *pNumBytes = 0;
    if (addHeader) {
        *pNumBytes += 12;
    }
    if (addPayload) {
        *pNumBytes += pPacket->GetDataSize();
    }

    // if needed, allocate the packet memory
    bool buffer_malloc = false;

    if (*ppBytes == NULL) {
        *ppBytes = (uint8_t*)MP4Malloc(*pNumBytes);
        buffer_malloc = true;
    }

    try {
        uint8_t* pDest = *ppBytes;

        if (addHeader) {
            *pDest++ =
                0x80 | (pPacket->GetPBit() << 5) | (pPacket->GetXBit() << 4);

            *pDest++ =
                (pPacket->GetMBit() << 7) | pPacket->GetPayload();

            *((uint16_t*)pDest) =
                MP4V2_HTONS(m_rtpSequenceStart + pPacket->GetSequenceNumber());
            pDest += 2;

            *((uint32_t*)pDest) =
                MP4V2_HTONL(m_rtpTimestampStart + (uint32_t)m_readHintTimestamp);
            pDest += 4;

            *((uint32_t*)pDest) =
                MP4V2_HTONL(ssrc);
            pDest += 4;
        }

        if (addPayload) {
            pPacket->GetData(pDest);
        }
    }
    catch (Exception* x) {
        if (buffer_malloc) {
            MP4Free(*ppBytes);
            *ppBytes = NULL;
        }
        throw x;
    }

    log.hexDump(0, MP4_LOG_VERBOSE1, *ppBytes, *pNumBytes,
                "\"%s\": %u ", GetFile().GetFilename().c_str(),
                packetIndex);
}

MP4Timestamp MP4RtpHintTrack::GetRtpTimestampStart()
{
    if (m_pRefTrack == NULL) {
        InitRefTrack();
        InitRtpStart();
    }

    return m_rtpTimestampStart;
}

void MP4RtpHintTrack::SetRtpTimestampStart(MP4Timestamp start)
{
    if (!m_pTsroProperty) {
        MP4Atom* pTsroAtom =
            m_File.AddDescendantAtoms(&m_trakAtom, "udta.hnti.rtp .tsro");

        ASSERT(pTsroAtom);

        (void)pTsroAtom->FindProperty("offset",
                                      (MP4Property**)&m_pTsroProperty);

        ASSERT(m_pTsroProperty);
    }

    m_pTsroProperty->SetValue(start);
    m_rtpTimestampStart = start;
}

void MP4RtpHintTrack::InitPayload()
{
    if (m_pRtpMapProperty == NULL) {
        (void)m_trakAtom.FindProperty(
            "trak.udta.hinf.payt.rtpMap",
            (MP4Property**)&m_pRtpMapProperty);
    }

    if (m_pPayloadNumberProperty == NULL) {
        (void)m_trakAtom.FindProperty(
            "trak.udta.hinf.payt.payloadNumber",
            (MP4Property**)&m_pPayloadNumberProperty);
    }

    if (m_pMaxPacketSizeProperty == NULL) {
        (void)m_trakAtom.FindProperty(
            "trak.mdia.minf.stbl.stsd.rtp .maxPacketSize",
            (MP4Property**)&m_pMaxPacketSizeProperty);
    }
}

void MP4RtpHintTrack::GetPayload(
    char** ppPayloadName,
    uint8_t* pPayloadNumber,
    uint16_t* pMaxPayloadSize,
    char **ppEncodingParams)
{
    const char* pRtpMap;
    const char* pSlash;
    uint32_t length;
    InitPayload();

    if (ppPayloadName || ppEncodingParams) {
        if (ppPayloadName)
            *ppPayloadName = NULL;
        if (ppEncodingParams)
            *ppEncodingParams = NULL;
        if (m_pRtpMapProperty) {
            pRtpMap = m_pRtpMapProperty->GetValue();
            pSlash = strchr(pRtpMap, '/');

            if (pSlash) {
                length = pSlash - pRtpMap;
            } else {
                length = (uint32_t)strlen(pRtpMap);
            }

            if (ppPayloadName) {
                *ppPayloadName = (char*)MP4Calloc(length + 1);
                strncpy(*ppPayloadName, pRtpMap, length);
            }
            if (pSlash && ppEncodingParams) {
                pSlash++;
                pSlash = strchr(pSlash, '/');
                if (pSlash != NULL) {
                    pSlash++;
                    if (pSlash != '\0') {
                        length = (uint32_t)strlen(pRtpMap) - (pSlash - pRtpMap);
                        *ppEncodingParams = (char *)MP4Calloc(length + 1);
                        strncpy(*ppEncodingParams, pSlash, length);
                    }
                }
            }
        }
    }

    if (pPayloadNumber) {
        if (m_pPayloadNumberProperty) {
            *pPayloadNumber = m_pPayloadNumberProperty->GetValue();
        } else {
            *pPayloadNumber = 0;
        }
    }

    if (pMaxPayloadSize) {
        if (m_pMaxPacketSizeProperty) {
            *pMaxPayloadSize = m_pMaxPacketSizeProperty->GetValue();
        } else {
            *pMaxPayloadSize = 0;
        }
    }
}

void MP4RtpHintTrack::SetPayload(
    const char* payloadName,
    uint8_t payloadNumber,
    uint16_t maxPayloadSize,
    const char *encoding_parms,
    bool include_rtp_map,
    bool include_mpeg4_esid)
{
    InitRefTrack();
    InitPayload();

    ASSERT(m_pRtpMapProperty);
    ASSERT(m_pPayloadNumberProperty);
    ASSERT(m_pMaxPacketSizeProperty);

    size_t len = strlen(payloadName) + 16;
    if (encoding_parms != NULL) {
        size_t temp = strlen(encoding_parms);
        if (temp == 0) {
            encoding_parms = NULL;
        } else {
            len += temp;
        }
    }

    char* rtpMapBuf = (char*)MP4Malloc(len);
    snprintf(rtpMapBuf, len, "%s/%u%c%s",
             payloadName,
             GetTimeScale(),
             encoding_parms != NULL ? '/' : '\0',
             encoding_parms == NULL ? "" : encoding_parms);
    m_pRtpMapProperty->SetValue(rtpMapBuf);

    m_pPayloadNumberProperty->SetValue(payloadNumber);

    if (maxPayloadSize == 0) {
        maxPayloadSize = 1460;
    }
    m_pMaxPacketSizeProperty->SetValue(maxPayloadSize);

    // set sdp media type
    const char* sdpMediaType;
    if (!strcmp(m_pRefTrack->GetType(), MP4_AUDIO_TRACK_TYPE)) {
        sdpMediaType = "audio";
    } else if (!strcmp(m_pRefTrack->GetType(), MP4_VIDEO_TRACK_TYPE)) {
        sdpMediaType = "video";
    } else if (!strcmp(m_pRefTrack->GetType(), MP4_CNTL_TRACK_TYPE)) {
        sdpMediaType = "control";
    } else {
        sdpMediaType = "application";
    }

    uint32_t maxlen =
        (uint32_t)strlen(sdpMediaType) + (uint32_t)strlen(rtpMapBuf) + 256;
    char* sdpBuf = (char*)MP4Malloc(maxlen);
    uint32_t buflen;
    buflen = snprintf(sdpBuf, maxlen,
                      "m=%s 0 RTP/AVP %u\015\012"
                      "a=control:trackID=%u\015\012",
                      sdpMediaType, payloadNumber,
                      m_trackId);
    if (include_rtp_map) {
        buflen += snprintf(sdpBuf + buflen, maxlen - buflen,
                           "a=rtpmap:%u %s\015\012",
                           payloadNumber, rtpMapBuf);
    }
    if (include_mpeg4_esid) {
        snprintf(sdpBuf + buflen, maxlen - buflen,
                 "a=mpeg4-esid:%u\015\012",
                 m_pRefTrack->GetId());
    }

    MP4StringProperty* pSdpProperty = NULL;
    (void)m_trakAtom.FindProperty("trak.udta.hnti.sdp .sdpText",
                                    (MP4Property**)&pSdpProperty);
    ASSERT(pSdpProperty);
    pSdpProperty->SetValue(sdpBuf);

    // cleanup
    MP4Free(rtpMapBuf);
    MP4Free(sdpBuf);
}

void MP4RtpHintTrack::AddHint(bool isBFrame, uint32_t timestampOffset)
{
    // on first hint, need to lookup the reference track
    if (m_writeHintId == MP4_INVALID_SAMPLE_ID) {
        InitRefTrack();
        InitStats();
    }

    if (m_pWriteHint) {
        throw new Exception("unwritten hint is still pending", __FILE__, __LINE__, __FUNCTION__);
    }

    m_pWriteHint = new MP4RtpHint(*this);
    m_pWriteHint->SetBFrame(isBFrame);
    m_pWriteHint->SetTimestampOffset(timestampOffset);

    m_bytesThisHint = 0;
    m_writeHintId++;
}

void MP4RtpHintTrack::AddPacket(bool setMbit, int32_t transmitOffset)
{
    if (m_pWriteHint == NULL) {
        throw new Exception("no hint pending", __FILE__, __LINE__, __FUNCTION__);
    }

    MP4RtpPacket* pPacket = m_pWriteHint->AddPacket();

    ASSERT(m_pPayloadNumberProperty);

    pPacket->Set(
        m_pPayloadNumberProperty->GetValue(),
        m_writePacketId++,
        setMbit);
    pPacket->SetTransmitOffset(transmitOffset);

    m_bytesThisHint += 12;
    if (m_bytesThisPacket > m_pPmax->GetValue()) {
        m_pPmax->SetValue(m_bytesThisPacket);
    }
    m_bytesThisPacket = 12;
    m_pNump->IncrementValue();
    m_pTrpy->IncrementValue(12); // RTP packet header size
}

void MP4RtpHintTrack::AddImmediateData(
    const uint8_t* pBytes,
    uint32_t numBytes)
{
    if (m_pWriteHint == NULL) {
        throw new Exception("no hint pending", __FILE__, __LINE__, __FUNCTION__ );
    }

    MP4RtpPacket* pPacket = m_pWriteHint->GetCurrentPacket();
    if (pPacket == NULL) {
        throw new Exception("no packet pending", __FILE__, __LINE__, __FUNCTION__);
    }

    if (pBytes == NULL || numBytes == 0) {
        throw new Exception("no data",
                            __FILE__, __LINE__, __FUNCTION__ );
    }
    if (numBytes > 14) {
        throw new Exception("data size is larger than 14 bytes",
                            __FILE__, __LINE__, __FUNCTION__ );
    }

    MP4RtpImmediateData* pData = new MP4RtpImmediateData(*pPacket);
    pData->Set(pBytes, numBytes);

    pPacket->AddData(pData);

    m_bytesThisHint += numBytes;
    m_bytesThisPacket += numBytes;
    m_pDimm->IncrementValue(numBytes);
    m_pTpyl->IncrementValue(numBytes);
    m_pTrpy->IncrementValue(numBytes);
}

void MP4RtpHintTrack::AddSampleData(
    MP4SampleId sampleId,
    uint32_t dataOffset,
    uint32_t dataLength)
{
    if (m_pWriteHint == NULL) {
        throw new Exception("no hint pending", __FILE__, __LINE__, __FUNCTION__ );
    }

    MP4RtpPacket* pPacket = m_pWriteHint->GetCurrentPacket();
    if (pPacket == NULL) {
        throw new Exception("no packet pending", __FILE__, __LINE__, __FUNCTION__ );
    }

    MP4RtpSampleData* pData = new MP4RtpSampleData(*pPacket);

    pData->SetReferenceSample(sampleId, dataOffset, dataLength);

    pPacket->AddData(pData);

    m_bytesThisHint += dataLength;
    m_bytesThisPacket += dataLength;
    m_pDmed->IncrementValue(dataLength);
    m_pTpyl->IncrementValue(dataLength);
    m_pTrpy->IncrementValue(dataLength);
}

void MP4RtpHintTrack::AddESConfigurationPacket()
{
    if (m_pWriteHint == NULL) {
        throw new Exception("no hint pending", __FILE__, __LINE__, __FUNCTION__ );
    }

    uint8_t* pConfig = NULL;
    uint32_t configSize = 0;

    m_File.GetTrackESConfiguration(m_pRefTrack->GetId(),
                                   &pConfig, &configSize);

    if (pConfig == NULL) {
        return;
    }

    ASSERT(m_pMaxPacketSizeProperty);

    if (configSize > m_pMaxPacketSizeProperty->GetValue()) {
        throw new Exception("ES configuration is too large for RTP payload", __FILE__, __LINE__, __FUNCTION__ );
    }

    AddPacket(false);

    MP4RtpPacket* pPacket = m_pWriteHint->GetCurrentPacket();
    ASSERT(pPacket);

    // This is ugly!
    // To get the ES configuration data somewhere known
    // we create a sample data reference that points to
    // this hint track (not the media track)
    // and this sample of the hint track
    // the offset into this sample is filled in during the write process
    MP4RtpSampleData* pData = new MP4RtpSampleData(*pPacket);

    pData->SetEmbeddedImmediate(m_writeSampleId, pConfig, configSize);

    pPacket->AddData(pData);

    m_bytesThisHint += configSize;
    m_bytesThisPacket += configSize;
    m_pTpyl->IncrementValue(configSize);
    m_pTrpy->IncrementValue(configSize);
}

void MP4RtpHintTrack::WriteHint(MP4Duration duration, bool isSyncSample)
{
    if (m_pWriteHint == NULL) {
        throw new Exception("no hint pending", __FILE__, __LINE__, __FUNCTION__ );
    }

    uint8_t* pBytes;
    uint64_t numBytes;

    m_File.EnableMemoryBuffer();

    m_pWriteHint->Write(m_File);

    m_File.DisableMemoryBuffer(&pBytes, &numBytes);

    WriteSample(pBytes, numBytes, duration, 0, isSyncSample);

    MP4Free(pBytes);

    // update statistics
    if (m_bytesThisPacket > m_pPmax->GetValue()) {
        m_pPmax->SetValue(m_bytesThisPacket);
    }

    if (duration > m_pDmax->GetValue()) {
        m_pDmax->SetValue(duration);
    }

    MP4Timestamp startTime;

    GetSampleTimes(m_writeHintId, &startTime, NULL);

    if (startTime < m_thisSec + GetTimeScale()) {
        m_bytesThisSec += m_bytesThisHint;
    } else {
        if (m_bytesThisSec > m_pMaxr->GetValue()) {
            m_pMaxr->SetValue(m_bytesThisSec);
        }
        m_thisSec = startTime - (startTime % GetTimeScale());
        m_bytesThisSec = m_bytesThisHint;
    }

    // cleanup
    delete m_pWriteHint;
    m_pWriteHint = NULL;
}

void MP4RtpHintTrack::FinishWrite(uint32_t option)
{
    if (m_writeHintId != MP4_INVALID_SAMPLE_ID) {
        m_pMaxPdu->SetValue(m_pPmax->GetValue());
        if (m_pNump->GetValue()) {
            m_pAvgPdu->SetValue(m_pTrpy->GetValue() / m_pNump->GetValue());
        }

        m_pMaxBitRate->SetValue(m_pMaxr->GetValue() * 8);
        if (GetDuration()) {
            m_pAvgBitRate->SetValue(
                m_pTrpy->GetValue() * 8 * GetTimeScale() / GetDuration());
        }
    }

    MP4Track::FinishWrite();
}

void MP4RtpHintTrack::InitStats()
{
    MP4Atom* pHinfAtom = m_trakAtom.FindAtom("trak.udta.hinf");

    ASSERT(pHinfAtom);

    (void)pHinfAtom->FindProperty("hinf.trpy.bytes", (MP4Property**)&m_pTrpy);
    (void)pHinfAtom->FindProperty("hinf.nump.packets", (MP4Property**)&m_pNump);
    (void)pHinfAtom->FindProperty("hinf.tpyl.bytes", (MP4Property**)&m_pTpyl);
    (void)pHinfAtom->FindProperty("hinf.maxr.bytes", (MP4Property**)&m_pMaxr);
    (void)pHinfAtom->FindProperty("hinf.dmed.bytes", (MP4Property**)&m_pDmed);
    (void)pHinfAtom->FindProperty("hinf.dimm.bytes", (MP4Property**)&m_pDimm);
    (void)pHinfAtom->FindProperty("hinf.pmax.bytes", (MP4Property**)&m_pPmax);
    (void)pHinfAtom->FindProperty("hinf.dmax.milliSecs", (MP4Property**)&m_pDmax);

    MP4Atom* pHmhdAtom = m_trakAtom.FindAtom("trak.mdia.minf.hmhd");

    ASSERT(pHmhdAtom);

    (void)pHmhdAtom->FindProperty("hmhd.maxPduSize", (MP4Property**)&m_pMaxPdu);
    (void)pHmhdAtom->FindProperty("hmhd.avgPduSize", (MP4Property**)&m_pAvgPdu);
    (void)pHmhdAtom->FindProperty("hmhd.maxBitRate", (MP4Property**)&m_pMaxBitRate);
    (void)pHmhdAtom->FindProperty("hmhd.avgBitRate", (MP4Property**)&m_pAvgBitRate);

    MP4Integer32Property* pMaxrPeriod = NULL;
    (void)pHinfAtom->FindProperty("hinf.maxr.granularity",
                                  (MP4Property**)&pMaxrPeriod);
    if (pMaxrPeriod) {
        pMaxrPeriod->SetValue(1000);    // 1 second
    }
}


MP4RtpHint::MP4RtpHint(MP4RtpHintTrack& track)
    : m_track(track)
{
    AddProperty( /* 0 */
        new MP4Integer16Property(this->GetTrack().GetTrakAtom(), "packetCount"));
    AddProperty( /* 1 */
        new MP4Integer16Property(this->GetTrack().GetTrakAtom(), "reserved"));
}

MP4RtpHint::~MP4RtpHint()
{
    for (uint32_t i = 0; i < m_rtpPackets.Size(); i++) {
        delete m_rtpPackets[i];
    }
}

MP4RtpPacket* MP4RtpHint::AddPacket()
{
    MP4RtpPacket* pPacket = new MP4RtpPacket(*this);
    m_rtpPackets.Add(pPacket);

    // packetCount property
    ((MP4Integer16Property*)m_pProperties[0])->IncrementValue();

    pPacket->SetBFrame(m_isBFrame);
    pPacket->SetTimestampOffset(m_timestampOffset);

    return pPacket;
}

void MP4RtpHint::Read(MP4File& file)
{
    // call base class Read for required properties
    MP4Container::Read(file);

    uint16_t numPackets =
        ((MP4Integer16Property*)m_pProperties[0])->GetValue();

    for (uint16_t i = 0; i < numPackets; i++) {
        MP4RtpPacket* pPacket = new MP4RtpPacket(*this);

        m_rtpPackets.Add(pPacket);

        pPacket->Read(file);
    }

    if (log.verbosity >= MP4_LOG_VERBOSE1) {
        log.verbose1f("\"%s\": ReadHint:", GetTrack().GetFile().GetFilename().c_str());
        Dump(10, false);
    }
}

void MP4RtpHint::Write(MP4File& file)
{
    uint64_t hintStartPos = file.GetPosition();

    MP4Container::Write(file);

    uint64_t packetStartPos = file.GetPosition();

    uint32_t i;

    // first write out packet (and data) entries
    for (i = 0; i < m_rtpPackets.Size(); i++) {
        m_rtpPackets[i]->Write(file);
    }

    // now let packets write their extra data into the hint sample
    for (i = 0; i < m_rtpPackets.Size(); i++) {
        m_rtpPackets[i]->WriteEmbeddedData(file, hintStartPos);
    }

    uint64_t endPos = file.GetPosition();

    file.SetPosition(packetStartPos);

    // finally rewrite the packet and data entries
    // which now contain the correct offsets for the embedded data
    for (i = 0; i < m_rtpPackets.Size(); i++) {
        m_rtpPackets[i]->Write(file);
    }

    file.SetPosition(endPos);

    if (log.verbosity >= MP4_LOG_VERBOSE1) {
        log.verbose1f("\"%s\": WriteRtpHint:", GetTrack().GetFile().GetFilename().c_str());
        Dump(14, false);
    }
}

void MP4RtpHint::Dump(uint8_t indent, bool dumpImplicits)
{
    MP4Container::Dump(indent, dumpImplicits);

    for (uint32_t i = 0; i < m_rtpPackets.Size(); i++) {
        log.dump(indent, MP4_LOG_VERBOSE1,"\"%s\": RtpPacket: %u",
                 GetTrack().GetFile().GetFilename().c_str(), i);
        m_rtpPackets[i]->Dump(indent + 1, dumpImplicits);
    }
}

MP4RtpPacket::MP4RtpPacket(MP4RtpHint& hint)
    : m_hint(hint)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(this->GetHint().GetTrack().GetTrakAtom(), "relativeXmitTime"));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "reserved1", 2));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "Pbit", 1));
    AddProperty( /* 3 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "Xbit", 1));
    AddProperty( /* 4 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "reserved2", 4));
    AddProperty( /* 5 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "Mbit", 1));
    AddProperty( /* 6 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "payloadType", 7));
    AddProperty( /* 7  */
        new MP4Integer16Property(this->GetHint().GetTrack().GetTrakAtom(), "sequenceNumber"));
    AddProperty( /* 8 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "reserved3", 13));
    AddProperty( /* 9 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "extraFlag", 1));
    AddProperty( /* 10 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "bFrameFlag", 1));
    AddProperty( /* 11 */
        new MP4BitfieldProperty(this->GetHint().GetTrack().GetTrakAtom(), "repeatFlag", 1));
    AddProperty( /* 12 */
        new MP4Integer16Property(this->GetHint().GetTrack().GetTrakAtom(), "entryCount"));
}

MP4RtpPacket::~MP4RtpPacket()
{
    for (uint32_t i = 0; i < m_rtpData.Size(); i++) {
        delete m_rtpData[i];
    }
}

void MP4RtpPacket::AddExtraProperties()
{
    AddProperty( /* 13 */
        new MP4Integer32Property(this->GetHint().GetTrack().GetTrakAtom(), "extraInformationLength"));

    // This is a bit of a hack, since the tlv entries are really defined
    // as atoms but there is only one type defined now, rtpo, and getting
    // our atom code hooked up here would be a major pain with little gain

    AddProperty( /* 14 */
        new MP4Integer32Property(this->GetHint().GetTrack().GetTrakAtom(), "tlvLength"));
    AddProperty( /* 15 */
        new MP4StringProperty(this->GetHint().GetTrack().GetTrakAtom(), "tlvType"));
    AddProperty( /* 16 */
        new MP4Integer32Property(this->GetHint().GetTrack().GetTrakAtom(), "timestampOffset"));

    ((MP4Integer32Property*)m_pProperties[13])->SetValue(16);
    ((MP4Integer32Property*)m_pProperties[14])->SetValue(12);
    ((MP4StringProperty*)m_pProperties[15])->SetFixedLength(4);
    ((MP4StringProperty*)m_pProperties[15])->SetValue("rtpo");
}

void MP4RtpPacket::Read(MP4File& file)
{
    // call base class Read for required properties
    MP4Container::Read(file);

    // read extra info if present
    // we only support the rtpo field!
    if (((MP4BitfieldProperty*)m_pProperties[9])->GetValue() == 1) {
        ReadExtra(file);
    }

    uint16_t numDataEntries =
        ((MP4Integer16Property*)m_pProperties[12])->GetValue();

    // read data entries
    for (uint16_t i = 0; i < numDataEntries; i++) {
        uint8_t dataType;
        file.PeekBytes(&dataType, 1);

        MP4RtpData* pData;

        switch (dataType) {
        case 0:
            pData = new MP4RtpNullData(*this);
            break;
        case 1:
            pData = new MP4RtpImmediateData(*this);
            break;
        case 2:
            pData = new MP4RtpSampleData(*this);
            break;
        case 3:
            pData = new MP4RtpSampleDescriptionData(*this);
            break;
        default:
            throw new Exception("unknown packet data entry type", __FILE__, __LINE__, __FUNCTION__ );
        }

        m_rtpData.Add(pData);

        // read data entry's properties
        pData->Read(file);
    }
}

void MP4RtpPacket::ReadExtra(MP4File& file)
{
    AddExtraProperties();

    int32_t extraLength = (int32_t)file.ReadUInt32();

    if (extraLength < 4) {
        throw new Exception("bad packet extra info length", __FILE__, __LINE__, __FUNCTION__ );
    }
    extraLength -= 4;

    while (extraLength > 0) {
        uint32_t entryLength = file.ReadUInt32();
        uint32_t entryTag = file.ReadUInt32();

        if (entryLength < 8) {
            throw new Exception("bad packet extra info entry length", __FILE__, __LINE__, __FUNCTION__ );
        }

        if (entryTag == STRTOINT32("rtpo") && entryLength == 12) {
            // read the rtp timestamp offset
            m_pProperties[16]->Read(file);
        } else {
            // ignore it, LATER carry it along
            file.SetPosition(file.GetPosition() + entryLength - 8);
        }

        extraLength -= entryLength;
    }

    if (extraLength < 0) {
        throw new Exception("invalid packet extra info length", __FILE__, __LINE__, __FUNCTION__ );
    }
}

void MP4RtpPacket::Set(uint8_t payloadNumber,
                       uint32_t packetId, bool setMbit)
{
    ((MP4BitfieldProperty*)m_pProperties[5])->SetValue(setMbit);
    ((MP4BitfieldProperty*)m_pProperties[6])->SetValue(payloadNumber);
    ((MP4Integer16Property*)m_pProperties[7])->SetValue(packetId);
}

int32_t MP4RtpPacket::GetTransmitOffset()
{
    return ((MP4Integer32Property*)m_pProperties[0])->GetValue();
}

void MP4RtpPacket::SetTransmitOffset(int32_t transmitOffset)
{
    ((MP4Integer32Property*)m_pProperties[0])->SetValue(transmitOffset);
}

bool MP4RtpPacket::GetPBit()
{
    return ((MP4BitfieldProperty*)m_pProperties[2])->GetValue();
}

bool MP4RtpPacket::GetXBit()
{
    return ((MP4BitfieldProperty*)m_pProperties[3])->GetValue();
}

bool MP4RtpPacket::GetMBit()
{
    return ((MP4BitfieldProperty*)m_pProperties[5])->GetValue();
}

uint8_t MP4RtpPacket::GetPayload()
{
    return ((MP4BitfieldProperty*)m_pProperties[6])->GetValue();
}

uint16_t MP4RtpPacket::GetSequenceNumber()
{
    return ((MP4Integer16Property*)m_pProperties[7])->GetValue();
}

bool MP4RtpPacket::IsBFrame()
{
    return ((MP4BitfieldProperty*)m_pProperties[10])->GetValue();
}

void MP4RtpPacket::SetBFrame(bool isBFrame)
{
    ((MP4BitfieldProperty*)m_pProperties[10])->SetValue(isBFrame);
}

void MP4RtpPacket::SetTimestampOffset(uint32_t timestampOffset)
{
    if (timestampOffset == 0) {
        return;
    }

    ASSERT(((MP4BitfieldProperty*)m_pProperties[9])->GetValue() == 0);

    // set X bit
    ((MP4BitfieldProperty*)m_pProperties[9])->SetValue(1);

    AddExtraProperties();

    ((MP4Integer32Property*)m_pProperties[16])->SetValue(timestampOffset);
}

void MP4RtpPacket::AddData(MP4RtpData* pData)
{
    m_rtpData.Add(pData);

    // increment entry count property
    ((MP4Integer16Property*)m_pProperties[12])->IncrementValue();
}

uint32_t MP4RtpPacket::GetDataSize()
{
    uint32_t totalDataSize = 0;

    for (uint32_t i = 0; i < m_rtpData.Size(); i++) {
        totalDataSize += m_rtpData[i]->GetDataSize();
    }

    return totalDataSize;
}

void MP4RtpPacket::GetData(uint8_t* pDest)
{
    for (uint32_t i = 0; i < m_rtpData.Size(); i++) {
        m_rtpData[i]->GetData(pDest);
        pDest += m_rtpData[i]->GetDataSize();
    }
}

void MP4RtpPacket::Write(MP4File& file)
{
    MP4Container::Write(file);

    for (uint32_t i = 0; i < m_rtpData.Size(); i++) {
        m_rtpData[i]->Write(file);
    }
}

void MP4RtpPacket::WriteEmbeddedData(MP4File& file, uint64_t startPos)
{
    for (uint32_t i = 0; i < m_rtpData.Size(); i++) {
        m_rtpData[i]->WriteEmbeddedData(file, startPos);
    }
}

void MP4RtpPacket::Dump(uint8_t indent, bool dumpImplicits)
{
    MP4Container::Dump(indent, dumpImplicits);

    for (uint32_t i = 0; i < m_rtpData.Size(); i++) {
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": RtpData: %u",
                 GetHint().GetTrack().GetFile().GetFilename().c_str(), i);
        m_rtpData[i]->Dump(indent + 1, dumpImplicits);
    }
}

MP4RtpData::MP4RtpData(MP4RtpPacket& packet)
    : m_packet(packet)
{
    AddProperty( /* 0 */
        new MP4Integer8Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "type"));
}

MP4Track* MP4RtpData::FindTrackFromRefIndex(uint8_t refIndex)
{
    MP4Track* pTrack;

    if (refIndex == (uint8_t)-1) {
        // ourselves
        pTrack = &GetPacket().GetHint().GetTrack();
    } else if (refIndex == 0) {
        // our reference track
        pTrack = GetPacket().GetHint().GetTrack().GetRefTrack();
    } else {
        // some other track
        MP4RtpHintTrack* pHintTrack =
            &GetPacket().GetHint().GetTrack();

        MP4Atom& trakAtom = pHintTrack->GetTrakAtom();

        MP4Integer32Property* pTrackIdProperty = NULL;
        (void)trakAtom.FindProperty(
            "trak.tref.hint.entries",
            (MP4Property**)&pTrackIdProperty);
        ASSERT(pTrackIdProperty);

        uint32_t refTrackId =
            pTrackIdProperty->GetValue(refIndex - 1);

        pTrack = pHintTrack->GetFile().GetTrack(refTrackId);
    }

    return pTrack;
}

MP4RtpNullData::MP4RtpNullData(MP4RtpPacket& packet)
        : MP4RtpData(packet)
{
    ((MP4Integer8Property*)m_pProperties[0])->SetValue(0);

    AddProperty( /* 1 */
        new MP4BytesProperty(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "pad", 15));

    ((MP4BytesProperty*)m_pProperties[1])->SetFixedSize(15);
}

MP4RtpImmediateData::MP4RtpImmediateData(MP4RtpPacket& packet)
        : MP4RtpData(packet)
{
    ((MP4Integer8Property*)m_pProperties[0])->SetValue(1);

    AddProperty( /* 1 */
        new MP4Integer8Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "count"));
    AddProperty( /* 2 */
        new MP4BytesProperty(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "data", 14));

    ((MP4BytesProperty*)m_pProperties[2])->SetFixedSize(14);
}

void MP4RtpImmediateData::Set(const uint8_t* pBytes, uint8_t numBytes)
{
    ((MP4Integer8Property*)m_pProperties[1])->SetValue(numBytes);
    ((MP4BytesProperty*)m_pProperties[2])->SetValue(pBytes, numBytes);
}

uint16_t MP4RtpImmediateData::GetDataSize()
{
    return ((MP4Integer8Property*)m_pProperties[1])->GetValue();
}

void MP4RtpImmediateData::GetData(uint8_t* pDest)
{
    uint8_t* pValue;
    uint32_t valueSize;
    ((MP4BytesProperty*)m_pProperties[2])->GetValue(&pValue, &valueSize);

    memcpy(pDest, pValue, GetDataSize());
    MP4Free(pValue);
}

MP4RtpSampleData::MP4RtpSampleData(MP4RtpPacket& packet)
        : MP4RtpData(packet)
{
    ((MP4Integer8Property*)m_pProperties[0])->SetValue(2);

    AddProperty( /* 1 */
        new MP4Integer8Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "trackRefIndex"));
    AddProperty( /* 2 */
        new MP4Integer16Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "length"));
    AddProperty( /* 3 */
        new MP4Integer32Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "sampleNumber"));
    AddProperty( /* 4 */
        new MP4Integer32Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "sampleOffset"));
    AddProperty( /* 5 */
        new MP4Integer16Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "bytesPerBlock"));
    AddProperty( /* 6 */
        new MP4Integer16Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "samplesPerBlock"));

    ((MP4Integer16Property*)m_pProperties[5])->SetValue(1);
    ((MP4Integer16Property*)m_pProperties[6])->SetValue(1);

    m_pRefData = NULL;
    m_pRefTrack = NULL;
    m_refSampleId = MP4_INVALID_SAMPLE_ID;
    m_refSampleOffset = 0;
}

void MP4RtpSampleData::SetEmbeddedImmediate(MP4SampleId sampleId,
        uint8_t* pData, uint16_t dataLength)
{
    ((MP4Integer8Property*)m_pProperties[1])->SetValue((uint8_t)-1);
    ((MP4Integer16Property*)m_pProperties[2])->SetValue(dataLength);
    ((MP4Integer32Property*)m_pProperties[3])->SetValue(sampleId);
    ((MP4Integer32Property*)m_pProperties[4])->SetValue(0);
    CHECK_AND_FREE(m_pRefData);
    m_pRefData = pData;
}

void MP4RtpSampleData::SetReferenceSample(
    MP4SampleId refSampleId, uint32_t refSampleOffset,
    uint16_t sampleLength)
{
    ((MP4Integer8Property*)m_pProperties[1])->SetValue(0);
    ((MP4Integer16Property*)m_pProperties[2])->SetValue(sampleLength);
    ((MP4Integer32Property*)m_pProperties[3])->SetValue(refSampleId);
    ((MP4Integer32Property*)m_pProperties[4])->SetValue(refSampleOffset);
}

void MP4RtpSampleData::SetEmbeddedSample(
    MP4SampleId sampleId, MP4Track* pRefTrack,
    MP4SampleId refSampleId, uint32_t refSampleOffset,
    uint16_t sampleLength)
{
    ((MP4Integer8Property*)m_pProperties[1])->SetValue((uint8_t)-1);
    ((MP4Integer16Property*)m_pProperties[2])->SetValue(sampleLength);
    ((MP4Integer32Property*)m_pProperties[3])->SetValue(sampleId);
    ((MP4Integer32Property*)m_pProperties[4])->SetValue(0);
    m_pRefTrack = pRefTrack;
    m_refSampleId = refSampleId;
    m_refSampleOffset = refSampleOffset;
}

uint16_t MP4RtpSampleData::GetDataSize()
{
    return ((MP4Integer16Property*)m_pProperties[2])->GetValue();
}

void MP4RtpSampleData::GetData(uint8_t* pDest)
{
    uint8_t trackRefIndex =
        ((MP4Integer8Property*)m_pProperties[1])->GetValue();

    MP4Track* pSampleTrack =
        FindTrackFromRefIndex(trackRefIndex);

    pSampleTrack->ReadSampleFragment(
        ((MP4Integer32Property*)m_pProperties[3])->GetValue(),  // sampleId
        ((MP4Integer32Property*)m_pProperties[4])->GetValue(),  // sampleOffset
        ((MP4Integer16Property*)m_pProperties[2])->GetValue(),  // sampleLength
        pDest);
}

void MP4RtpSampleData::WriteEmbeddedData(MP4File& file, uint64_t startPos)
{
    // if not using embedded data, nothing to do
    if (((MP4Integer8Property*)m_pProperties[1])->GetValue() != (uint8_t)-1) {
        return;
    }

    // figure out the offset within this hint sample for this embedded data
    uint64_t offset = file.GetPosition() - startPos;
    ASSERT(offset <= 0xFFFFFFFF);
    ((MP4Integer32Property*)m_pProperties[4])->SetValue((uint32_t)offset);

    uint16_t length = ((MP4Integer16Property*)m_pProperties[2])->GetValue();

    if (m_pRefData) {
        file.WriteBytes(m_pRefData, length);
        return;
    }

    if (m_refSampleId != MP4_INVALID_SAMPLE_ID) {
        uint8_t* pSample = NULL;
        uint32_t sampleSize = 0;

        ASSERT(m_pRefTrack);
        m_pRefTrack->ReadSample(m_refSampleId, &pSample, &sampleSize);

        ASSERT(m_refSampleOffset + length <= sampleSize);

        file.WriteBytes(&pSample[m_refSampleOffset], length);

        MP4Free(pSample);
        return;
    }
}

MP4RtpSampleDescriptionData::MP4RtpSampleDescriptionData(MP4RtpPacket& packet)
        : MP4RtpData(packet)
{
    ((MP4Integer8Property*)m_pProperties[0])->SetValue(3);

    AddProperty( /* 1 */
        new MP4Integer8Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "trackRefIndex"));
    AddProperty( /* 2 */
        new MP4Integer16Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "length"));
    AddProperty( /* 3 */
        new MP4Integer32Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "sampleDescriptionIndex"));
    AddProperty( /* 4 */
        new MP4Integer32Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "sampleDescriptionOffset"));
    AddProperty( /* 5 */
        new MP4Integer32Property(this->GetPacket().GetHint().GetTrack().GetTrakAtom(), "reserved"));
}

void MP4RtpSampleDescriptionData::Set(uint32_t sampleDescrIndex,
                                      uint32_t offset, uint16_t length)
{
    ((MP4Integer16Property*)m_pProperties[2])->SetValue(length);
    ((MP4Integer32Property*)m_pProperties[3])->SetValue(sampleDescrIndex);
    ((MP4Integer32Property*)m_pProperties[4])->SetValue(offset);
}

uint16_t MP4RtpSampleDescriptionData::GetDataSize()
{
    return ((MP4Integer16Property*)m_pProperties[2])->GetValue();
}

void MP4RtpSampleDescriptionData::GetData(uint8_t* pDest)
{
    // we start with the index into our track references
    uint8_t trackRefIndex =
        ((MP4Integer8Property*)m_pProperties[1])->GetValue();

    // from which we can find the track structure
    MP4Track* pSampleTrack =
        FindTrackFromRefIndex(trackRefIndex);

    // next find the desired atom in the track's sample description table
    uint32_t sampleDescrIndex =
        ((MP4Integer32Property*)m_pProperties[3])->GetValue();

    MP4Atom& trakAtom =
        pSampleTrack->GetTrakAtom();

    char sdName[64];
    snprintf(sdName, 64, "trak.mdia.minf.stbl.stsd.*[%u]", sampleDescrIndex);

    MP4Atom* pSdAtom =
        trakAtom.FindAtom(sdName);

    // bad reference
    if (pSdAtom == NULL) {
        throw new Exception("invalid sample description index", __FILE__, __LINE__, __FUNCTION__ );
    }

    // check validity of the upcoming copy
    uint16_t length =
        ((MP4Integer16Property*)m_pProperties[2])->GetValue();
    uint32_t offset =
        ((MP4Integer32Property*)m_pProperties[4])->GetValue();

    if (offset + length > pSdAtom->GetSize()) {
        throw new Exception("offset and/or length are too large",
                            __FILE__, __LINE__, __FUNCTION__);
    }

    // now we use the raw file to get the desired bytes

    MP4File& file = GetPacket().GetHint().GetTrack().GetFile();

    uint64_t orgPos = file.GetPosition();

    // It's not entirely clear from the spec whether the offset is from
    // the start of the sample descirption atom, or the start of the atom's
    // data. I believe it is the former, but the commented out code will
    // realize the latter interpretation if I turn out to be wrong.
    uint64_t dataPos = pSdAtom->GetStart();
    //uint64_t dataPos = pSdAtom->GetEnd() - pSdAtom->GetSize();

    file.SetPosition(dataPos + offset);

    file.ReadBytes(pDest, length);

    file.SetPosition(orgPos);
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
