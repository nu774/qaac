#ifdef __MINGW32__
#include <stdexcept>
#include "wicimage.h"

bool WICConvertArtwork(const void *data, size_t size, unsigned maxSize,
	std::vector<char> *outImage)
{
    throw std::runtime_error("Not supported");
}
#endif

#ifdef _MSC_VER
#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <comdef.h>
#include <wincodec.h>
#include "util.h"
#include "win32util.h"
#include "wicimage.h"

_COM_SMARTPTR_TYPEDEF(IStream, __uuidof(IStream));
_COM_SMARTPTR_TYPEDEF(IPropertyBag2, __uuidof(IPropertyBag2));
_COM_SMARTPTR_TYPEDEF(IWICImagingFactory, __uuidof(IWICImagingFactory));
_COM_SMARTPTR_TYPEDEF(IWICStream, __uuidof(IWICStream));
_COM_SMARTPTR_TYPEDEF(IWICBitmap, __uuidof(IWICBitmap));
_COM_SMARTPTR_TYPEDEF(IWICBitmapDecoder, __uuidof(IWICBitmapDecoder));
_COM_SMARTPTR_TYPEDEF(IWICBitmapFrameDecode, __uuidof(IWICBitmapFrameDecode));
_COM_SMARTPTR_TYPEDEF(IWICBitmapSource, __uuidof(IWICBitmapSource));
_COM_SMARTPTR_TYPEDEF(IWICBitmapScaler, __uuidof(IWICBitmapScaler));
_COM_SMARTPTR_TYPEDEF(IWICBitmapEncoder, __uuidof(IWICBitmapEncoder));
_COM_SMARTPTR_TYPEDEF(IWICBitmapFrameEncode, __uuidof(IWICBitmapFrameEncode));

inline
void throwIfError(HRESULT expr, const char *msg)
{
    if (FAILED(expr))
	throw_win32_error(msg, expr);
}
#define HR(expr) (void)(throwIfError((expr), #expr))

static IWICBitmapSourcePtr
OpenSource(IWICImagingFactory *factory, const void *data, size_t size)
{
    IWICStreamPtr stream;
    HR(factory->CreateStream(&stream));
    HR(stream->InitializeFromMemory(
		static_cast<BYTE*>(const_cast<void*>(data)), size));

    IWICBitmapDecoderPtr decoder;
    HR(factory->CreateDecoderFromStream(stream, 0,
		WICDecodeMetadataCacheOnDemand, &decoder));

    IWICBitmapFrameDecodePtr source;
    HR(decoder->GetFrame(0, &source));
    return source;
}

static
void SetJpegEncodingQuality(IPropertyBag2 *props, float quality)
{
    PROPBAG2 option = { 0 };
    option.pstrName = L"ImageQuality";
    VARIANT value;
    VariantInit(&value);
    value.vt = VT_R4;
    value.fltVal = quality;
    HR(props->Write(1, &option, &value));
}

bool WICConvertArtwork(const void *data, size_t size, unsigned maxSize,
	std::vector<char> *outImage)
{
    IWICImagingFactoryPtr factory;
    HR(factory.CreateInstance(CLSID_WICImagingFactory));

    IWICBitmapSourcePtr source(OpenSource(factory, data, size));
    
    UINT width, height;
    HR(source->GetSize(&width, &height));
    if (maxSize >= width || maxSize >= height)
	return false;

    double scale = static_cast<double>(maxSize) / std::min(width, height);
    UINT newWidth = static_cast<UINT>(width * scale),
	 newHeight = static_cast<UINT>(height * scale);

    IWICBitmapScalerPtr scaler;
    HR(factory->CreateBitmapScaler(&scaler));
    HR(scaler->Initialize(source, newWidth, newHeight,
		WICBitmapInterpolationModeFant));

    IStreamPtr ostream;
    HR(CreateStreamOnHGlobal(0, TRUE, &ostream));

    IWICBitmapEncoderPtr encoder;
    HR(factory->CreateEncoder(GUID_ContainerFormatJpeg, 0, &encoder));
    HR(encoder->Initialize(ostream, WICBitmapEncoderNoCache));

    IWICBitmapFrameEncodePtr sink;
    IPropertyBag2Ptr props;
    HR(encoder->CreateNewFrame(&sink, &props));

    SetJpegEncodingQuality(props, 0.95f);
    HR(sink->Initialize(props));
    HR(sink->WriteSource(scaler, 0));
    HR(sink->Commit());
    HR(encoder->Commit());

    LARGE_INTEGER li = { 0 };
    ULARGE_INTEGER ui;
    HR(ostream->Seek(li, STREAM_SEEK_END, &ui));
    std::vector<char> vec(ui.LowPart);
    HR(ostream->Seek(li, STREAM_SEEK_SET, &ui));
    ULONG nread;
    HR(ostream->Read(&vec[0], vec.size(), &nread));
    outImage->swap(vec);
    return true;
}
#endif // _MSC_VER
