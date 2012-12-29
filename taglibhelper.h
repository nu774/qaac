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

    class FDIOStreamReader: public TagLib::IOStream {
        int m_fd;
    public:
        FDIOStreamReader(int fd): m_fd(fd) {}
        FileName name() const { return L"Dummy"; }
        ByteVector readBlock(ulong length)
        {
            ByteVector v(static_cast<uint>(length));
            int n = util::nread(m_fd, v.data(), length);
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
            _lseeki64(m_fd, offset, p);
        }
        void clear() { }
        long tell() const
        {
            int64_t n = _lseeki64(m_fd, 0, SEEK_CUR);
            if (n > 0xffffffffLL)
                throw std::runtime_error("File position exceeded "
                                         "the limit of TagLib");
            return n;
        }
        long length()
        {
            int64_t n = _filelengthi64(m_fd);
            if (n > 0xffffffffLL)
                throw std::runtime_error("File size exceeded "
                                         "the limit of TagLib");
            return n;
        }
        void truncate(long length)
        {
            throw NotImplemented("truncate");
        }
    };
}
