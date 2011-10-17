#ifndef QTMETA_H
#define QTMETA_H
#include "qthelper.h"

class QTMetaDataX: public PropertySupport<QTMetaDataX> {
    typedef std::tr1::shared_ptr<OpaqueQTMetaDataRef> owner_t;
    owner_t m_instance;
public:
    QTMetaDataX() {}
    QTMetaDataX(QTMetaDataRef meta): m_instance(meta, QTMetaDataRelease) {}
    operator QTMetaDataRef() { return m_instance.get(); }
    static QTMetaDataX FromMovie(Movie movie)
    {
	QTMetaDataRef meta;
	TRYE(QTCopyMovieMetaData(movie, &meta));
	return QTMetaDataX(meta);
    }
    static QTMetaDataX FromTrack(Track track)
    {
	QTMetaDataRef meta;
	TRYE(QTCopyTrackMetaData(track, &meta));
	return QTMetaDataX(meta);
    }
    static QTMetaDataX FromMedia(Media media)
    {
	QTMetaDataRef meta;
	TRYE(QTCopyMediaMetaData(media, &meta));
	return QTMetaDataX(meta);
    }
};


class QTMetaDataItemX: public PropertySupport<QTMetaDataItemX> {
    QTMetaDataRef m_meta;
    QTMetaDataItem m_item;
public:
    QTMetaDataItemX(QTMetaDataRef meta, QTMetaDataItem item)
	: m_meta(meta), m_item(item)
    {}

    void getValue(std::vector<uint8_t> *result)
    {
	getVectorProperty(
		kPropertyClass_MetaDataItem,
		kQTMetaDataItemPropertyID_Value,
		result);
    }
    UInt32 getDataType()
    {
	UInt32 result;
	getPodProperty(
		kPropertyClass_MetaDataItem,
		kQTMetaDataItemPropertyID_DataType,
		&result);
	return result;
    }
    UInt32 getStorageFormat()
    {
	UInt32 result;
	getPodProperty(
		kPropertyClass_MetaDataItem,
		kQTMetaDataItemPropertyID_StorageFormat,
		&result);
	return result;
    }
    void getKey(std::vector<uint8_t> *result)
    {
	getVectorProperty(
		kPropertyClass_MetaDataItem,
		kQTMetaDataItemPropertyID_Key,
		result);
    }
    OSType getKeyFormat()
    {
	OSType result;
	getPodProperty(
		kPropertyClass_MetaDataItem,
		kQTMetaDataItemPropertyID_Key,
		&result);
	return result;
    }

    /* for PropertySupport */
    long _getPropertyInfo(
	    OSType inPropClass,
	    OSType inPropID,
	    OSType *outPropType,
	    ByteCount *outPropValueSize,
	    UInt32 *outPropertyFlags)
    {
	return QTMetaDataGetItemPropertyInfo(
		    m_meta,
		    m_item,
		    inPropClass,
		    inPropID,
		    outPropType,
		    outPropValueSize,
		    outPropertyFlags);
    }
    long _getProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    void * outPropValueAddress,
	    ByteCount *outPropValueSizeUsed)
    {
	return QTMetaDataGetItemProperty(
		    m_meta,
		    m_item,
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    outPropValueAddress,
		    outPropValueSizeUsed);
    }
    long _setProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    const void *inPropValueAddress)
    {
	return QTMetaDataSetItemProperty(
		    m_meta,
		    m_item,
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    inPropValueAddress);
    }
};

#endif
