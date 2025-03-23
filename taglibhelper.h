#include <tiostream.h>

namespace TagLibX {
    using TagLib::FileName;
    using TagLib::ByteVector;
    using TagLib::uint;
    using TagLib::ulong;

    class NotImplemented: public std::runtime_error {
    public:
        NotImplemented():
            std::runtime_error("Not implemented") {}
        NotImplemented(const std::string &msg):
            std::runtime_error(msg + ": Not implemented") {}
    };

    class IStreamReader: public TagLib::IOStream {
        std::shared_ptr<IInputStream> m_stream;
    public:
        IStreamReader(std::shared_ptr<IInputStream> stream): m_stream(stream) {}
        FileName name() const { return L"Dummy"; }
        ByteVector readBlock(ulong length)
        {
            ByteVector v(static_cast<uint>(length));
            int n = m_stream->read(v.data(), length);
            v.resize(std::max(0, n));
            return v;
        }
        void writeBlock(const ByteVector &data)
        {
            throw NotImplemented("writeBlock");
        }
        void insert(const ByteVector &data, ulong start, ulong replace)
        {
            throw NotImplemented("insert");
        }
        void removeBlock(ulong start=0, ulong length=0)
        {
            throw NotImplemented("removeBlock");
        }

        bool readOnly() const { return true; }
        bool isOpen() const { return true; }

        void seek(long offset, IOStream::Position p = Beginning)
        {
            m_stream->seek(offset, p);
        }
        void clear() { }
        long tell() const
        {
            return m_stream->tell();
        }
        long length()
        {
            return m_stream->size();
        }
        void truncate(long length)
        {
            throw NotImplemented("truncate");
        }
    };
}
