#include <QuickTimeComponents.h>
#include <Quickdraw.h>
#include "impl.h"
#include "cfhelper.h"
#include "qtimage.h"

namespace itmf = mp4v2::impl::itmf;

static
OSType GetMacOSFileType(const void *data, size_t size)
{
    itmf::BasicType typeCode = itmf::computeBasicType(data, size);
    switch (typeCode) {
    case itmf::BT_GIF:  return kQTFileTypeGIF;
    case itmf::BT_JPEG: return kQTFileTypeJPEG;
    case itmf::BT_PNG:  return kQTFileTypePNG;
    case itmf::BT_BMP:  return kQTFileTypeBMP;
    }
    return 0;
}

static
Handle CreateDataReferenceFromPointer(const void *data,
	size_t size, OSType ftype)
{
    Handle dataRef = 0;
    PointerDataRefRecord rec = { const_cast<void*>(data), size };
    TRYE(PtrToHand(&rec, &dataRef, sizeof rec));

    ComponentInstance handler;
    TRYE(OpenADataHandler(dataRef, PointerDataHandlerSubType,
		0, 0, 0, kDataHCanRead, &handler));
    x::shared_ptr<ComponentInstanceRecord>
	handlerPtr(handler, CloseComponent);

    Handle handle = 0;
    {
	unsigned char ch = 0;
	TRYE(PtrToHand(&ch, &handle, sizeof ch));
	x::shared_ptr<Ptr> disposer(handle, DisposeHandle);
	TRYE(DataHSetDataRefExtension(handler, handle,
				      kDataRefExtensionFileName));
    }
    {
	OSType ftypeB = EndianU32_NtoB(ftype);
	TRYE(PtrToHand(&ftypeB, &handle, sizeof ftypeB));
	x::shared_ptr<Ptr> disposer(handle, DisposeHandle);
	TRYE(DataHSetDataRefExtension(handler, handle,
				      kDataRefExtensionMacOSFileType));
    }
    return dataRef;
}

bool QTConvertArtwork(const void *data, size_t size, int maxSize,
	std::vector<char> *outImage)
{
    OSType ftype = GetMacOSFileType(data, size);
    if (!ftype) throw std::runtime_error("Unknown image format");
    Handle dataRef = CreateDataReferenceFromPointer(data, size, ftype);
    HandleX disposer(dataRef);

    ComponentInstance importer;
    TRYE(GetGraphicsImporterForDataRef(dataRef,
		PointerDataHandlerSubType, &importer));
    x::shared_ptr<ComponentInstanceRecord> importerPtr;

    Rect imageBounds;
    TRYE(GraphicsImportGetNaturalBounds(importer, &imageBounds));
    MacOffsetRect(&imageBounds, -imageBounds.left, -imageBounds.top);
    if (maxSize >= imageBounds.right || maxSize >= imageBounds.bottom)
	return false;

    TRYE(GraphicsImportSetQuality(importer, codecLosslessQuality));

    double scale = static_cast<double>(maxSize) /
	std::min(imageBounds.right, imageBounds.bottom);
    Rect newBounds = { 0 };
    newBounds.right = imageBounds.right * scale;
    newBounds.bottom = imageBounds.bottom * scale;
    TRYE(GraphicsImportSetBoundsRect(importer, &newBounds));

    ComponentInstance exporter;
    TRYE(OpenADefaultComponent(GraphicsExporterComponentType,
		kQTFileTypeJPEG, &exporter));
    x::shared_ptr<ComponentInstanceRecord> exporterPtr;
    TRYE(GraphicsExportSetInputGraphicsImporter(exporter, importer));
    TRYE(GraphicsExportSetCompressionQuality(exporter, codecHighQuality));

    Handle outPic = NewHandle(0);
    x::shared_ptr<Ptr> outPicPtr(outPic, DisposeHandle);
    TRYE(GraphicsExportSetOutputHandle(exporter, outPic));
    TRYE(GraphicsExportDoExport(exporter, 0));
    {
	HandleLockerX lock(outPic);
	size_t size = GetHandleSize(outPic);
	std::vector<char> vec(size);
	std::memcpy(&vec[0], *outPic, size);
	outImage->swap(vec);
    }
    return true;
}
