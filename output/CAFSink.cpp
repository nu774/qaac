#include <cstdio>
#include <iostream>
#include <sstream>
#include "CAFSink.h"
#include "win32util.h"

void CAFSink::init(const std::shared_ptr<FILE> &file,
                   const AudioStreamBasicDescription &asbd,
                   uint32_t channel_layout,
                   const std::vector<uint8_t> &cookie)
{
    m_data_pos = 0;
    m_bytes_written = 0;
    m_frames_written = 0;
    m_file = file;
    m_channel_layout = channel_layout;
    m_magic_cookie.assign(cookie.begin(), cookie.end());
    m_asbd = asbd;
    m_seekable = win32::is_seekable(fileno(m_file.get()));
    if (asbd.mFormatID != 'lpcm' && !m_seekable) {
        throw std::runtime_error("piped output of CAF is only available for "
                                 "LPCM");
    }
}

void CAFSink::beginWrite()
{
    write("caff\0\001\0\0", 8);
    desc();
    if (m_channel_layout)
        chan();
    if (m_magic_cookie.size())
        kuki();
    if (m_asbd.mFormatID == 'aach')
        ldsc();
    if (m_tags.size())
        info();
    data();
}

void CAFSink::writeSamples(const void *data, size_t length, size_t nsamples)
{
    void *bp = const_cast<void *>(data); /* XXX */
    uint32_t pbpf = packedBytesPerFrame();
    if (pbpf < m_asbd.mBytesPerFrame) {
        util::pack(bp, &length,
                   m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame,
                   pbpf / m_asbd.mChannelsPerFrame);
    }
    write(bp, length);
    m_bytes_written += length;
    m_frames_written += nsamples;
    if (m_asbd.mBytesPerFrame == 0)
        m_packet_table.push_back(length);
}

void CAFSink::finishWrite(const AudioFilePacketTableInfo &info)
{
    if (!m_seekable)
        return;
    int64_t pos = _ftelli64(m_file.get());
    if (_fseeki64(m_file.get(), m_data_pos - 12, SEEK_SET) == 0)
        write64(m_bytes_written + 4);
    if (m_asbd.mBytesPerFrame == 0 && _fseeki64(m_file.get(), pos, SEEK_SET) == 0)
        pakt(info);
}

void CAFSink::writeBER(uint32_t n)
{
    unsigned char buf[5] = { 0 };
    int i;

    for (i = 0; n; n >>= 7)
        buf[i++] = ((n & 0x7f) | 0x80);

    buf[0] ^= 0x80;

    for (--i; i >= 0; --i)
        _putc_nolock(buf[i], m_file.get());
}

void CAFSink::writeASBD(uint32_t format)
{
    uint32_t flags = m_asbd.mFormatFlags;
    bool downsampled = m_asbd.mFormatID == 'aach' && format == 'aac ';

    if (format == 'lpcm')
        flags = (2 | (m_asbd.mFormatFlags & kAudioFormatFlagIsFloat));

    writef64(downsampled ? m_asbd.mSampleRate / 2.0 : m_asbd.mSampleRate);
    write32(format);
    write32(flags);
    write32(packedBytesPerFrame());
    write32(downsampled ? m_asbd.mFramesPerPacket / 2
                        : m_asbd.mFramesPerPacket);
    write32(m_asbd.mChannelsPerFrame);
    write32(m_asbd.mBitsPerChannel);
}

void CAFSink::desc()
{
    write("desc", 4);
    write64(32);
    writeASBD(m_asbd.mFormatID == 'aach' ? 'aac ' : m_asbd.mFormatID);
}

void CAFSink::chan()
{
    write("chan", 4);
    write64(12);

    if (m_asbd.mFormatID == 'lpcm') {
        write32(kAudioChannelLayoutTag_UseChannelBitmap);
        write32(m_channel_layout);
    } else {
        write32(m_channel_layout);
        write32(0);
    }
    write32(0);
}

void CAFSink::kuki()
{
    write("kuki", 4);
    write64(m_magic_cookie.size());
    write(&m_magic_cookie[0], m_magic_cookie.size());
}

void CAFSink::ldsc()
{
    write("ldsc", 4);
    write64(36 * 2);
    writeASBD('aach');
    write32(m_channel_layout);
    writeASBD('aac ');
    write32(m_channel_layout);
}

void CAFSink::info()
{
    size_t count = m_tags.size();
    std::string buf;
    buf.push_back(count >> 24);
    buf.push_back((count >> 16) & 0xff);
    buf.push_back((count >> 8) & 0xff);
    buf.push_back(count & 0xff);

    std::map<std::string, std::string>::const_iterator it;
    for (it = m_tags.begin(); it != m_tags.end(); ++it) {
        buf.append(it->first).push_back('\0');
        buf.append(it->second).push_back('\0');
    }
    write("info", 4);
    write64(buf.size());
    write(buf.data(), buf.size());
}

void CAFSink::data()
{
    write("data", 4);
    write64(-1LL); /* initially unknown */
    write32(0);    /* mEditCount */
    m_data_pos = std::ftell(m_file.get());
}

void CAFSink::pakt(const AudioFilePacketTableInfo &info)
{
    int64_t pakt_pos = _ftelli64(m_file.get());
    write("pakt", 4);
    write64(0);
    write64(m_packet_table.size());
    if (info.mPrimingFrames || info.mRemainderFrames)
        write64(info.mNumberValidFrames);
    else
        write64(m_frames_written);
    write32(info.mPrimingFrames);
    if (info.mPrimingFrames || info.mRemainderFrames)
        write32(info.mRemainderFrames);
    else {
        uint32_t remainder =
            m_packet_table.size() * m_asbd.mFramesPerPacket - m_frames_written;
        write32(remainder);
    }
    for (size_t i = 0; i < m_packet_table.size(); ++i)
        writeBER(m_packet_table[i]);

    int64_t off = _ftelli64(m_file.get());
    if (_fseeki64(m_file.get(), pakt_pos + 4, SEEK_SET) == 0)
        write64(off - pakt_pos - 12);
}
