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
 * 3GPP features implementation is based on 3GPP's TS26.234-v5.60,
 * and was contributed by Ximpo Group Ltd.
 *
 * Portions created by Ximpo Group Ltd. are
 * Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie       dmackie@cisco.com
 *              Alix Marchandise-Franquet alix@cisco.com
 *              Ximpo Group Ltd.          mp4v2@ximpo.com
 *              Bill May                  wmay@cisco.com
 */

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4File::MP4File( ) :
    m_file             ( NULL )
    , m_fileOriginalSize ( 0 )
    , m_createFlags      ( 0 )
{
    this->Init();
}

/**
 * Initialize member variables (shared among constructors)
 */
void MP4File::Init()
{
    m_pRootAtom = NULL;
    m_odTrackId = MP4_INVALID_TRACK_ID;

    m_useIsma = false;

    m_pModificationProperty = NULL;
    m_pTimeScaleProperty = NULL;
    m_pDurationProperty = NULL;

    m_memoryBuffer = NULL;
    m_memoryBufferSize = 0;
    m_memoryBufferPosition = 0;

    m_numReadBits = 0;
    m_bufReadBits = 0;
    m_numWriteBits = 0;
    m_bufWriteBits = 0;
    m_editName = NULL;
    m_trakName[0] = '\0';
}

MP4File::~MP4File()
{
    delete m_pRootAtom;
    for( uint32_t i = 0; i < m_pTracks.Size(); i++ )
        delete m_pTracks[i];
    MP4Free( m_memoryBuffer ); // just in case
    CHECK_AND_FREE( m_editName );
    delete m_file;
}

const std::string &
MP4File::GetFilename() const
{
    // No one should call this unless Read, etc. has
    // succeeded and m_file exists since this method really
    // only exists for the public API.  This helps us
    // guarantee that MP4GetFilename always returns a valid
    // string given a valid MP4FileHandle
    ASSERT(m_file);
    return m_file->name;
}

void MP4File::Read( const char* name, const MP4FileProvider* provider )
{
    Open( name, File::MODE_READ, provider );
    ReadFromFile();
    CacheProperties();
}

void MP4File::Create( const char* fileName,
                      uint32_t    flags,
                      int         add_ftyp,
                      int         add_iods,
                      char*       majorBrand,
                      uint32_t    minorVersion,
                      char**      supportedBrands,
                      uint32_t    supportedBrandsCount )
{
    m_createFlags = flags;
    Open( fileName, File::MODE_CREATE, NULL );

    // generate a skeletal atom tree
    m_pRootAtom = MP4Atom::CreateAtom(*this, NULL, NULL);
    m_pRootAtom->Generate();

    if (add_ftyp != 0) {
        MakeFtypAtom(majorBrand, minorVersion,
                     supportedBrands, supportedBrandsCount);
    }

    CacheProperties();

    // create mdat, and insert it after ftyp, and before moov
    (void)InsertChildAtom(m_pRootAtom, "mdat",
                          add_ftyp != 0 ? 1 : 0);

    // start writing
    m_pRootAtom->BeginWrite();
    if (add_iods != 0) {
        (void)AddChildAtom("moov", "iods");
    }
}

bool MP4File::Use64Bits (const char *atomName)
{
    uint32_t atomid = ATOMID(atomName);
    if (atomid == ATOMID("mdat") || atomid == ATOMID("stbl")) {
        return (m_createFlags & MP4_CREATE_64BIT_DATA) == MP4_CREATE_64BIT_DATA;
    }
    if (atomid == ATOMID("mvhd") ||
            atomid == ATOMID("tkhd") ||
            atomid == ATOMID("mdhd")) {
        return (m_createFlags & MP4_CREATE_64BIT_TIME) == MP4_CREATE_64BIT_TIME;
    }
    return false;
}

void MP4File::Check64BitStatus (const char *atomName)
{
    uint32_t atomid = ATOMID(atomName);

    if (atomid == ATOMID("mdat") || atomid == ATOMID("stbl")) {
        m_createFlags |= MP4_CREATE_64BIT_DATA;
    } else if (atomid == ATOMID("mvhd") ||
               atomid == ATOMID("tkhd") ||
               atomid == ATOMID("mdhd")) {
        m_createFlags |= MP4_CREATE_64BIT_TIME;
    }
}


bool MP4File::Modify( const char* fileName )
{
    Open( fileName, File::MODE_MODIFY, NULL );
    ReadFromFile();

    // find the moov atom
    MP4Atom* pMoovAtom = m_pRootAtom->FindAtom("moov");
    uint32_t numAtoms;

    if (pMoovAtom == NULL) {
        // there isn't one, odd but we can still proceed
        log.warningf("%s: \"%s\": no moov atom, can't modify",
                     __FUNCTION__, GetFilename().c_str());
        return false;
        //pMoovAtom = AddChildAtom(m_pRootAtom, "moov");
    } else {
        numAtoms = m_pRootAtom->GetNumberOfChildAtoms();

        // work backwards thru the top level atoms
        int32_t i;
        bool lastAtomIsMoov = true;
        MP4Atom* pLastAtom = NULL;

        for (i = numAtoms - 1; i >= 0; i--) {
            MP4Atom* pAtom = m_pRootAtom->GetChildAtom(i);
            const char* type = pAtom->GetType();

            // get rid of any trailing free or skips
            if (!strcmp(type, "free") || !strcmp(type, "skip")) {
                m_pRootAtom->DeleteChildAtom(pAtom);
                continue;
            }

            if (strcmp(type, "moov")) {
                if (pLastAtom == NULL) {
                    pLastAtom = pAtom;
                    lastAtomIsMoov = false;
                }
                continue;
            }

            // now at moov atom

            // multiple moov atoms?!?
            if (pAtom != pMoovAtom) {
                throw new Exception(
                    "Badly formed mp4 file, multiple moov atoms",
                    __FILE__,__LINE__,__FUNCTION__);
            }

            if (lastAtomIsMoov) {
                // position to start of moov atom,
                // effectively truncating file
                // prior to adding new mdat
                SetPosition(pMoovAtom->GetStart());

            } else { // last atom isn't moov
                // need to place a free atom
                MP4Atom* pFreeAtom = MP4Atom::CreateAtom(*this, NULL, "free");

                // in existing position of the moov atom
                m_pRootAtom->InsertChildAtom(pFreeAtom, i);
                m_pRootAtom->DeleteChildAtom(pMoovAtom);
                m_pRootAtom->AddChildAtom(pMoovAtom);

                // write free atom to disk
                SetPosition(pMoovAtom->GetStart());
                pFreeAtom->SetSize(pMoovAtom->GetSize());
                pFreeAtom->Write();

                // finally set our file position to the end of the last atom
                SetPosition(pLastAtom->GetEnd());
            }

            break;
        }
        ASSERT(i != -1);
    }

    CacheProperties();  // of moov atom

    numAtoms = m_pRootAtom->GetNumberOfChildAtoms();

    // insert another mdat prior to moov atom (the last atom)
    MP4Atom* pMdatAtom = InsertChildAtom(m_pRootAtom, "mdat", numAtoms - 1);

    // start writing new mdat
    pMdatAtom->BeginWrite(Use64Bits("mdat"));
    return true;
}

void MP4File::Optimize( const char* srcFileName, const char* dstFileName )
{
    File* src = NULL;
    File* dst = NULL;

    // compute destination filename
    string dname;
    if( dstFileName ) {
        dname = dstFileName;
    } else {
        // No destination given, so let's kludge together a temporary file.
        // We'll try to create it in the same directory as the srcFileName, since
        // it's more likely that directory is writable.  In the absence of that,
        // we'll create it in "./", which is the default pathnameTemp() provides.
        string s(srcFileName);
        size_t pos = s.find_last_of("\\/");
        const char *d;
        if (pos == string::npos) {
            d = ".";
        } else {
            s = s.substr(0, pos);
            d = s.c_str();
        }
        FileSystem::pathnameTemp( dname, d, "tmp", ".mp4" );
    }

    try {
        // file source to optimize
        Open( srcFileName, File::MODE_READ, NULL );
        ReadFromFile();
        CacheProperties(); // of moov atom

        src = m_file;
        m_file = NULL;

        // optimized file destination
        Open( dname.c_str(), File::MODE_CREATE, NULL );
        dst = m_file;

        SetIntegerProperty( "moov.mvhd.modificationTime", MP4GetAbsTimestamp() );

        // writing meta info in the optimal order
        ((MP4RootAtom*)m_pRootAtom)->BeginOptimalWrite();

        // write data in optimal order
        RewriteMdat( *src, *dst );

        // finish writing
        ((MP4RootAtom*)m_pRootAtom)->FinishOptimalWrite();

    }
    catch (...) {
        // cleanup and rethrow.  Without this, we'd leak memory and an open file handle(s).
       if(src == NULL && dst == NULL)
            delete m_file;// We didn't make it far enough to have m_file go to src or dst.

        m_file = NULL;
        delete dst;
        delete src;
        throw;
    }

    // cleanup
    delete dst;
    delete src;
    m_file = NULL;

    // move temporary file into place
    if( !dstFileName )
        Rename( dname.c_str(), srcFileName );
}

void MP4File::RewriteMdat( File& src, File& dst )
{
    uint32_t numTracks = m_pTracks.Size();

    MP4ChunkId* chunkIds = new MP4ChunkId[numTracks];
    MP4ChunkId* maxChunkIds = new MP4ChunkId[numTracks];
    MP4Timestamp* nextChunkTimes = new MP4Timestamp[numTracks];

    for( uint32_t i = 0; i < numTracks; i++ ) {
        chunkIds[i] = 1;
        maxChunkIds[i] = m_pTracks[i]->GetNumberOfChunks();
        nextChunkTimes[i] = MP4_INVALID_TIMESTAMP;
    }

    for( ;; ) {
        uint32_t nextTrackIndex = (uint32_t)-1;
        MP4Timestamp nextTime = MP4_INVALID_TIMESTAMP;

        for( uint32_t i = 0; i < numTracks; i++ ) {
            if( chunkIds[i] > maxChunkIds[i] )
                continue;

            if( nextChunkTimes[i] == MP4_INVALID_TIMESTAMP ) {
                MP4Timestamp chunkTime = m_pTracks[i]->GetChunkTime( chunkIds[i] );
                nextChunkTimes[i] = MP4ConvertTime( chunkTime, m_pTracks[i]->GetTimeScale(), GetTimeScale() );
            }

            // time is not earliest so far
            if( nextChunkTimes[i] > nextTime )
                continue;

            // prefer hint tracks to media tracks if times are equal
            if( nextChunkTimes[i] == nextTime && strcmp( m_pTracks[i]->GetType(), MP4_HINT_TRACK_TYPE ))
                continue;

            // this is our current choice of tracks
            nextTime = nextChunkTimes[i];
            nextTrackIndex = i;
        }

        if( nextTrackIndex == (uint32_t)-1 )
            break;

        uint8_t* pChunk;
        uint32_t chunkSize;

        // point into original mp4 file for read chunk call
        m_file = &src;
        m_pTracks[nextTrackIndex]->ReadChunk( chunkIds[nextTrackIndex], &pChunk, &chunkSize );

        // point back at the new mp4 file for write chunk
        m_file = &dst;
        m_pTracks[nextTrackIndex]->RewriteChunk( chunkIds[nextTrackIndex], pChunk, chunkSize );

        MP4Free( pChunk );

        chunkIds[nextTrackIndex]++;
        nextChunkTimes[nextTrackIndex] = MP4_INVALID_TIMESTAMP;
    }

    delete [] chunkIds;
    delete [] maxChunkIds;
    delete [] nextChunkTimes;
}

void MP4File::Open( const char* name, File::Mode mode, const MP4FileProvider* provider )
{
    ASSERT( !m_file );

    m_file = new File( name, mode, provider ? new io::CustomFileProvider( *provider ) : NULL );
    if( m_file->open() ) {
        ostringstream msg;
        msg << "open(" << name << ") failed";
        throw new Exception( msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }

    switch( mode ) {
        case File::MODE_READ:
        case File::MODE_MODIFY:
            m_fileOriginalSize = m_file->size;
            break;

        case File::MODE_CREATE:
        default:
            m_fileOriginalSize = 0;
            break;
    }
}

void MP4File::ReadFromFile()
{
    // ensure we start at beginning of file
    SetPosition(0);

    // create a new root atom
    ASSERT(m_pRootAtom == NULL);
    m_pRootAtom = MP4Atom::CreateAtom(*this, NULL, NULL);

    uint64_t fileSize = GetSize();

    m_pRootAtom->SetStart(0);
    m_pRootAtom->SetSize(fileSize);
    m_pRootAtom->SetEnd(fileSize);

    m_pRootAtom->Read();

    // create MP4Track's for any tracks in the file
    GenerateTracks();
}

void MP4File::GenerateTracks()
{
    uint32_t trackIndex = 0;

    while (true) {
        char trackName[32];
        snprintf(trackName, sizeof(trackName), "moov.trak[%u]", trackIndex);

        // find next trak atom
        MP4Atom* pTrakAtom = m_pRootAtom->FindAtom(trackName);

        // done, no more trak atoms
        if (pTrakAtom == NULL) {
            break;
        }

        // find track id property
        MP4Integer32Property* pTrackIdProperty = NULL;
        (void)pTrakAtom->FindProperty(
            "trak.tkhd.trackId",
            (MP4Property**)&pTrackIdProperty);

        // find track type property
        MP4StringProperty* pTypeProperty = NULL;
        (void)pTrakAtom->FindProperty(
            "trak.mdia.hdlr.handlerType",
            (MP4Property**)&pTypeProperty);

        // ensure we have the basics properties
        if (pTrackIdProperty && pTypeProperty) {

            m_trakIds.Add(pTrackIdProperty->GetValue());

            MP4Track* pTrack = NULL;
            try {
                if (!strcmp(pTypeProperty->GetValue(), MP4_HINT_TRACK_TYPE)) {
                    pTrack = new MP4RtpHintTrack(*this, *pTrakAtom);
                } else {
                    pTrack = new MP4Track(*this, *pTrakAtom);
                }
                m_pTracks.Add(pTrack);
            }
            catch( Exception* x ) {
                log.errorf(*x);
                delete x;
            }

            // remember when we encounter the OD track
            if (pTrack && !strcmp(pTrack->GetType(), MP4_OD_TRACK_TYPE)) {
                if (m_odTrackId == MP4_INVALID_TRACK_ID) {
                    m_odTrackId = pTrackIdProperty->GetValue();
                } else {
                    log.warningf("%s: \"%s\": multiple OD tracks present",
                                 __FUNCTION__, GetFilename().c_str() );
                }
            }
        } else {
            m_trakIds.Add(0);
        }

        trackIndex++;
    }
}

void MP4File::CacheProperties()
{
    FindIntegerProperty("moov.mvhd.modificationTime",
                        (MP4Property**)&m_pModificationProperty);

    FindIntegerProperty("moov.mvhd.timeScale",
                        (MP4Property**)&m_pTimeScaleProperty);

    FindIntegerProperty("moov.mvhd.duration",
                        (MP4Property**)&m_pDurationProperty);
}

void MP4File::BeginWrite()
{
    m_pRootAtom->BeginWrite();
}

void MP4File::FinishWrite(uint32_t options)
{
    // remove empty moov.udta.meta.ilst
    {
        MP4Atom* ilst = FindAtom( "moov.udta.meta.ilst" );
        if( ilst ) {
            if( ilst->GetNumberOfChildAtoms() == 0 ) {
                ilst->GetParentAtom()->DeleteChildAtom( ilst );
                delete ilst;
            }
        }
    }

    // remove empty moov.udta.meta
    {
        MP4Atom* meta = FindAtom( "moov.udta.meta" );
        if( meta ) {
            if( meta->GetNumberOfChildAtoms() == 0 ) {
                meta->GetParentAtom()->DeleteChildAtom( meta );
                delete meta;
            }
            else if( meta->GetNumberOfChildAtoms() == 1 ) {
                if( ATOMID( meta->GetChildAtom( 0 )->GetType() ) == ATOMID( "hdlr" )) {
                    meta->GetParentAtom()->DeleteChildAtom( meta );
                    delete meta;
                }
            }
        }
    }

    // remove empty moov.udta.name
    {
        MP4Atom* name = FindAtom( "moov.udta.name" );
        if( name ) {
            unsigned char *val = NULL;
            uint32_t valSize = 0;
            GetBytesProperty("moov.udta.name.value", (uint8_t**)&val, &valSize);
            if( valSize == 0 ) {
                name->GetParentAtom()->DeleteChildAtom( name );
                delete name;
            }
        }
    }

    // remove empty moov.udta
    {
        MP4Atom* udta = FindAtom( "moov.udta" );
        if( udta ) {
            if( udta->GetNumberOfChildAtoms() == 0 ) {
                udta->GetParentAtom()->DeleteChildAtom( udta );
                delete udta;
            }
        }
    }

    // for all tracks, flush chunking buffers
    for( uint32_t i = 0; i < m_pTracks.Size(); i++ ) {
        ASSERT( m_pTracks[i] );
        m_pTracks[i]->FinishWrite(options);
    }

    // ask root atom to write
    m_pRootAtom->FinishWrite();

    // finished all writes, if position < size then file has shrunk and
    // we mark remaining bytes as free atom; otherwise trailing garbage remains.
    if( GetPosition() < GetSize() ) {
        MP4RootAtom* root = (MP4RootAtom*)FindAtom( "" );
        ASSERT( root );

        // compute size of free atom; always has 8 bytes of overhead
        uint64_t size = GetSize() - GetPosition();
        if( size < 8 )
            size = 0;
        else
            size -= 8;

        MP4FreeAtom* freeAtom = (MP4FreeAtom*)MP4Atom::CreateAtom( *this, NULL, "free" );
        ASSERT( freeAtom );
        freeAtom->SetSize( size );
        root->AddChildAtom( freeAtom );
        freeAtom->Write();
    }
}

void MP4File::UpdateDuration(MP4Duration duration)
{
    MP4Duration currentDuration = GetDuration();
    if (duration > currentDuration) {
        SetDuration(duration);
    }
}

void MP4File::Dump( bool dumpImplicits )
{
    log.dump(0, MP4_LOG_VERBOSE1, "\"%s\": Dumping meta-information...", m_file->name.c_str() );
    m_pRootAtom->Dump( 0, dumpImplicits);
}

void MP4File::Close(uint32_t options)
{
    if( IsWriteMode() ) {
        SetIntegerProperty( "moov.mvhd.modificationTime", MP4GetAbsTimestamp() );
        FinishWrite(options);
    }

    delete m_file;
    m_file = NULL;
}

void MP4File::Rename(const char* oldFileName, const char* newFileName)
{
    if( FileSystem::rename( oldFileName, newFileName ))
        throw new PlatformException( sys::getLastErrorStr(), sys::getLastError(), __FILE__, __LINE__, __FUNCTION__ );
}

void MP4File::ProtectWriteOperation(const char* file,
                                    int         line,
                                    const char* func )
{
    if( !IsWriteMode() )
        throw new Exception( "operation not permitted in read mode", file, line, func );
}

MP4Track* MP4File::GetTrack(MP4TrackId trackId)
{
    return m_pTracks[FindTrackIndex(trackId)];
}

MP4Atom* MP4File::FindAtom(const char* name)
{
    MP4Atom* pAtom = NULL;
    if (!name || !strcmp(name, "")) {
        pAtom = m_pRootAtom;
    } else {
        pAtom = m_pRootAtom->FindAtom(name);
    }
    return pAtom;
}

MP4Atom* MP4File::AddChildAtom(
    const char* parentName,
    const char* childName)
{
    return AddChildAtom(FindAtom(parentName), childName);
}

MP4Atom* MP4File::AddChildAtom(
    MP4Atom* pParentAtom,
    const char* childName)
{
    return InsertChildAtom(pParentAtom, childName,
                           pParentAtom->GetNumberOfChildAtoms());
}

MP4Atom* MP4File::InsertChildAtom(
    const char* parentName,
    const char* childName,
    uint32_t index)
{
    return InsertChildAtom(FindAtom(parentName), childName, index);
}

MP4Atom* MP4File::InsertChildAtom(
    MP4Atom* pParentAtom,
    const char* childName,
    uint32_t index)
{
    MP4Atom* pChildAtom = MP4Atom::CreateAtom(*this, pParentAtom, childName);

    ASSERT(pParentAtom);
    pParentAtom->InsertChildAtom(pChildAtom, index);

    pChildAtom->Generate();

    return pChildAtom;
}

MP4Atom* MP4File::AddDescendantAtoms(
    const char* ancestorName,
    const char* descendantNames)
{
    return AddDescendantAtoms(FindAtom(ancestorName), descendantNames);
}

MP4Atom* MP4File::AddDescendantAtoms(
    MP4Atom* pAncestorAtom, const char* descendantNames)
{
    ASSERT(pAncestorAtom);

    MP4Atom* pParentAtom = pAncestorAtom;
    MP4Atom* pChildAtom = NULL;

    while (true) {
        char* childName = MP4NameFirst(descendantNames);

        if (childName == NULL) {
            break;
        }

        descendantNames = MP4NameAfterFirst(descendantNames);

        pChildAtom = pParentAtom->FindChildAtom(childName);

        if (pChildAtom == NULL) {
            pChildAtom = AddChildAtom(pParentAtom, childName);
        }

        pParentAtom = pChildAtom;

        MP4Free(childName);
    }

    return pChildAtom;
}

bool MP4File::FindProperty(const char* name,
                           MP4Property** ppProperty, uint32_t* pIndex)
{
    if( pIndex )
        *pIndex = 0; // set the default answer for index
    return m_pRootAtom->FindProperty(name, ppProperty, pIndex);
}

void MP4File::FindIntegerProperty(const char* name,
                                  MP4Property** ppProperty, uint32_t* pIndex)
{
    if (!FindProperty(name, ppProperty, pIndex)) {
        ostringstream msg;
        msg << "no such property - " << name;
        throw new Exception(msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }

    switch ((*ppProperty)->GetType()) {
    case Integer8Property:
    case Integer16Property:
    case Integer24Property:
    case Integer32Property:
    case Integer64Property:
        break;
    default:
        ostringstream msg;
        msg << "type mismatch - property " << name << " type " << (*ppProperty)->GetType();
        throw new Exception(msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }
}

uint64_t MP4File::GetIntegerProperty(const char* name)
{
    MP4Property* pProperty;
    uint32_t index;

    FindIntegerProperty(name, &pProperty, &index);

    return ((MP4IntegerProperty*)pProperty)->GetValue(index);
}

void MP4File::SetIntegerProperty(const char* name, uint64_t value)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Property* pProperty = NULL;
    uint32_t index = 0;

    FindIntegerProperty(name, &pProperty, &index);

    ((MP4IntegerProperty*)pProperty)->SetValue(value, index);
}

void MP4File::FindFloatProperty(const char* name,
                                MP4Property** ppProperty, uint32_t* pIndex)
{
    if (!FindProperty(name, ppProperty, pIndex)) {
        ostringstream msg;
        msg << "no such property - " << name;
        throw new Exception(msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }
    if ((*ppProperty)->GetType() != Float32Property) {
        ostringstream msg;
        msg << "type mismatch - property " << name << " type " << (*ppProperty)->GetType();
        throw new Exception(msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }
}

float MP4File::GetFloatProperty(const char* name)
{
    MP4Property* pProperty;
    uint32_t index;

    FindFloatProperty(name, &pProperty, &index);

    return ((MP4Float32Property*)pProperty)->GetValue(index);
}

void MP4File::SetFloatProperty(const char* name, float value)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Property* pProperty;
    uint32_t index;

    FindFloatProperty(name, &pProperty, &index);

    ((MP4Float32Property*)pProperty)->SetValue(value, index);
}

void MP4File::FindStringProperty(const char* name,
                                 MP4Property** ppProperty, uint32_t* pIndex)
{
    if (!FindProperty(name, ppProperty, pIndex)) {
        ostringstream msg;
        msg << "no such property - " << name;
        throw new Exception(msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }
    if ((*ppProperty)->GetType() != StringProperty) {
        ostringstream msg;
        msg << "type mismatch - property " << name << " type " << (*ppProperty)->GetType();
        throw new Exception(msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }
}

const char* MP4File::GetStringProperty(const char* name)
{
    MP4Property* pProperty;
    uint32_t index;

    FindStringProperty(name, &pProperty, &index);

    return ((MP4StringProperty*)pProperty)->GetValue(index);
}

void MP4File::SetStringProperty(const char* name, const char* value)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Property* pProperty;
    uint32_t index;

    FindStringProperty(name, &pProperty, &index);

    ((MP4StringProperty*)pProperty)->SetValue(value, index);
}

void MP4File::FindBytesProperty(const char* name,
                                MP4Property** ppProperty, uint32_t* pIndex)
{
    if (!FindProperty(name, ppProperty, pIndex)) {
        ostringstream msg;
        msg << "no such property " << name;
        throw new Exception(msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }
    if ((*ppProperty)->GetType() != BytesProperty) {
        ostringstream msg;
        msg << "type mismatch - property " << name << " - type " <<  (*ppProperty)->GetType();
        throw new Exception(msg.str(), __FILE__, __LINE__, __FUNCTION__);
    }
}

void MP4File::GetBytesProperty(const char* name,
                               uint8_t** ppValue, uint32_t* pValueSize)
{
    MP4Property* pProperty;
    uint32_t index;

    FindBytesProperty(name, &pProperty, &index);

    ((MP4BytesProperty*)pProperty)->GetValue(ppValue, pValueSize, index);
}

void MP4File::SetBytesProperty(const char* name,
                               const uint8_t* pValue, uint32_t valueSize)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Property* pProperty;
    uint32_t index;

    FindBytesProperty(name, &pProperty, &index);

    ((MP4BytesProperty*)pProperty)->SetValue(pValue, valueSize, index);
}


// track functions

MP4TrackId MP4File::AddTrack(const char* type, uint32_t timeScale)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    // create and add new trak atom
    MP4Atom* pTrakAtom = AddChildAtom("moov", "trak");
    ASSERT(pTrakAtom);

    // allocate a new track id
    MP4TrackId trackId = AllocTrackId();

    m_trakIds.Add(trackId);

    // set track id
    MP4Integer32Property* pInteger32Property = NULL;
    (void)pTrakAtom->FindProperty("trak.tkhd.trackId",
                                  (MP4Property**)&pInteger32Property);
    ASSERT(pInteger32Property);
    pInteger32Property->SetValue(trackId);

    // set track type
    const char* normType = MP4NormalizeTrackType(type);

    // sanity check for user defined types
    if (strlen(normType) > 4) {
        log.warningf("%s: \"%s\": type truncated to four characters",
                     __FUNCTION__, GetFilename().c_str());
        // StringProperty::SetValue() will do the actual truncation
    }

    MP4StringProperty* pStringProperty = NULL;
    (void)pTrakAtom->FindProperty("trak.mdia.hdlr.handlerType",
                                  (MP4Property**)&pStringProperty);
    ASSERT(pStringProperty);
    pStringProperty->SetValue(normType);

    // set track time scale
    pInteger32Property = NULL;
    (void)pTrakAtom->FindProperty("trak.mdia.mdhd.timeScale",
                                  (MP4Property**)&pInteger32Property);
    ASSERT(pInteger32Property);
    pInteger32Property->SetValue(timeScale ? timeScale : 1000);

    // now have enough to create MP4Track object
    MP4Track* pTrack = NULL;
    if (!strcmp(normType, MP4_HINT_TRACK_TYPE)) {
        pTrack = new MP4RtpHintTrack(*this, *pTrakAtom);
    } else {
        pTrack = new MP4Track(*this, *pTrakAtom);
    }
    m_pTracks.Add(pTrack);

    // mark non-hint tracks as enabled
    if (strcmp(normType, MP4_HINT_TRACK_TYPE)) {
        SetTrackIntegerProperty(trackId, "tkhd.flags", 1);
    }

    // mark track as contained in this file
    // LATER will provide option for external data references
    AddDataReference(trackId, NULL);

    return trackId;
}

void MP4File::AddTrackToIod(MP4TrackId trackId)
{
    MP4DescriptorProperty* pDescriptorProperty = NULL;
    (void)m_pRootAtom->FindProperty("moov.iods.esIds",
                                    (MP4Property**)&pDescriptorProperty);
    ASSERT(pDescriptorProperty);

    MP4Descriptor* pDescriptor =
        pDescriptorProperty->AddDescriptor(MP4ESIDIncDescrTag);
    ASSERT(pDescriptor);

    MP4Integer32Property* pIdProperty = NULL;
    (void)pDescriptor->FindProperty("id",
                                    (MP4Property**)&pIdProperty);
    ASSERT(pIdProperty);

    pIdProperty->SetValue(trackId);
}

void MP4File::RemoveTrackFromIod(MP4TrackId trackId, bool shallHaveIods)
{
    MP4DescriptorProperty* pDescriptorProperty = NULL;
    if (!m_pRootAtom->FindProperty("moov.iods.esIds",(MP4Property**)&pDescriptorProperty)
        || pDescriptorProperty == NULL)
        return;

    for (uint32_t i = 0; i < pDescriptorProperty->GetCount(); i++) {
        /* static */
        char name[32];
        snprintf(name, sizeof(name), "esIds[%u].id", i);

        MP4Integer32Property* pIdProperty = NULL;
        (void)pDescriptorProperty->FindProperty(name,
                                                (MP4Property**)&pIdProperty);
        // wmay ASSERT(pIdProperty);

        if (pIdProperty != NULL &&
                pIdProperty->GetValue() == trackId) {
            pDescriptorProperty->DeleteDescriptor(i);
            break;
        }
    }
}

void MP4File::AddTrackToOd(MP4TrackId trackId)
{
    if (!m_odTrackId) {
        return;
    }

    AddTrackReference(MakeTrackName(m_odTrackId, "tref.mpod"), trackId);
}

void MP4File::RemoveTrackFromOd(MP4TrackId trackId)
{
    if (!m_odTrackId) {
        return;
    }

    RemoveTrackReference(MakeTrackName(m_odTrackId, "tref.mpod"), trackId);
}

/*
 * Try to obtain the properties of this reference track, if not found then return
 * NULL in *ppCountProperty and *ppTrackIdProperty.
 */
void MP4File::GetTrackReferenceProperties(const char* trefName,
        MP4Property** ppCountProperty, MP4Property** ppTrackIdProperty)
{
    char propName[1024];

    snprintf(propName, sizeof(propName), "%s.%s", trefName, "entryCount");
    (void)m_pRootAtom->FindProperty(propName, ppCountProperty);

    snprintf(propName, sizeof(propName), "%s.%s", trefName, "entries.trackId");
    (void)m_pRootAtom->FindProperty(propName, ppTrackIdProperty);
}

void MP4File::AddTrackReference(const char* trefName, MP4TrackId refTrackId)
{
    MP4Integer32Property* pCountProperty = NULL;
    MP4Integer32Property* pTrackIdProperty = NULL;

    GetTrackReferenceProperties(trefName,
                                (MP4Property**)&pCountProperty,
                                (MP4Property**)&pTrackIdProperty);

    if (pCountProperty && pTrackIdProperty) {
        pTrackIdProperty->AddValue(refTrackId);
        pCountProperty->IncrementValue();
    }
}

uint32_t MP4File::FindTrackReference(const char* trefName,
                                     MP4TrackId refTrackId)
{
    MP4Integer32Property* pCountProperty = NULL;
    MP4Integer32Property* pTrackIdProperty = NULL;

    GetTrackReferenceProperties(trefName,
                                (MP4Property**)&pCountProperty,
                                (MP4Property**)&pTrackIdProperty);

    if (pCountProperty && pTrackIdProperty) {
        for (uint32_t i = 0; i < pCountProperty->GetValue(); i++) {
            if (refTrackId == pTrackIdProperty->GetValue(i)) {
                return i + 1;   // N.B. 1 not 0 based index
            }
        }
    }
    return 0;
}

void MP4File::RemoveTrackReference(const char* trefName, MP4TrackId refTrackId)
{
    MP4Integer32Property* pCountProperty = NULL;
    MP4Integer32Property* pTrackIdProperty = NULL;

    GetTrackReferenceProperties(trefName,
                                (MP4Property**)&pCountProperty,
                                (MP4Property**)&pTrackIdProperty);

    if (pCountProperty && pTrackIdProperty) {
        for (uint32_t i = 0; i < pCountProperty->GetValue(); i++) {
            if (refTrackId == pTrackIdProperty->GetValue(i)) {
                pTrackIdProperty->DeleteValue(i);
                pCountProperty->IncrementValue(-1);
            }
        }
    }
}

void MP4File::AddDataReference(MP4TrackId trackId, const char* url)
{
    MP4Atom* pDrefAtom =
        FindAtom(MakeTrackName(trackId, "mdia.minf.dinf.dref"));
    ASSERT(pDrefAtom);

    MP4Integer32Property* pCountProperty = NULL;
    (void)pDrefAtom->FindProperty("dref.entryCount",
                                  (MP4Property**)&pCountProperty);
    ASSERT(pCountProperty);
    pCountProperty->IncrementValue();

    MP4Atom* pUrlAtom = AddChildAtom(pDrefAtom, "url ");

    if (url && url[0] != '\0') {
        pUrlAtom->SetFlags(pUrlAtom->GetFlags() & 0xFFFFFE);

        MP4StringProperty* pUrlProperty = NULL;
        (void)pUrlAtom->FindProperty("url .location",
                                     (MP4Property**)&pUrlProperty);
        ASSERT(pUrlProperty);
        pUrlProperty->SetValue(url);
    } else {
        pUrlAtom->SetFlags(pUrlAtom->GetFlags() | 1);
    }
}

MP4TrackId MP4File::AddSystemsTrack(const char* type, uint32_t timeScale)
{
    const char* normType = MP4NormalizeTrackType(type);

    // TBD if user type, fix name to four chars, and warn

    MP4TrackId trackId = AddTrack(type, timeScale);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "nmhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "mp4s");

    AddDescendantAtoms(MakeTrackName(trackId, NULL), "udta.name");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4s atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4s.esds.ESID",
                            0
                           );

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr.objectTypeId",
                            MP4SystemsV1ObjectType);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr.streamType",
                            ConvertTrackTypeToStreamType(normType));

    return trackId;
}

MP4TrackId MP4File::AddODTrack()
{
    // until a demonstrated need emerges
    // we limit ourselves to one object description track
    if (m_odTrackId != MP4_INVALID_TRACK_ID) {
        throw new Exception("object description track already exists",__FILE__, __LINE__, __FUNCTION__);
    }

    m_odTrackId = AddSystemsTrack(MP4_OD_TRACK_TYPE);

    AddTrackToIod(m_odTrackId);

    (void)AddDescendantAtoms(MakeTrackName(m_odTrackId, NULL), "tref.mpod");

    return m_odTrackId;
}

MP4TrackId MP4File::AddSceneTrack()
{
    MP4TrackId trackId = AddSystemsTrack(MP4_SCENE_TRACK_TYPE);

    AddTrackToIod(trackId);
    AddTrackToOd(trackId);

    return trackId;
}

bool MP4File::ShallHaveIods()
{
    // NULL terminated list of brands which require the IODS atom
    const char* brandsWithIods[] = {
        "mp42",
        "isom",
        NULL
    };

    MP4FtypAtom* ftyp = (MP4FtypAtom*)m_pRootAtom->FindAtom( "ftyp" );
    if( !ftyp )
        return false;

    // check major brand
    const char* brand = ftyp->majorBrand.GetValue();
    for( uint32_t i = 0; brandsWithIods[i] != NULL; i++ ) {
        if( !strcasecmp( brandsWithIods[i], brand ))
            return true;
    }

    // check compatible brands
    uint32_t max = ftyp->compatibleBrands.GetCount();
    for( uint32_t i = 0; i < max; i++ ) {
        brand = ftyp->compatibleBrands.GetValue( i );
        for( uint32_t j = 0; brandsWithIods[j] != NULL ; j++) {
            if( !strcasecmp( brandsWithIods[j], brand ))
                return true;
        }
    }

    return false;
}

void MP4File::SetAmrVendor(
    MP4TrackId trackId,
    uint32_t vendor)
{
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*.damr.vendor",
                            vendor);
}

void MP4File::SetAmrDecoderVersion(
    MP4TrackId trackId,
    uint8_t decoderVersion)
{

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*.damr.decoderVersion",
                            decoderVersion);
}

void MP4File::SetAmrModeSet(
    MP4TrackId trackId,
    uint16_t modeSet)
{
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*.damr.modeSet",
                            modeSet);
}
uint16_t MP4File::GetAmrModeSet(MP4TrackId trackId)
{
    return GetTrackIntegerProperty(trackId,
                                   "mdia.minf.stbl.stsd.*.damr.modeSet");
}

MP4TrackId MP4File::AddAmrAudioTrack(
    uint32_t timeScale,
    uint16_t modeSet,
    uint8_t modeChangePeriod,
    uint8_t framesPerSample,
    bool isAmrWB)
{

    uint32_t fixedSampleDuration = (timeScale * 20)/1000; // 20mSec/Sample

    MP4TrackId trackId = AddTrack(MP4_AUDIO_TRACK_TYPE, timeScale);

    AddTrackToOd(trackId);

    SetTrackFloatProperty(trackId, "tkhd.volume", 1.0);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "smhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), isAmrWB ? "sawb" : "samr");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4a atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*.timeScale",
                            timeScale);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*.damr.modeSet",
                            modeSet);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*.damr.modeChangePeriod",
                            modeChangePeriod);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*.damr.framesPerSample",
                            framesPerSample);


    m_pTracks[FindTrackIndex(trackId)]->
    SetFixedSampleDuration(fixedSampleDuration);

    return trackId;
}

MP4TrackId MP4File::AddULawAudioTrack(    uint32_t timeScale)
{
    uint32_t fixedSampleDuration = (timeScale * 20)/1000; // 20mSec/Sample

    MP4TrackId trackId = AddTrack(MP4_AUDIO_TRACK_TYPE, timeScale);

    AddTrackToOd(trackId);

    SetTrackFloatProperty(trackId, "tkhd.volume", 1.0);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "smhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "ulaw");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4a atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.ulaw.timeScale",
                            timeScale<<16);

    m_pTracks[FindTrackIndex(trackId)]->SetFixedSampleDuration(fixedSampleDuration);

    return trackId;
}

MP4TrackId MP4File::AddALawAudioTrack(    uint32_t timeScale)
{
    uint32_t fixedSampleDuration = (timeScale * 20)/1000; // 20mSec/Sample

    MP4TrackId trackId = AddTrack(MP4_AUDIO_TRACK_TYPE, timeScale);

    AddTrackToOd(trackId);

    SetTrackFloatProperty(trackId, "tkhd.volume", 1.0);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "smhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "alaw");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4a atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.alaw.timeScale",
                            timeScale<<16);

    m_pTracks[FindTrackIndex(trackId)]->SetFixedSampleDuration(fixedSampleDuration);

    return trackId;
}

MP4TrackId MP4File::AddAudioTrack(
    uint32_t timeScale,
    MP4Duration sampleDuration,
    uint8_t audioType)
{
    MP4TrackId trackId = AddTrack(MP4_AUDIO_TRACK_TYPE, timeScale);

    AddTrackToOd(trackId);

    SetTrackFloatProperty(trackId, "tkhd.volume", 1.0);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "smhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "mp4a");

    AddDescendantAtoms(MakeTrackName(trackId, NULL), "udta.name");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4a atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4a.timeScale", timeScale << 16);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4a.esds.ESID",
                            0
                           );

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.objectTypeId",
                            audioType);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.streamType",
                            MP4AudioStreamType);

    m_pTracks[FindTrackIndex(trackId)]->
    SetFixedSampleDuration(sampleDuration);

    return trackId;
}

MP4TrackId MP4File::AddAC3AudioTrack(
    uint32_t samplingRate,
    uint8_t fscod,
    uint8_t bsid,
    uint8_t bsmod,
    uint8_t acmod,
    uint8_t lfeon,
    uint8_t bit_rate_code)
{
    MP4TrackId trackId = AddTrack(MP4_AUDIO_TRACK_TYPE, samplingRate);

    AddTrackToOd(trackId);

    SetTrackFloatProperty(trackId, "tkhd.volume", 1.0);

    InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "smhd", 0);

    AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "ac-3");

    // Set Ac3 settings
    MP4Integer16Property* pSampleRateProperty = NULL;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.ac-3.samplingRate"),
        (MP4Property**)&pSampleRateProperty);
    if (pSampleRateProperty) {
        pSampleRateProperty->SetValue(samplingRate);
    } else {
        throw new Exception("no ac-3.samplingRate property", __FILE__, __LINE__, __FUNCTION__);
    }

    MP4BitfieldProperty* pBitfieldProperty = NULL;

    FindProperty(MakeTrackName(trackId, "mdia.minf.stbl.stsd.ac-3.dac3.fscod"),
                               (MP4Property**)&pBitfieldProperty);
    if (pBitfieldProperty) {
        pBitfieldProperty->SetValue(fscod);
        pBitfieldProperty = NULL;
    } else {
        throw new Exception("no dac3.fscod property", __FILE__, __LINE__, __FUNCTION__);
    }

    FindProperty(MakeTrackName(trackId, "mdia.minf.stbl.stsd.ac-3.dac3.bsid"),
                               (MP4Property**)&pBitfieldProperty);
    if (pBitfieldProperty) {
        pBitfieldProperty->SetValue(bsid);
        pBitfieldProperty = NULL;
    } else {
        throw new Exception("no dac3.bsid property", __FILE__, __LINE__, __FUNCTION__);
    }

    FindProperty(MakeTrackName(trackId, "mdia.minf.stbl.stsd.ac-3.dac3.bsmod"),
                               (MP4Property**)&pBitfieldProperty);
    if (pBitfieldProperty) {
        pBitfieldProperty->SetValue(bsmod);
        pBitfieldProperty = NULL;
    } else {
        throw new Exception("no dac3.bsmod property", __FILE__, __LINE__, __FUNCTION__);
    }

    FindProperty(MakeTrackName(trackId, "mdia.minf.stbl.stsd.ac-3.dac3.acmod"),
                               (MP4Property**)&pBitfieldProperty);
    if (pBitfieldProperty) {
        pBitfieldProperty->SetValue(acmod);
        pBitfieldProperty = NULL;
    } else {
        throw new Exception("no dac3.acmod property", __FILE__, __LINE__, __FUNCTION__);
    }

    FindProperty(MakeTrackName(trackId, "mdia.minf.stbl.stsd.ac-3.dac3.lfeon"),
                               (MP4Property**)&pBitfieldProperty);
    if (pBitfieldProperty) {
        pBitfieldProperty->SetValue(lfeon);
        pBitfieldProperty = NULL;
    } else {
        throw new Exception("no dac3.lfeon property", __FILE__, __LINE__, __FUNCTION__);
    }

    FindProperty(MakeTrackName(trackId, "mdia.minf.stbl.stsd.ac-3.dac3.bit_rate_code"),
                               (MP4Property**)&pBitfieldProperty);
    if (pBitfieldProperty) {
        pBitfieldProperty->SetValue(bit_rate_code);
        pBitfieldProperty = NULL;
    } else {
        throw new Exception("no dac3.bit_rate_code property", __FILE__, __LINE__, __FUNCTION__);
    }

    AddDescendantAtoms(MakeTrackName(trackId, NULL), "udta.name");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4a atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    m_pTracks[FindTrackIndex(trackId)]->
        SetFixedSampleDuration(1536);

    return trackId;
}

MP4TrackId MP4File::AddEncAudioTrack(uint32_t timeScale,
                                     MP4Duration sampleDuration,
                                     uint8_t audioType,
                                     uint32_t scheme_type,
                                     uint16_t scheme_version,
                                     uint8_t  key_ind_len,
                                     uint8_t  iv_len,
                                     bool      selective_enc,
                                     const char *kms_uri,
                                     bool use_ismacryp
                                    )
{
    uint32_t original_fmt = 0;

    MP4TrackId trackId = AddTrack(MP4_AUDIO_TRACK_TYPE, timeScale);

    AddTrackToOd(trackId);

    SetTrackFloatProperty(trackId, "tkhd.volume", 1.0);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "smhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "enca");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the enca atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
                        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();


    /* set all the ismacryp-specific values */
    // original format is mp4a
    if (use_ismacryp) {
        original_fmt = ATOMID("mp4a");
        SetTrackIntegerProperty(trackId,
                                "mdia.minf.stbl.stsd.enca.sinf.frma.data-format",
                                original_fmt);

        (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.enca.sinf"),
                           "schm");
        (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.enca.sinf"),
                           "schi");
        (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.enca.sinf.schi"),
                           "iKMS");
        (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.enca.sinf.schi"),
                           "iSFM");
        SetTrackIntegerProperty(trackId,
                                "mdia.minf.stbl.stsd.enca.sinf.schm.scheme_type",
                                scheme_type);

        SetTrackIntegerProperty(trackId,
                                "mdia.minf.stbl.stsd.enca.sinf.schm.scheme_version",
                                scheme_version);

        SetTrackStringProperty(trackId,
                               "mdia.minf.stbl.stsd.enca.sinf.schi.iKMS.kms_URI",
                               kms_uri);

        SetTrackIntegerProperty(trackId,
                                "mdia.minf.stbl.stsd.enca.sinf.schi.iSFM.selective-encryption",
                                selective_enc);

        SetTrackIntegerProperty(trackId,
                                "mdia.minf.stbl.stsd.enca.sinf.schi.iSFM.key-indicator-length",
                                key_ind_len);

        SetTrackIntegerProperty(trackId,
                                "mdia.minf.stbl.stsd.enca.sinf.schi.iSFM.IV-length",
                                iv_len);
        /* end ismacryp */
    }

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.enca.timeScale", timeScale);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.enca.esds.ESID",
                            0
                           );

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.enca.esds.decConfigDescr.objectTypeId",
                            audioType);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.enca.esds.decConfigDescr.streamType",
                            MP4AudioStreamType);

    m_pTracks[FindTrackIndex(trackId)]->
    SetFixedSampleDuration(sampleDuration);

    return trackId;
}

MP4TrackId MP4File::AddCntlTrackDefault (uint32_t timeScale,
        MP4Duration sampleDuration,
        const char *type)
{
    MP4TrackId trackId = AddTrack(MP4_CNTL_TRACK_TYPE, timeScale);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "nmhd", 0);
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), type);

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4v atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsz.sampleSize", sampleDuration);

    m_pTracks[FindTrackIndex(trackId)]->
    SetFixedSampleDuration(sampleDuration);

    return trackId;
}

MP4TrackId MP4File::AddHrefTrack (uint32_t timeScale,
                                  MP4Duration sampleDuration,
                                  const char *base_url)
{
    MP4TrackId trackId = AddCntlTrackDefault(timeScale, sampleDuration, "href");

    if (base_url != NULL) {
        (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.href"),
                           "burl");
        SetTrackStringProperty(trackId, "mdia.minf.stbl.stsd.href.burl.base_url",
                               base_url);
    }

    return trackId;
}

MP4TrackId MP4File::AddVideoTrackDefault(
    uint32_t timeScale,
    MP4Duration sampleDuration,
    uint16_t width,
    uint16_t height,
    const char *videoType)
{
    MP4TrackId trackId = AddTrack(MP4_VIDEO_TRACK_TYPE, timeScale);

    AddTrackToOd(trackId);

    SetTrackFloatProperty(trackId, "tkhd.width", width);
    SetTrackFloatProperty(trackId, "tkhd.height", height);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "vmhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), videoType);

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4v atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsz.sampleSize", sampleDuration);

    m_pTracks[FindTrackIndex(trackId)]->
    SetFixedSampleDuration(sampleDuration);

    return trackId;
}
MP4TrackId MP4File::AddMP4VideoTrack(
    uint32_t timeScale,
    MP4Duration sampleDuration,
    uint16_t width,
    uint16_t height,
    uint8_t videoType)
{
    MP4TrackId trackId = AddVideoTrackDefault(timeScale,
                         sampleDuration,
                         width,
                         height,
                         "mp4v");

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4v.width", width);
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4v.height", height);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4v.esds.ESID",
                            0
                           );

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4v.esds.decConfigDescr.objectTypeId",
                            videoType);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4v.esds.decConfigDescr.streamType",
                            MP4VisualStreamType);

    return trackId;
}

// ismacrypted
MP4TrackId MP4File::AddEncVideoTrack(uint32_t timeScale,
                                     MP4Duration sampleDuration,
                                     uint16_t width,
                                     uint16_t height,
                                     uint8_t videoType,
                                     mp4v2_ismacrypParams *icPp,
                                     const char *oFormat
                                    )
{
    uint32_t original_fmt = 0;

    MP4TrackId trackId = AddVideoTrackDefault(timeScale,
                         sampleDuration,
                         width,
                         height,
                         "encv");

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.width", width);
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.height", height);

    /* set all the ismacryp-specific values */

    original_fmt = ATOMID(oFormat);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.sinf.frma.data-format",
                            original_fmt);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.sinf"),
                       "schm");
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.sinf"),
                       "schi");
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.sinf.schi"),
                       "iKMS");
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.sinf.schi"),
                       "iSFM");

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.sinf.schm.scheme_type",
                            icPp->scheme_type);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.sinf.schm.scheme_version",
                            icPp->scheme_version);

    SetTrackStringProperty(trackId,
                           "mdia.minf.stbl.stsd.encv.sinf.schi.iKMS.kms_URI",
                           icPp->kms_uri);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.sinf.schi.iSFM.selective-encryption",
                            icPp->selective_enc);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.sinf.schi.iSFM.key-indicator-length",
                            icPp->key_ind_len);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.sinf.schi.iSFM.IV-length",
                            icPp->iv_len);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.esds.ESID",
                            0
                           );

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.esds.decConfigDescr.objectTypeId",
                            videoType);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.encv.esds.decConfigDescr.streamType",
                            MP4VisualStreamType);

    return trackId;
}

MP4TrackId MP4File::AddH264VideoTrack(
    uint32_t timeScale,
    MP4Duration sampleDuration,
    uint16_t width,
    uint16_t height,
    uint8_t AVCProfileIndication,
    uint8_t profile_compat,
    uint8_t AVCLevelIndication,
    uint8_t sampleLenFieldSizeMinusOne)
{
    MP4TrackId trackId = AddVideoTrackDefault(timeScale,
                         sampleDuration,
                         width,
                         height,
                         "avc1");

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.avc1.width", width);
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.avc1.height", height);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.avc1.avcC.AVCProfileIndication",
                            AVCProfileIndication);
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.avc1.avcC.profile_compatibility",
                            profile_compat);
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.avc1.avcC.AVCLevelIndication",
                            AVCLevelIndication);
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.avc1.avcC.lengthSizeMinusOne",
                            sampleLenFieldSizeMinusOne);

    return trackId;
}

MP4TrackId MP4File::AddEncH264VideoTrack(
    uint32_t timeScale,
    MP4Duration sampleDuration,
    uint16_t width,
    uint16_t height,
    MP4Atom *srcAtom,
    mp4v2_ismacrypParams *icPp)

{

    uint32_t original_fmt = 0;
    MP4Atom *avcCAtom;

    MP4TrackId trackId = AddVideoTrackDefault(timeScale,
                         sampleDuration,
                         width,
                         height,
                         "encv");

    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.encv.width", width);
    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.encv.height", height);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv"), "avcC");

    // create default values
    avcCAtom = FindAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.avcC"));

    // export source atom
    ((MP4AvcCAtom *) srcAtom)->Clone((MP4AvcCAtom *)avcCAtom);

    /* set all the ismacryp-specific values */

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.sinf"), "schm");
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.sinf"), "schi");
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.sinf.schi"), "iKMS");
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.sinf.schi"), "iSFM");

    // per ismacrypt E&A V1.1 section 9.1.2.1 'avc1' is renamed '264b'
    // avc1 must not appear as a sample entry name or original format name
    original_fmt = ATOMID("264b");
    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.encv.sinf.frma.data-format",
                            original_fmt);

    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.encv.sinf.schm.scheme_type",
                            icPp->scheme_type);

    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.encv.sinf.schm.scheme_version",
                            icPp->scheme_version);

    SetTrackStringProperty(trackId, "mdia.minf.stbl.stsd.encv.sinf.schi.iKMS.kms_URI",
                           icPp->kms_uri);

    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.encv.sinf.schi.iSFM.selective-encryption",
                            icPp->selective_enc);

    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.encv.sinf.schi.iSFM.key-indicator-length",
                            icPp->key_ind_len);

    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.encv.sinf.schi.iSFM.IV-length",
                            icPp->iv_len);


    return trackId;
}


void MP4File::AddH264SequenceParameterSet (MP4TrackId trackId,
        const uint8_t *pSequence,
        uint16_t sequenceLen)
{
    const char *format;
    MP4Atom *avcCAtom;

    // get 4cc media format - can be avc1 or encv for ismacrypted track
    format = GetTrackMediaDataName(trackId);

    if (!strcasecmp(format, "avc1"))
        avcCAtom = FindAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.avc1.avcC"));
    else if (!strcasecmp(format, "encv"))
        avcCAtom = FindAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.avcC"));
    else
        // huh?  unknown track format
        return;


    MP4BitfieldProperty *pCount;
    MP4Integer16Property *pLength;
    MP4BytesProperty *pUnit;
    if ((avcCAtom->FindProperty("avcC.numOfSequenceParameterSets",
                                (MP4Property **)&pCount) == false) ||
            (avcCAtom->FindProperty("avcC.sequenceEntries.sequenceParameterSetLength",
                                    (MP4Property **)&pLength) == false) ||
            (avcCAtom->FindProperty("avcC.sequenceEntries.sequenceParameterSetNALUnit",
                                    (MP4Property **)&pUnit) == false)) {
        log.errorf("%s: \"%s\": Could not find avcC properties",
                   __FUNCTION__, GetFilename().c_str() );
        return;
    }
    uint32_t count = pCount->GetValue();

    if (count > 0) {
        // see if we already exist
        for (uint32_t index = 0; index < count; index++) {
            if (pLength->GetValue(index) == sequenceLen) {
                uint8_t *seq;
                uint32_t seqlen;
                pUnit->GetValue(&seq, &seqlen, index);
                if (memcmp(seq, pSequence, sequenceLen) == 0) {
                    free(seq);
                    return;
                }
                free(seq);
            }
        }
    }
    pLength->AddValue(sequenceLen);
    pUnit->AddValue(pSequence, sequenceLen);
    pCount->IncrementValue();

    return;
}
void MP4File::AddH264PictureParameterSet (MP4TrackId trackId,
        const uint8_t *pPict,
        uint16_t pictLen)
{
    MP4Atom *avcCAtom =
        FindAtom(MakeTrackName(trackId,
                               "mdia.minf.stbl.stsd.avc1.avcC"));
    MP4Integer8Property *pCount;
    MP4Integer16Property *pLength;
    MP4BytesProperty *pUnit;
    if ((avcCAtom->FindProperty("avcC.numOfPictureParameterSets",
                                (MP4Property **)&pCount) == false) ||
            (avcCAtom->FindProperty("avcC.pictureEntries.pictureParameterSetLength",
                                    (MP4Property **)&pLength) == false) ||
            (avcCAtom->FindProperty("avcC.pictureEntries.pictureParameterSetNALUnit",
                                    (MP4Property **)&pUnit) == false)) {
        log.errorf("%s: \"%s\": Could not find avcC picture table properties",
                   __FUNCTION__, GetFilename().c_str());
        return;
    }

    ASSERT(pCount);
    uint32_t count = pCount->GetValue();

    if (count > 0) {
        // see if we already exist
        for (uint32_t index = 0; index < count; index++) {
            if (pLength->GetValue(index) == pictLen) {
                uint8_t *seq;
                uint32_t seqlen;
                pUnit->GetValue(&seq, &seqlen, index);
                if (memcmp(seq, pPict, pictLen) == 0) {
                    log.verbose1f("\"%s\": picture matches %d", 
                                  GetFilename().c_str(), index);
                    free(seq);
                    return;
                }
                free(seq);
            }
        }
    }
    pLength->AddValue(pictLen);
    pUnit->AddValue(pPict, pictLen);
    pCount->IncrementValue();
    log.verbose1f("\"%s\": new picture added %d", GetFilename().c_str(),
                  pCount->GetValue());

    return;
}
void  MP4File::SetH263Vendor(
    MP4TrackId trackId,
    uint32_t vendor)
{
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.d263.vendor",
                            vendor);
}

void MP4File::SetH263DecoderVersion(
    MP4TrackId trackId,
    uint8_t decoderVersion)
{
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.d263.decoderVersion",
                            decoderVersion);
}

void MP4File::SetH263Bitrates(
    MP4TrackId trackId,
    uint32_t avgBitrate,
    uint32_t maxBitrate)
{
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.d263.bitr.avgBitrate",
                            avgBitrate);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.d263.bitr.maxBitrate",
                            maxBitrate);

}

MP4TrackId MP4File::AddH263VideoTrack(
    uint32_t timeScale,
    MP4Duration sampleDuration,
    uint16_t width,
    uint16_t height,
    uint8_t h263Level,
    uint8_t h263Profile,
    uint32_t avgBitrate,
    uint32_t maxBitrate)

{
    MP4TrackId trackId = AddVideoTrackDefault(timeScale,
                         sampleDuration,
                         width,
                         height,
                         "s263");

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.width", width);
    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.height", height);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.d263.h263Level", h263Level);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.d263.h263Profile", h263Profile);

    // Add the bitr atom
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.s263.d263"),
                       "bitr");

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.d263.bitr.avgBitrate", avgBitrate);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.s263.d263.bitr.maxBitrate", maxBitrate);


    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsz.sampleSize", sampleDuration);

    return trackId;

}

MP4TrackId MP4File::AddHintTrack(MP4TrackId refTrackId)
{
    // validate reference track id
    (void)FindTrackIndex(refTrackId);

    MP4TrackId trackId =
        AddTrack(MP4_HINT_TRACK_TYPE, GetTrackTimeScale(refTrackId));

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "hmhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "rtp ");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the rtp atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.rtp .tims.timeScale",
                            GetTrackTimeScale(trackId));

    (void)AddDescendantAtoms(MakeTrackName(trackId, NULL), "tref.hint");

    AddTrackReference(MakeTrackName(trackId, "tref.hint"), refTrackId);

    (void)AddDescendantAtoms(MakeTrackName(trackId, NULL), "udta.hnti.sdp ");

    (void)AddDescendantAtoms(MakeTrackName(trackId, NULL), "udta.hinf");

    return trackId;
}

MP4TrackId MP4File::AddTextTrack(MP4TrackId refTrackId)
{
    // validate reference track id
    (void)FindTrackIndex(refTrackId);

    MP4TrackId trackId =
        AddTrack(MP4_TEXT_TRACK_TYPE, GetTrackTimeScale(refTrackId));

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "gmhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "text");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the text atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    return trackId;
}

MP4TrackId MP4File::AddSubtitleTrack(uint32_t timescale,
                                     uint16_t width,
                                     uint16_t height)
{
    MP4TrackId trackId =
        AddTrack(MP4_SUBTITLE_TRACK_TYPE, timescale);

    InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "nmhd", 0);

    AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "tx3g");

    SetTrackFloatProperty(trackId, "tkhd.width", width);
    SetTrackFloatProperty(trackId, "tkhd.height", height);

    // Hardcoded crap... add the ftab atom and add one font entry
    MP4Atom* pFtabAtom = AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.tx3g"), "ftab");

    ((MP4Integer16Property*)pFtabAtom->GetProperty(0))->IncrementValue();

    MP4Integer16Property* pfontID = (MP4Integer16Property*)((MP4TableProperty*)pFtabAtom->GetProperty(1))->GetProperty(0);
    pfontID->AddValue(1);

    MP4StringProperty* pName = (MP4StringProperty*)((MP4TableProperty*)pFtabAtom->GetProperty(1))->GetProperty(1);
    pName->AddValue("Arial");

    SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.tx3g.fontID", 1);

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the tx3g atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    return trackId;
}

MP4TrackId MP4File::AddSubpicTrack(uint32_t timescale,
                                     uint16_t width,
                                     uint16_t height)
{
    MP4TrackId trackId =
        AddTrack(MP4_SUBPIC_TRACK_TYPE, timescale);

    InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "nmhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "mp4s");

    SetTrackFloatProperty(trackId, "tkhd.width", width);
    SetTrackFloatProperty(trackId, "tkhd.height", height);
    SetTrackIntegerProperty(trackId, "tkhd.layer", 0);

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the mp4s atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4s.esds.ESID",
                            0
                           );

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr.objectTypeId",
                            MP4SubpicObjectType);

    SetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr.streamType",
                            MP4NeroSubpicStreamType);
    return trackId;
}

MP4TrackId MP4File::AddChapterTextTrack(MP4TrackId refTrackId, uint32_t timescale)
{
    // validate reference track id
    (void)FindTrackIndex(refTrackId);

    if (0 == timescale)
    {
        timescale = GetTrackTimeScale(refTrackId);
    }

    MP4TrackId trackId = AddTrack(MP4_TEXT_TRACK_TYPE, timescale);

    (void)InsertChildAtom(MakeTrackName(trackId, "mdia.minf"), "gmhd", 0);

    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "text");

    // stsd is a unique beast in that it has a count of the number
    // of child atoms that needs to be incremented after we add the text atom
    MP4Integer32Property* pStsdCountProperty;
    FindIntegerProperty(
        MakeTrackName(trackId, "mdia.minf.stbl.stsd.entryCount"),
        (MP4Property**)&pStsdCountProperty);
    pStsdCountProperty->IncrementValue();

    // add a "text" atom to the generic media header
    // this is different to the stsd "text" atom added above
    // truth be told, it's not clear what this second "text" atom does,
    // but all iTunes Store movies (with chapter markers) have it,
    // as do all movies with chapter tracks made by hand in QuickTime Pro
    (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.gmhd"), "text");

    // disable the chapter text track
    // it won't display anyway, as it has zero display size,
    // but nonetheless it's good to disable it
    // the track still operates as a chapter track when disabled
    MP4Atom *pTkhdAtom = FindAtom(MakeTrackName(trackId, "tkhd"));
    if (pTkhdAtom) {
        pTkhdAtom->SetFlags(0xE);
    }

    // add a "chapter" track reference to our reference track,
    // pointing to this new chapter track
    (void)AddDescendantAtoms(MakeTrackName(refTrackId, NULL), "tref.chap");
    AddTrackReference(MakeTrackName(refTrackId, "tref.chap"), trackId);

    return trackId;
}

MP4TrackId MP4File::AddPixelAspectRatio(MP4TrackId trackId, uint32_t hSpacing, uint32_t vSpacing)
{
    // validate reference track id
    (void)FindTrackIndex(trackId);
    const char *format = GetTrackMediaDataName (trackId);

    if (!strcasecmp(format, "avc1"))
    {
        (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.avc1"), "pasp");
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.avc1.pasp.hSpacing", hSpacing);
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.avc1.pasp.vSpacing", vSpacing);
    }
    else if (!strcasecmp(format, "mp4v"))
    {
        (void)AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.mp4v"), "pasp");
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.mp4v.pasp.hSpacing", hSpacing);
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.mp4v.pasp.vSpacing", vSpacing);
    }

    return trackId;
}

MP4TrackId MP4File::AddColr(MP4TrackId trackId,
                            uint16_t primariesIndex,
                            uint16_t transferFunctionIndex,
                            uint16_t matrixIndex)
{
    // validate reference track id
    (void)FindTrackIndex(trackId);
    const char *format = GetTrackMediaDataName (trackId);

    if (!strcasecmp(format, "avc1"))
    {
        AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.avc1"), "colr");
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.avc1.colr.primariesIndex", primariesIndex);
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.avc1.colr.transferFunctionIndex", transferFunctionIndex);
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.avc1.colr.matrixIndex", matrixIndex);
    }
    else if (!strcasecmp(format, "mp4v"))
    {
        AddChildAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.mp4v"), "colr");
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.mp4v.colr.primariesIndex", primariesIndex);
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.mp4v.colr.transferFunctionIndex", transferFunctionIndex);
        SetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsd.mp4v.colr.matrixIndex", matrixIndex);
    }

    return trackId;
}


void MP4File::AddChapter(MP4TrackId chapterTrackId, MP4Duration chapterDuration, const char *chapterTitle)
{
    if (MP4_INVALID_TRACK_ID == chapterTrackId)
    {
        throw new Exception("No chapter track given",__FILE__, __LINE__, __FUNCTION__);
    }

    uint32_t sampleLength = 0;
    uint8_t  sample[1040] = {0};
    int textLen = 0;
    char *text = (char *)&(sample[2]);

    if(chapterTitle != NULL)
    {
        textLen = min((uint32_t)strlen(chapterTitle), (uint32_t)MP4V2_CHAPTER_TITLE_MAX);
        if (0 < textLen)
        {
            strncpy(text, chapterTitle, textLen);
        }
    }
    else
    {
        MP4Track * pChapterTrack = GetTrack(chapterTrackId);
        snprintf( text, 1023, "Chapter %03d", pChapterTrack->GetNumberOfSamples() + 1 );
        textLen = (uint32_t)strlen(text);
    }

    sampleLength = textLen + 2 + 12; // Account for text length code and other marker

    // 2-byte length marker
    sample[0] = (textLen >> 8) & 0xff;
    sample[1] = textLen & 0xff;

    int x = 2 + textLen;

    // Modifier Length Marker
    sample[x] = 0x00;
    sample[x+1] = 0x00;
    sample[x+2] = 0x00;
    sample[x+3] = 0x0C;

    // Modifier Type Code
    sample[x+4] = 'e';
    sample[x+5] = 'n';
    sample[x+6] = 'c';
    sample[x+7] = 'd';

    // Modifier Value
    sample[x+8] = 0x00;
    sample[x+9] = 0x00;
    sample[x+10] = (256 >> 8) & 0xff;
    sample[x+11] = 256 & 0xff;

    WriteSample(chapterTrackId, sample, sampleLength, chapterDuration);
}

void MP4File::AddNeroChapter(MP4Timestamp chapterStart, const char * chapterTitle)
{
    MP4Atom * pChpl = FindAtom("moov.udta.chpl");
    if (!pChpl)
    {
        pChpl = AddDescendantAtoms("", "moov.udta.chpl");
    }

    MP4Integer32Property * pCount = (MP4Integer32Property*)pChpl->GetProperty(3);
    pCount->IncrementValue();

    char buffer[256];

    if (0 == chapterTitle)
    {
        snprintf( buffer, 255, "Chapter %03d", pCount->GetValue() );
    }
    else
    {
        int len = min((uint32_t)strlen(chapterTitle), (uint32_t)255);
        strncpy( buffer, chapterTitle, len );
        buffer[len] = 0;
    }

    MP4TableProperty * pTable;
    if (pChpl->FindProperty("chpl.chapters", (MP4Property **)&pTable))
    {
        MP4Integer64Property * pStartTime = (MP4Integer64Property *) pTable->GetProperty(0);
        MP4StringProperty * pName = (MP4StringProperty *) pTable->GetProperty(1);
        if (pStartTime && pTable)
        {
            pStartTime->AddValue(chapterStart);
            pName->AddValue(buffer);
        }
    }
}

MP4TrackId MP4File::FindChapterReferenceTrack(MP4TrackId chapterTrackId, char * trackName, int trackNameSize)
{
    for (uint32_t i = 0; i < m_pTracks.Size(); i++)
    {
        if( MP4_IS_VIDEO_TRACK_TYPE( m_pTracks[i]->GetType() ) ||
            MP4_IS_AUDIO_TRACK_TYPE( m_pTracks[i]->GetType() ) )
        {
            MP4TrackId refTrackId = m_pTracks[i]->GetId();
            char *name = MakeTrackName(refTrackId, "tref.chap");
            if( FindTrackReference( name, chapterTrackId ) )
            {
                if( 0 != trackName )
                {
                    int nameLen = min((uint32_t)strlen(name), (uint32_t)trackNameSize);
                    strncpy(trackName, name, nameLen);
                    trackName[nameLen] = 0;
                }

                return m_pTracks[i]->GetId();
            }
        }
    }

    return MP4_INVALID_TRACK_ID;
}

MP4TrackId MP4File::FindChapterTrack(char * trackName, int trackNameSize)
{
    for (uint32_t i = 0; i < m_pTracks.Size(); i++)
    {
        if( !strcasecmp(MP4_TEXT_TRACK_TYPE, m_pTracks[i]->GetType()) )
        {
            MP4TrackId refTrackId = FindChapterReferenceTrack(m_pTracks[i]->GetId(), trackName, trackNameSize);
            if (MP4_INVALID_TRACK_ID != refTrackId)
            {
                return m_pTracks[i]->GetId();
            }
        }
    }

    return MP4_INVALID_TRACK_ID;
}

MP4ChapterType MP4File::DeleteChapters(MP4ChapterType chapterType, MP4TrackId chapterTrackId)
{
    MP4ChapterType deletedType = MP4ChapterTypeNone;

    if (MP4ChapterTypeAny == chapterType || MP4ChapterTypeNero == chapterType)
    {
        MP4Atom * pChpl = FindAtom("moov.udta.chpl");
        if (pChpl)
        {
            MP4Atom * pParent = pChpl->GetParentAtom();
            pParent->DeleteChildAtom(pChpl);
            deletedType = MP4ChapterTypeNero;
        }
    }

    if (MP4ChapterTypeAny == chapterType || MP4ChapterTypeQt == chapterType)
    {
        char trackName[128] = {0};

        // no text track given, find a suitable
        if (MP4_INVALID_TRACK_ID == chapterTrackId)
        {
            chapterTrackId = FindChapterTrack(trackName, 127);
        }

        if (MP4_INVALID_TRACK_ID != chapterTrackId)
        {
            FindChapterReferenceTrack(chapterTrackId, trackName, 127);
        }

        if (MP4_INVALID_TRACK_ID != chapterTrackId && 0 != trackName[0])
        {
            // remove the reference
            MP4Atom * pChap = FindAtom( trackName );
            if( pChap )
            {
                MP4Atom * pTref = pChap->GetParentAtom();
                if( pTref )
                {
                    pTref->DeleteChildAtom( pChap );

                    MP4Atom* pParent = pTref->GetParentAtom();
                    pParent->DeleteChildAtom( pTref );
                }
            }

            // remove the chapter track
            DeleteTrack(chapterTrackId);
            deletedType = MP4ChapterTypeNone == deletedType ? MP4ChapterTypeQt : MP4ChapterTypeAny;
        }
    }
    return deletedType;
}

MP4ChapterType MP4File::GetChapters(MP4Chapter_t ** chapterList, uint32_t * chapterCount, MP4ChapterType fromChapterType)
{
    *chapterList = 0;
    *chapterCount = 0;

    if (MP4ChapterTypeAny == fromChapterType || MP4ChapterTypeQt == fromChapterType)
    {
        uint8_t * sample = 0;
        uint32_t sampleSize = 0;
        MP4Timestamp startTime = 0;
        MP4Duration duration = 0;

        // get the chapter track
        MP4TrackId chapterTrackId = FindChapterTrack();
        if (MP4_INVALID_TRACK_ID == chapterTrackId)
        {
            if (MP4ChapterTypeQt == fromChapterType)
            {
                return MP4ChapterTypeNone;
            }
        }
        else
        {
            // get infos about the chapters
            MP4Track * pChapterTrack = GetTrack(chapterTrackId);
            uint32_t counter = pChapterTrack->GetNumberOfSamples();

            if (0 < counter)
            {
                uint32_t timescale = pChapterTrack->GetTimeScale();
                MP4Chapter_t * chapters = (MP4Chapter_t*)MP4Malloc(sizeof(MP4Chapter_t) * counter);

                // process all chapter sample
                for (uint32_t i = 0; i < counter; ++i)
                {
                    // get the sample corresponding to the starttime
                    MP4SampleId sampleId = pChapterTrack->GetSampleIdFromTime(startTime + duration, true);
                    pChapterTrack->ReadSample(sampleId, &sample, &sampleSize);

                    // get the starttime and duration
                    pChapterTrack->GetSampleTimes(sampleId, &startTime, &duration);

                    // we know that sample+2 contains the title (sample[0] and sample[1] is the length)
                    const char * title = (const char *)&(sample[2]);
                    int titleLen = min((uint32_t)((sample[0] << 8) | sample[1]), (uint32_t)MP4V2_CHAPTER_TITLE_MAX);
                    strncpy(chapters[i].title, title, titleLen);
                    chapters[i].title[titleLen] = 0;

                    // write the duration (in milliseconds)
                    chapters[i].duration = MP4ConvertTime(duration, timescale, MP4_MILLISECONDS_TIME_SCALE);

                    // we're done with this sample
                    MP4Free(sample);
                    sample = 0;
                }

                *chapterList = chapters;
                *chapterCount = counter;

                // we got chapters so we are done
                return MP4ChapterTypeQt;
            }
        }
    }

    if (MP4ChapterTypeAny == fromChapterType || MP4ChapterTypeNero == fromChapterType)
    {
        MP4Atom * pChpl = FindAtom("moov.udta.chpl");
        if (!pChpl)
        {
            return MP4ChapterTypeNone;
        }

        MP4Integer32Property * pCounter = 0;
        if (!pChpl->FindProperty("chpl.chaptercount", (MP4Property **)&pCounter))
        {
            log.warningf("%s: \"%s\": Nero chapter count does not exist",
                         __FUNCTION__, GetFilename().c_str());
            return MP4ChapterTypeNone;
        }

        uint32_t counter = pCounter->GetValue();
        if (0 == counter)
        {
            log.warningf("%s: \"%s\": No Nero chapters available",
                         __FUNCTION__, GetFilename().c_str());
            return MP4ChapterTypeNone;
        }

        MP4TableProperty * pTable = 0;
        MP4Integer64Property * pStartTime = 0;
        MP4StringProperty * pName = 0;
        MP4Duration chapterDurationSum = 0;
        const char * name = 0;

        if (!pChpl->FindProperty("chpl.chapters", (MP4Property **)&pTable))
        {
            log.warningf("%s: \"%s\": Nero chapter list does not exist",
                         __FUNCTION__, GetFilename().c_str());
            return MP4ChapterTypeNone;
        }

        if (0 == (pStartTime = (MP4Integer64Property *) pTable->GetProperty(0)))
        {
            log.warningf("%s: \"%s\": List of Chapter starttimes does not exist",
                         __FUNCTION__, GetFilename().c_str());
            return MP4ChapterTypeNone;
        }
        if (0 == (pName = (MP4StringProperty *) pTable->GetProperty(1)))
        {
            log.warningf("%s: \"%s\": List of Chapter titles does not exist",
                         __FUNCTION__, GetFilename().c_str());
            return MP4ChapterTypeNone;
        }

        MP4Chapter_t * chapters = (MP4Chapter_t*)MP4Malloc(sizeof(MP4Chapter_t) * counter);

        // get the name of the first chapter
        name = pName->GetValue();

        // process remaining chapters
        uint32_t i, j;
        for (i = 0, j = 1; i < counter; ++i, ++j)
        {
            // insert the chapter title
            uint32_t len = min((uint32_t)strlen(name), (uint32_t)MP4V2_CHAPTER_TITLE_MAX);
            strncpy(chapters[i].title, name, len);
            chapters[i].title[len] = 0;

            // calculate the duration
            MP4Duration duration = 0;
            if (j < counter)
            {
                duration = MP4ConvertTime(pStartTime->GetValue(j),
                                          (MP4_NANOSECONDS_TIME_SCALE / 100),
                                          MP4_MILLISECONDS_TIME_SCALE) - chapterDurationSum;

                // now get the name of the chapter (to be written next)
                name = pName->GetValue(j);
            }
            else
            {
                // last chapter
                duration = MP4ConvertTime(GetDuration(), GetTimeScale(), MP4_MILLISECONDS_TIME_SCALE) - chapterDurationSum;
            }

            // sum up the chapter duration
            chapterDurationSum += duration;

            // insert the chapter duration
            chapters[i].duration = duration;
        }

        *chapterList = chapters;
        *chapterCount = counter;

        return MP4ChapterTypeNero;
    }

    return MP4ChapterTypeNone;
}

MP4ChapterType MP4File::SetChapters(MP4Chapter_t * chapterList, uint32_t chapterCount, MP4ChapterType toChapterType)
{
    MP4ChapterType setType = MP4ChapterTypeNone;

    // first remove any existing chapters
    DeleteChapters(toChapterType, MP4_INVALID_TRACK_ID);

    if( MP4ChapterTypeAny == toChapterType || MP4ChapterTypeNero == toChapterType )
    {
        MP4Duration duration = 0;
        for( uint32_t i = 0; i < chapterCount; ++i )
        {
            AddNeroChapter(duration, chapterList[i].title);
            duration += 10 * MP4_MILLISECONDS_TIME_SCALE * chapterList[i].duration;
        }

        setType = MP4ChapterTypeNero;
    }

    if (MP4ChapterTypeAny == toChapterType || MP4ChapterTypeQt == toChapterType)
    {
        // find the first video or audio track
        MP4TrackId refTrack = MP4_INVALID_TRACK_ID;
        for( uint32_t i = 0; i < m_pTracks.Size(); i++ )
        {
            if( MP4_IS_VIDEO_TRACK_TYPE( m_pTracks[i]->GetType() ) ||
                MP4_IS_AUDIO_TRACK_TYPE( m_pTracks[i]->GetType() ) )
            {
                refTrack = m_pTracks[i]->GetId();
                break;
            }
        }

        if( refTrack == MP4_INVALID_TRACK_ID )
        {
            return setType;
        }

        // create the chapter track
        MP4TrackId chapterTrack = AddChapterTextTrack(refTrack, MP4_MILLISECONDS_TIME_SCALE);

        for( uint32_t i = 0 ; i < chapterCount; ++i )
        {
            // create and write the chapter track sample
            AddChapter( chapterTrack, chapterList[i].duration, chapterList[i].title );
        }

        setType = MP4ChapterTypeNone == setType ? MP4ChapterTypeQt : MP4ChapterTypeAny;
    }

    return setType;
}

MP4ChapterType MP4File::ConvertChapters(MP4ChapterType toChapterType)
{
    MP4ChapterType sourceType = MP4ChapterTypeNone;
    const char* errMsg = 0;

    if( MP4ChapterTypeQt == toChapterType )
    {
        sourceType = MP4ChapterTypeNero;
        errMsg = "Could not find Nero chapter markers";
    }
    else if( MP4ChapterTypeNero == toChapterType )
    {
        sourceType = MP4ChapterTypeQt;
        errMsg = "Could not find QuickTime chapter markers";
    }
    else
    {
        return MP4ChapterTypeNone;
    }

    MP4Chapter_t * chapters = 0;
    uint32_t chapterCount = 0;

    GetChapters(&chapters, &chapterCount, sourceType);
    if (0 == chapterCount)
    {
        log.warningf("%s: \"%s\": %s", __FUNCTION__, GetFilename().c_str(),
                     errMsg);
        return MP4ChapterTypeNone;
    }

    SetChapters(chapters, chapterCount, toChapterType);

    MP4Free(chapters);

    return toChapterType;
}

void MP4File::ChangeMovieTimeScale(uint32_t timescale)
{
    uint32_t origTimeScale = GetTimeScale();
    if (timescale == origTimeScale) {
        // already done
        return;
    }

    MP4Duration movieDuration = GetDuration();

    // set movie header timescale and duration
    SetTimeScale(timescale);
    SetDuration(MP4ConvertTime(movieDuration, origTimeScale, timescale));

    // set track header duration (calculated with movie header timescale)
    uint32_t trackCount = GetNumberOfTracks();
    for (uint32_t i = 0; i < trackCount; ++i)
    {
        MP4Track * track = GetTrack(FindTrackId(i));
        MP4Atom & trackAtom = track->GetTrakAtom();
        MP4IntegerProperty * duration;

        if (trackAtom.FindProperty("trak.tkhd.duration", (MP4Property**)&duration))
        {
            duration->SetValue(MP4ConvertTime(duration->GetValue(), origTimeScale, timescale));
        }
    }
}

void MP4File::DeleteTrack(MP4TrackId trackId)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    uint32_t trakIndex = FindTrakAtomIndex(trackId);
    uint16_t trackIndex = FindTrackIndex(trackId);
    MP4Track* pTrack = m_pTracks[trackIndex];

    MP4Atom& trakAtom = pTrack->GetTrakAtom();

    MP4Atom* pMoovAtom = FindAtom("moov");
    ASSERT(pMoovAtom);

    RemoveTrackFromIod(trackId, ShallHaveIods());
    RemoveTrackFromOd(trackId);

    if (trackId == m_odTrackId) {
        m_odTrackId = 0;
    }

    pMoovAtom->DeleteChildAtom(&trakAtom);

    m_trakIds.Delete(trakIndex);

    m_pTracks.Delete(trackIndex);

    delete pTrack;
    delete &trakAtom;
}

uint32_t MP4File::GetNumberOfTracks(const char* type, uint8_t subType)
{
    if (type == NULL) {
        return m_pTracks.Size();
    }

    uint32_t typeSeen = 0;
    const char* normType = MP4NormalizeTrackType(type);

    for (uint32_t i = 0; i < m_pTracks.Size(); i++) {
        if (!strcmp(normType, m_pTracks[i]->GetType())) {
            if (subType) {
                if (strcmp(normType, MP4_AUDIO_TRACK_TYPE) == 0) {
                    if (subType != GetTrackEsdsObjectTypeId(m_pTracks[i]->GetId())) {
                        continue;
                    }
                } else if (strcmp(normType, MP4_VIDEO_TRACK_TYPE)) {
                    if (subType != GetTrackEsdsObjectTypeId(m_pTracks[i]->GetId())) {
                        continue;
                    }
                }
                // else unknown subtype, ignore it
            }
            typeSeen++;
        }
    }
    return typeSeen;
}

MP4TrackId MP4File::AllocTrackId()
{
    MP4TrackId trackId =
        GetIntegerProperty("moov.mvhd.nextTrackId");

    if (trackId <= 0xFFFF) {
        // check that nextTrackid is correct
        try {
            (void)FindTrackIndex(trackId);
            // ERROR, this trackId is in use
        }
        catch (Exception* x) {
            // OK, this trackId is not in use, proceed
            delete x;
            SetIntegerProperty("moov.mvhd.nextTrackId", trackId + 1);
            return trackId;
        }
    }

    // we need to search for a track id
    for (trackId = 1; trackId <= 0xFFFF; trackId++) {
        try {
            (void)FindTrackIndex(trackId);
            // KEEP LOOKING, this trackId is in use
        }
        catch (Exception* x) {
            // OK, this trackId is not in use, proceed
            delete x;
            return trackId;
        }
    }

    // extreme case where mp4 file has 2^16 tracks in it
    throw new Exception("too many existing tracks", __FILE__, __LINE__, __FUNCTION__);
    return MP4_INVALID_TRACK_ID;        // to keep MSVC happy
}

MP4TrackId MP4File::FindTrackId(uint16_t trackIndex,
                                const char* type, uint8_t subType)
{
    if (type == NULL) {
        return m_pTracks[trackIndex]->GetId();
    }

    uint32_t typeSeen = 0;
    const char* normType = MP4NormalizeTrackType(type);

    for (uint32_t i = 0; i < m_pTracks.Size(); i++) {
        if (!strcmp(normType, m_pTracks[i]->GetType())) {
            if (subType) {
                if (strcmp(normType, MP4_AUDIO_TRACK_TYPE) == 0) {
                    if (subType != GetTrackEsdsObjectTypeId(m_pTracks[i]->GetId())) {
                        continue;
                    }
                } else if (strcmp(normType, MP4_VIDEO_TRACK_TYPE) == 0) {
                    if (subType != GetTrackEsdsObjectTypeId(m_pTracks[i]->GetId())) {
                        continue;
                    }
                }
                // else unknown subtype, ignore it
            }

            if (trackIndex == typeSeen) {
                return m_pTracks[i]->GetId();
            }

            typeSeen++;
        }
    }

    ostringstream msg;
    msg << "Track index doesn't exist - track " << trackIndex << " type " << type;
    throw new Exception(msg.str(),__FILE__, __LINE__, __FUNCTION__);
    return MP4_INVALID_TRACK_ID; // satisfy MS compiler
}

uint16_t MP4File::FindTrackIndex(MP4TrackId trackId)
{
    for (uint32_t i = 0; i < m_pTracks.Size() && i <= 0xFFFF; i++) {
        if (m_pTracks[i]->GetId() == trackId) {
            return (uint16_t)i;
        }
    }

    ostringstream msg;
    msg << "Track id " << trackId << " doesn't exist";
    throw new Exception(msg.str(),__FILE__, __LINE__, __FUNCTION__);
    return (uint16_t)-1; // satisfy MS compiler
}

uint16_t MP4File::FindTrakAtomIndex(MP4TrackId trackId)
{
    if (trackId) {
        for (uint32_t i = 0; i < m_trakIds.Size(); i++) {
            if (m_trakIds[i] == trackId) {
                return i;
            }
        }
    }

    ostringstream msg;
    msg << "Track id " << trackId << " doesn't exist";
    throw new Exception(msg.str(),__FILE__, __LINE__, __FUNCTION__);
    return (uint16_t)-1; // satisfy MS compiler
}

uint32_t MP4File::GetSampleSize(MP4TrackId trackId, MP4SampleId sampleId)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetSampleSize(sampleId);
}

uint32_t MP4File::GetTrackMaxSampleSize(MP4TrackId trackId)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetMaxSampleSize();
}

MP4SampleId MP4File::GetSampleIdFromTime(MP4TrackId trackId,
        MP4Timestamp when, bool wantSyncSample)
{
    return m_pTracks[FindTrackIndex(trackId)]->
           GetSampleIdFromTime(when, wantSyncSample);
}

MP4Timestamp MP4File::GetSampleTime(
    MP4TrackId trackId, MP4SampleId sampleId)
{
    MP4Timestamp timestamp;
    m_pTracks[FindTrackIndex(trackId)]->
    GetSampleTimes(sampleId, &timestamp, NULL);
    return timestamp;
}

MP4Duration MP4File::GetSampleDuration(
    MP4TrackId trackId, MP4SampleId sampleId)
{
    MP4Duration duration;
    m_pTracks[FindTrackIndex(trackId)]->
    GetSampleTimes(sampleId, NULL, &duration);
    return duration;
}

MP4Duration MP4File::GetSampleRenderingOffset(
    MP4TrackId trackId, MP4SampleId sampleId)
{
    return m_pTracks[FindTrackIndex(trackId)]->
           GetSampleRenderingOffset(sampleId);
}

bool MP4File::GetSampleSync(MP4TrackId trackId, MP4SampleId sampleId)
{
    return m_pTracks[FindTrackIndex(trackId)]->IsSyncSample(sampleId);
}

void MP4File::ReadSample(
    MP4TrackId    trackId,
    MP4SampleId   sampleId,
    uint8_t**     ppBytes,
    uint32_t*     pNumBytes,
    MP4Timestamp* pStartTime,
    MP4Duration*  pDuration,
    MP4Duration*  pRenderingOffset,
    bool*         pIsSyncSample,
    bool*         hasDependencyFlags,
    uint32_t*     dependencyFlags )
{
    m_pTracks[FindTrackIndex(trackId)]->ReadSample(
        sampleId,
        ppBytes,
        pNumBytes,
        pStartTime,
        pDuration,
        pRenderingOffset,
        pIsSyncSample,
        hasDependencyFlags,
        dependencyFlags );
}

void MP4File::WriteSample(
    MP4TrackId     trackId,
    const uint8_t* pBytes,
    uint32_t       numBytes,
    MP4Duration    duration,
    MP4Duration    renderingOffset,
    bool           isSyncSample )
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);
    m_pTracks[FindTrackIndex(trackId)]->WriteSample(
        pBytes, numBytes, duration, renderingOffset, isSyncSample );
    m_pModificationProperty->SetValue( MP4GetAbsTimestamp() );
}

void MP4File::WriteSampleDependency(
    MP4TrackId     trackId,
    const uint8_t* pBytes, 
    uint32_t       numBytes,
    MP4Duration    duration,
    MP4Duration    renderingOffset,
    bool           isSyncSample,
    uint32_t       dependencyFlags )
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);
    m_pTracks[FindTrackIndex(trackId)]->WriteSampleDependency(
        pBytes, numBytes, duration, renderingOffset, isSyncSample, dependencyFlags );
    m_pModificationProperty->SetValue( MP4GetAbsTimestamp() );
}

void MP4File::SetSampleRenderingOffset(MP4TrackId trackId,
                                       MP4SampleId sampleId, MP4Duration renderingOffset)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);
    m_pTracks[FindTrackIndex(trackId)]->
    SetSampleRenderingOffset(sampleId, renderingOffset);

    m_pModificationProperty->SetValue(MP4GetAbsTimestamp());
}

void MP4File::MakeFtypAtom(
    char*    majorBrand,
    uint32_t minorVersion,
    char**   compatibleBrands,
    uint32_t compatibleBrandsCount)
{
    MP4FtypAtom* ftyp = (MP4FtypAtom*)m_pRootAtom->FindAtom( "ftyp" );
    if (ftyp == NULL)
        ftyp = (MP4FtypAtom*)InsertChildAtom( m_pRootAtom, "ftyp", 0 );

    // bail if majorbrand is not specified; defaults suffice.
    if (majorBrand == NULL)
        return;

    ftyp->majorBrand.SetValue( majorBrand );
    ftyp->minorVersion.SetValue( minorVersion );

    ftyp->compatibleBrands.SetCount( compatibleBrandsCount );
    for( uint32_t i = 0; i < compatibleBrandsCount; i++ )
        ftyp->compatibleBrands.SetValue( compatibleBrands[i], i );
}

char* MP4File::MakeTrackName(MP4TrackId trackId, const char* name)
{
    uint16_t trakIndex = FindTrakAtomIndex(trackId);

    if (name == NULL || name[0] == '\0') {
        snprintf(m_trakName, sizeof(m_trakName),
                 "moov.trak[%u]", trakIndex);
    } else {
        snprintf(m_trakName, sizeof(m_trakName),
                 "moov.trak[%u].%s", trakIndex, name);
    }
    return m_trakName;
}

MP4Atom *MP4File::FindTrackAtom (MP4TrackId trackId, const char *name)
{
    return FindAtom(MakeTrackName(trackId, name));
}

uint64_t MP4File::GetTrackIntegerProperty(MP4TrackId trackId, const char* name)
{
    return GetIntegerProperty(MakeTrackName(trackId, name));
}

void MP4File::SetTrackIntegerProperty(MP4TrackId trackId, const char* name,
                                      int64_t value)
{
    SetIntegerProperty(MakeTrackName(trackId, name), value);
}

float MP4File::GetTrackFloatProperty(MP4TrackId trackId, const char* name)
{
    return GetFloatProperty(MakeTrackName(trackId, name));
}

void MP4File::SetTrackFloatProperty(MP4TrackId trackId, const char* name,
                                    float value)
{
    SetFloatProperty(MakeTrackName(trackId, name), value);
}

const char* MP4File::GetTrackStringProperty(MP4TrackId trackId, const char* name)
{
    return GetStringProperty(MakeTrackName(trackId, name));
}

void MP4File::SetTrackStringProperty(MP4TrackId trackId, const char* name,
                                     const char* value)
{
    SetStringProperty(MakeTrackName(trackId, name), value);
}

void MP4File::GetTrackBytesProperty(MP4TrackId trackId, const char* name,
                                    uint8_t** ppValue, uint32_t* pValueSize)
{
    GetBytesProperty(MakeTrackName(trackId, name), ppValue, pValueSize);
}

void MP4File::SetTrackBytesProperty(MP4TrackId trackId, const char* name,
                                    const uint8_t* pValue, uint32_t valueSize)
{
    SetBytesProperty(MakeTrackName(trackId, name), pValue, valueSize);
}

bool MP4File::GetTrackLanguage( MP4TrackId trackId, char* code )
{
    ostringstream oss;
    oss << "moov.trak[" << FindTrakAtomIndex(trackId) << "].mdia.mdhd.language";

    MP4Property* prop;
    if( !m_pRootAtom->FindProperty( oss.str().c_str(), &prop ))
        return false;

    if( prop->GetType() != LanguageCodeProperty )
        return false;

    MP4LanguageCodeProperty& lang = *static_cast<MP4LanguageCodeProperty*>(prop);
    string slang;
    bmff::enumLanguageCode.toString( lang.GetValue(), slang );
    if( slang.length() != 3 ) {
        memset( code, '\0', 4 );
    }
    else {
        memcpy( code, slang.c_str(), 3 );
        code[3] = '\0';
    }

    return true;
}

bool MP4File::SetTrackLanguage( MP4TrackId trackId, const char* code )
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    ostringstream oss;
    oss << "moov.trak[" << FindTrakAtomIndex(trackId) << "].mdia.mdhd.language";

    MP4Property* prop;
    if( !m_pRootAtom->FindProperty( oss.str().c_str(), &prop ))
        return false;

    if( prop->GetType() != LanguageCodeProperty )
        return false;

    MP4LanguageCodeProperty& lang = *static_cast<MP4LanguageCodeProperty*>(prop);
    lang.SetValue( bmff::enumLanguageCode.toType( code ));

    return true;
}

    
bool MP4File::GetTrackName( MP4TrackId trackId, char** name )
{
    unsigned char *val = NULL;
    uint32_t valSize = 0;
    MP4Atom *pMetaAtom;

    pMetaAtom = m_pRootAtom->FindAtom(MakeTrackName(trackId,"udta.name"));

    if (pMetaAtom)
    {
        GetBytesProperty(MakeTrackName(trackId,"udta.name.value"), (uint8_t**)&val, &valSize);
    }
    if (valSize > 0)
    {
        *name = (char*)malloc((valSize+1)*sizeof(char));
        if (*name == NULL) {
            free(val);
            return false;
        }
        memcpy(*name, val, valSize*sizeof(unsigned char));
        free(val);
        (*name)[valSize] = '\0';
        return true;
    }

    return false;
}

bool MP4File::SetTrackName( MP4TrackId trackId, const char* name )
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);
    char atomstring[40];
    MP4Atom *pMetaAtom;
    MP4BytesProperty *pMetadataProperty = NULL;
    snprintf(atomstring, 40, "%s", MakeTrackName(trackId,"udta.name"));

    pMetaAtom = m_pRootAtom->FindAtom(atomstring);

    if (!pMetaAtom)
    {
        if (!AddDescendantAtoms(MakeTrackName(trackId, NULL), "udta.name"))
            return false;
        
        pMetaAtom = m_pRootAtom->FindAtom(atomstring);
        if (pMetaAtom == NULL) return false;
    }

    ASSERT(pMetaAtom->FindProperty("name.value",
                                   (MP4Property**)&pMetadataProperty));
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((uint8_t*)name, (uint32_t)strlen(name));

    return true;
}

// file level convenience functions

MP4Duration MP4File::GetDuration()
{
    return m_pDurationProperty->GetValue();
}

void MP4File::SetDuration(MP4Duration value)
{
    m_pDurationProperty->SetValue(value);
}

uint32_t MP4File::GetTimeScale()
{
    return m_pTimeScaleProperty->GetValue();
}

void MP4File::SetTimeScale(uint32_t value)
{
    if (value == 0) {
        throw new Exception("invalid value", __FILE__, __LINE__, __FUNCTION__);
    }
    m_pTimeScaleProperty->SetValue(value);
}

uint8_t MP4File::GetODProfileLevel()
{
    return GetIntegerProperty("moov.iods.ODProfileLevelId");
}

void MP4File::SetODProfileLevel(uint8_t value)
{
    SetIntegerProperty("moov.iods.ODProfileLevelId", value);
}

uint8_t MP4File::GetSceneProfileLevel()
{
    return GetIntegerProperty("moov.iods.sceneProfileLevelId");
}

void MP4File::SetSceneProfileLevel(uint8_t value)
{
    SetIntegerProperty("moov.iods.sceneProfileLevelId", value);
}

uint8_t MP4File::GetVideoProfileLevel()
{
    return GetIntegerProperty("moov.iods.visualProfileLevelId");
}

void MP4File::SetVideoProfileLevel(uint8_t value)
{
    SetIntegerProperty("moov.iods.visualProfileLevelId", value);
}

uint8_t MP4File::GetAudioProfileLevel()
{
    return GetIntegerProperty("moov.iods.audioProfileLevelId");
}

void MP4File::SetAudioProfileLevel(uint8_t value)
{
    SetIntegerProperty("moov.iods.audioProfileLevelId", value);
}

uint8_t MP4File::GetGraphicsProfileLevel()
{
    return GetIntegerProperty("moov.iods.graphicsProfileLevelId");
}

void MP4File::SetGraphicsProfileLevel(uint8_t value)
{
    SetIntegerProperty("moov.iods.graphicsProfileLevelId", value);
}

const char* MP4File::GetSessionSdp()
{
    return GetStringProperty("moov.udta.hnti.rtp .sdpText");
}

void MP4File::SetSessionSdp(const char* sdpString)
{
    (void)AddDescendantAtoms("moov", "udta.hnti.rtp ");

    SetStringProperty("moov.udta.hnti.rtp .sdpText", sdpString);
}

void MP4File::AppendSessionSdp(const char* sdpFragment)
{
    const char* oldSdpString = NULL;
    try {
        oldSdpString = GetSessionSdp();
    }
    catch (Exception* x) {
        delete x;
        SetSessionSdp(sdpFragment);
        return;
    }

    char* newSdpString =
        (char*)MP4Malloc(strlen(oldSdpString) + strlen(sdpFragment) + 1);
    strcpy(newSdpString, oldSdpString);
    strcat(newSdpString, sdpFragment);
    SetSessionSdp(newSdpString);
    MP4Free(newSdpString);
}

//
// ismacrypt API - retrieve OriginalFormatBox
//
// parameters are assumed to have been sanity tested in mp4.cpp
// don't call this unless media data name is 'encv',
// results may otherwise be unpredictable.
//
// input:
// trackID - valid encv track ID for this file
// buflen  - length of oFormat, minimum is 5 (4cc plus null terminator)
//
// output:
// oFormat - buffer to return null terminated string containing
//           track original format
// return:
// 0       - original format returned OK
// 1       - buffer length error or problem retrieving track property
//
//
bool MP4File::GetTrackMediaDataOriginalFormat(MP4TrackId trackId,
        char *originalFormat, uint32_t buflen)
{
    uint32_t format;

    if (buflen < 5)
        return false;

    format = GetTrackIntegerProperty(trackId,
                                     "mdia.minf.stbl.stsd.*.sinf.frma.data-format");

    IDATOM(format, originalFormat);
    return true;

}


// track level convenience functions

MP4SampleId MP4File::GetTrackNumberOfSamples(MP4TrackId trackId)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetNumberOfSamples();
}

const char* MP4File::GetTrackType(MP4TrackId trackId)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetType();
}

const char *MP4File::GetTrackMediaDataName (MP4TrackId trackId)
{
    MP4Atom *pChild;
    MP4Atom *pAtom =
        FindAtom(MakeTrackName(trackId,
                               "mdia.minf.stbl.stsd"));
    if (pAtom->GetNumberOfChildAtoms() != 1) {
        log.errorf("%s: \"%s\": track %d has more than 1 child atoms in stsd", 
                   __FUNCTION__, GetFilename().c_str(), trackId);
        return NULL;
    }
    pChild = pAtom->GetChildAtom(0);
    return pChild->GetType();
}


uint32_t MP4File::GetTrackTimeScale(MP4TrackId trackId)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetTimeScale();
}

void MP4File::SetTrackTimeScale(MP4TrackId trackId, uint32_t value)
{
    if (value == 0) {
        throw new Exception("invalid value", __FILE__, __LINE__, __FUNCTION__);
    }
    SetTrackIntegerProperty(trackId, "mdia.mdhd.timeScale", value);
}

MP4Duration MP4File::GetTrackDuration(MP4TrackId trackId)
{
    return GetTrackIntegerProperty(trackId, "mdia.mdhd.duration");
}

uint8_t MP4File::GetTrackEsdsObjectTypeId(MP4TrackId trackId)
{
    // changed mp4a to * to handle enca case
    try {
        return GetTrackIntegerProperty(trackId,
                                       "mdia.minf.stbl.stsd.*.esds.decConfigDescr.objectTypeId");
    } catch (Exception *x) {
        delete x;
        return GetTrackIntegerProperty(trackId,
                                       "mdia.minf.stbl.stsd.*.*.esds.decConfigDescr.objectTypeId");
    }
}

uint8_t MP4File::GetTrackAudioMpeg4Type(MP4TrackId trackId)
{
    // verify that track is an MPEG-4 audio track
    if (GetTrackEsdsObjectTypeId(trackId) != MP4_MPEG4_AUDIO_TYPE) {
        return MP4_MPEG4_INVALID_AUDIO_TYPE;
    }

    uint8_t* pEsConfig = NULL;
    uint32_t esConfigSize;

    // The Mpeg4 audio type (AAC, CELP, HXVC, ...)
    // is the first 5 bits of the ES configuration

    GetTrackESConfiguration(trackId, &pEsConfig, &esConfigSize);

    if (esConfigSize < 1) {
        free(pEsConfig);
        return MP4_MPEG4_INVALID_AUDIO_TYPE;
    }

    uint8_t mpeg4Type = ((pEsConfig[0] >> 3) & 0x1f);
    // TTTT TXXX XXX  potentially 6 bits of extension.
    if (mpeg4Type == 0x1f) {
        if (esConfigSize < 2) {
            free(pEsConfig);
            return MP4_MPEG4_INVALID_AUDIO_TYPE;
        }
        mpeg4Type = 32 +
                    (((pEsConfig[0] & 0x7) << 3) | ((pEsConfig[1] >> 5) & 0x7));
    }

    free(pEsConfig);

    return mpeg4Type;
}


MP4Duration MP4File::GetTrackFixedSampleDuration(MP4TrackId trackId)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetFixedSampleDuration();
}

double MP4File::GetTrackVideoFrameRate(MP4TrackId trackId)
{
    MP4SampleId numSamples =
        GetTrackNumberOfSamples(trackId);
    uint64_t
    msDuration =
        ConvertFromTrackDuration(trackId,
                                 GetTrackDuration(trackId), MP4_MSECS_TIME_SCALE);

    if (msDuration == 0) {
        return 0.0;
    }

    return ((double)numSamples / double(msDuration)) * MP4_MSECS_TIME_SCALE;
}

int MP4File::GetTrackAudioChannels (MP4TrackId trackId)
{
    return GetTrackIntegerProperty(trackId,
                                   "mdia.minf.stbl.stsd.*[0].channels");
}

// true if media track encrypted according to ismacryp
bool MP4File::IsIsmaCrypMediaTrack(MP4TrackId trackId)
{
    if (GetTrackIntegerProperty(trackId,
                                "mdia.minf.stbl.stsd.*.sinf.frma.data-format")
            != (uint64_t)-1) {
        return true;
    }
    return false;
}

bool MP4File::IsWriteMode()
{
    if( !m_file )
        return false;

    switch( m_file->mode ) {
        case File::MODE_READ:
            return false;

        case File::MODE_MODIFY:
        case File::MODE_CREATE:
        default:
            break;
    }

    return true;
}

void MP4File::GetTrackESConfiguration(MP4TrackId trackId,
                                      uint8_t** ppConfig, uint32_t* pConfigSize)
{
    try {
        GetTrackBytesProperty(trackId,
                              "mdia.minf.stbl.stsd.*[0].esds.decConfigDescr.decSpecificInfo[0].info",
                              ppConfig, pConfigSize);
    } catch (Exception *x) {
        delete x;
        GetTrackBytesProperty(trackId,
                              "mdia.minf.stbl.stsd.*[0].*.esds.decConfigDescr.decSpecificInfo[0].info",
                              ppConfig, pConfigSize);
    }
}

void MP4File::GetTrackVideoMetadata(MP4TrackId trackId,
                                    uint8_t** ppConfig, uint32_t* pConfigSize)
{
    GetTrackBytesProperty(trackId,
                          "mdia.minf.stbl.stsd.*[0].*.metadata",
                          ppConfig, pConfigSize);
}

void MP4File::SetTrackESConfiguration(MP4TrackId trackId,
                                      const uint8_t* pConfig, uint32_t configSize)
{
    // get a handle on the track decoder config descriptor
    MP4DescriptorProperty* pConfigDescrProperty = NULL;
    if (FindProperty(MakeTrackName(trackId,
                                   "mdia.minf.stbl.stsd.*[0].esds.decConfigDescr.decSpecificInfo"),
                     (MP4Property**)&pConfigDescrProperty) == false ||
            pConfigDescrProperty == NULL) {
        // probably trackId refers to a hint track
        throw new Exception("no such property", __FILE__, __LINE__, __FUNCTION__);
    }

    // lookup the property to store the configuration
    MP4BytesProperty* pInfoProperty = NULL;
    (void)pConfigDescrProperty->FindProperty("decSpecificInfo[0].info",
            (MP4Property**)&pInfoProperty);

    // configuration being set for the first time
    if (pInfoProperty == NULL) {
        // need to create a new descriptor to hold it
        MP4Descriptor* pConfigDescr =
            pConfigDescrProperty->AddDescriptor(MP4DecSpecificDescrTag);
        pConfigDescr->Generate();

        (void)pConfigDescrProperty->FindProperty(
            "decSpecificInfo[0].info",
            (MP4Property**)&pInfoProperty);
        ASSERT(pInfoProperty);
    }

    // set the value
    pInfoProperty->SetValue(pConfig, configSize);
}


void MP4File::GetTrackH264SeqPictHeaders (MP4TrackId trackId,
        uint8_t ***pppSeqHeader,
        uint32_t **ppSeqHeaderSize,
        uint8_t ***pppPictHeader,
        uint32_t **ppPictHeaderSize)
{
    uint32_t count;
    const char *format;
    MP4Atom *avcCAtom;

    *pppSeqHeader = NULL;
    *pppPictHeader = NULL;
    *ppSeqHeaderSize = NULL;
    *ppPictHeaderSize = NULL;

    // get 4cc media format - can be avc1 or encv for ismacrypted track
    format = GetTrackMediaDataName (trackId);

    if (!strcasecmp(format, "avc1"))
        avcCAtom = FindAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.avc1.avcC"));
    else if (!strcasecmp(format, "encv"))
        avcCAtom = FindAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd.encv.avcC"));
    else
        // huh?  unknown track format
        return;

    MP4BitfieldProperty *pSeqCount;
    MP4IntegerProperty *pSeqLen, *pPictCount, *pPictLen;
    MP4BytesProperty *pSeqVal, *pPictVal;

    if ((avcCAtom->FindProperty("avcC.numOfSequenceParameterSets",
                                (MP4Property **)&pSeqCount) == false) ||
            (avcCAtom->FindProperty("avcC.sequenceEntries.sequenceParameterSetLength",
                                    (MP4Property **)&pSeqLen) == false) ||
            (avcCAtom->FindProperty("avcC.sequenceEntries.sequenceParameterSetNALUnit",
                                    (MP4Property **)&pSeqVal) == false)) {
        log.errorf("%s: \"%s\": Could not find avcC properties", __FUNCTION__, GetFilename().c_str());
        return ;
    }
    uint8_t **ppSeqHeader =
        (uint8_t **)malloc((pSeqCount->GetValue() + 1) * sizeof(uint8_t *));
    if (ppSeqHeader == NULL) return;
    *pppSeqHeader = ppSeqHeader;

    uint32_t *pSeqHeaderSize =
        (uint32_t *)malloc((pSeqCount->GetValue() + 1) * sizeof(uint32_t *));

    if (pSeqHeaderSize == NULL) return;

    *ppSeqHeaderSize = pSeqHeaderSize;
    for (count = 0; count < pSeqCount->GetValue(); count++) {
        pSeqVal->GetValue(&(ppSeqHeader[count]), &(pSeqHeaderSize[count]),
                          count);
    }
    ppSeqHeader[count] = NULL;
    pSeqHeaderSize[count] = 0;

    if ((avcCAtom->FindProperty("avcC.numOfPictureParameterSets",
                                (MP4Property **)&pPictCount) == false) ||
            (avcCAtom->FindProperty("avcC.pictureEntries.pictureParameterSetLength",
                                    (MP4Property **)&pPictLen) == false) ||
            (avcCAtom->FindProperty("avcC.pictureEntries.pictureParameterSetNALUnit",
                                    (MP4Property **)&pPictVal) == false)) {
        log.errorf("%s: \"%s\": Could not find avcC picture table properties",
                   __FUNCTION__, GetFilename().c_str());
        return ;
    }
    uint8_t
    **ppPictHeader =
        (uint8_t **)malloc((pPictCount->GetValue() + 1) * sizeof(uint8_t *));
    if (ppPictHeader == NULL) return;
    uint32_t *pPictHeaderSize =
        (uint32_t *)malloc((pPictCount->GetValue() + 1)* sizeof(uint32_t *));
    if (pPictHeaderSize == NULL) {
        free(ppPictHeader);
        return;
    }
    *pppPictHeader = ppPictHeader;
    *ppPictHeaderSize = pPictHeaderSize;

    for (count = 0; count < pPictCount->GetValue(); count++) {
        pPictVal->GetValue(&(ppPictHeader[count]), &(pPictHeaderSize[count]),
                           count);
    }
    ppPictHeader[count] = NULL;
    pPictHeaderSize[count] = 0;
    return ;
}



const char* MP4File::GetHintTrackSdp(MP4TrackId hintTrackId)
{
    return GetTrackStringProperty(hintTrackId, "udta.hnti.sdp .sdpText");
}

void MP4File::SetHintTrackSdp(MP4TrackId hintTrackId, const char* sdpString)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track", __FILE__, __LINE__, __FUNCTION__);
    }

    (void)AddDescendantAtoms(
        MakeTrackName(hintTrackId, NULL), "udta.hnti.sdp ");

    SetTrackStringProperty(hintTrackId, "udta.hnti.sdp .sdpText", sdpString);
}

void MP4File::AppendHintTrackSdp(MP4TrackId hintTrackId,
                                 const char* sdpFragment)
{
    const char* oldSdpString = NULL;
    try {
        oldSdpString = GetHintTrackSdp(hintTrackId);
    }
    catch (Exception* x) {
        delete x;
        SetHintTrackSdp(hintTrackId, sdpFragment);
        return;
    }

    char* newSdpString =
        (char*)MP4Malloc(strlen(oldSdpString) + strlen(sdpFragment) + 1);
    strcpy(newSdpString, oldSdpString);
    strcat(newSdpString, sdpFragment);
    SetHintTrackSdp(hintTrackId, newSdpString);
    MP4Free(newSdpString);
}

void MP4File::GetHintTrackRtpPayload(
    MP4TrackId hintTrackId,
    char** ppPayloadName,
    uint8_t* pPayloadNumber,
    uint16_t* pMaxPayloadSize,
    char **ppEncodingParams)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track", __FILE__, __LINE__, __FUNCTION__);
    }

    ((MP4RtpHintTrack*)pTrack)->GetPayload(
        ppPayloadName, pPayloadNumber, pMaxPayloadSize, ppEncodingParams);
}

void MP4File::SetHintTrackRtpPayload(MP4TrackId hintTrackId,
                                     const char* payloadName, uint8_t* pPayloadNumber, uint16_t maxPayloadSize,
                                     const char *encoding_params,
                                     bool include_rtp_map,
                                     bool include_mpeg4_esid)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track", __FILE__, __LINE__, __FUNCTION__);
    }

    uint8_t payloadNumber;
    if (pPayloadNumber && *pPayloadNumber != MP4_SET_DYNAMIC_PAYLOAD) {
        payloadNumber = *pPayloadNumber;
    } else {
        payloadNumber = AllocRtpPayloadNumber();
        if (pPayloadNumber) {
            *pPayloadNumber = payloadNumber;
        }
    }

    ((MP4RtpHintTrack*)pTrack)->SetPayload(
        payloadName, payloadNumber, maxPayloadSize, encoding_params,
        include_rtp_map, include_mpeg4_esid);
}

uint8_t MP4File::AllocRtpPayloadNumber()
{
    MP4Integer32Array usedPayloads;
    uint32_t i;

    // collect rtp payload numbers in use by existing tracks
    for (i = 0; i < m_pTracks.Size(); i++) {
        MP4Atom& trakAtom = m_pTracks[i]->GetTrakAtom();

        MP4Integer32Property* pPayloadProperty = NULL;
        if (trakAtom.FindProperty("trak.udta.hinf.payt.payloadNumber",
                                    (MP4Property**)&pPayloadProperty) &&
                pPayloadProperty) {
            usedPayloads.Add(pPayloadProperty->GetValue());
        }
    }

    // search dynamic payload range for an available slot
    uint8_t payload;
    for (payload = 96; payload < 128; payload++) {
        for (i = 0; i < usedPayloads.Size(); i++) {
            if (payload == usedPayloads[i]) {
                break;
            }
        }
        if (i == usedPayloads.Size()) {
            break;
        }
    }

    if (payload >= 128) {
        throw new Exception("no more available rtp payload numbers",
                            __FILE__, __LINE__, __FUNCTION__);
    }

    return payload;
}

MP4TrackId MP4File::GetHintTrackReferenceTrackId(
    MP4TrackId hintTrackId)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }

    MP4Track* pRefTrack = ((MP4RtpHintTrack*)pTrack)->GetRefTrack();

    if (pRefTrack == NULL) {
        return MP4_INVALID_TRACK_ID;
    }
    return pRefTrack->GetId();
}

void MP4File::ReadRtpHint(
    MP4TrackId hintTrackId,
    MP4SampleId hintSampleId,
    uint16_t* pNumPackets)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track", __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->
    ReadHint(hintSampleId, pNumPackets);
}

uint16_t MP4File::GetRtpHintNumberOfPackets(
    MP4TrackId hintTrackId)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    return ((MP4RtpHintTrack*)pTrack)->GetHintNumberOfPackets();
}

int8_t MP4File::GetRtpPacketBFrame(
    MP4TrackId hintTrackId,
    uint16_t packetIndex)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    return ((MP4RtpHintTrack*)pTrack)->GetPacketBFrame(packetIndex);
}

int32_t MP4File::GetRtpPacketTransmitOffset(
    MP4TrackId hintTrackId,
    uint16_t packetIndex)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    return ((MP4RtpHintTrack*)pTrack)->GetPacketTransmitOffset(packetIndex);
}

void MP4File::ReadRtpPacket(
    MP4TrackId hintTrackId,
    uint16_t packetIndex,
    uint8_t** ppBytes,
    uint32_t* pNumBytes,
    uint32_t ssrc,
    bool includeHeader,
    bool includePayload)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track", __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->ReadPacket(
        packetIndex, ppBytes, pNumBytes,
        ssrc, includeHeader, includePayload);
}

MP4Timestamp MP4File::GetRtpTimestampStart(
    MP4TrackId hintTrackId)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track", __FILE__, __LINE__, __FUNCTION__);
    }
    return ((MP4RtpHintTrack*)pTrack)->GetRtpTimestampStart();
}

void MP4File::SetRtpTimestampStart(
    MP4TrackId hintTrackId,
    MP4Timestamp rtpStart)
{
    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->SetRtpTimestampStart(rtpStart);
}

void MP4File::AddRtpHint(MP4TrackId hintTrackId,
                         bool isBframe, uint32_t timestampOffset)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track", __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->AddHint(isBframe, timestampOffset);
}

void MP4File::AddRtpPacket(
    MP4TrackId hintTrackId, bool setMbit, int32_t transmitOffset)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track", __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->AddPacket(setMbit, transmitOffset);
}

void MP4File::AddRtpImmediateData(MP4TrackId hintTrackId,
                                  const uint8_t* pBytes, uint32_t numBytes)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->AddImmediateData(pBytes, numBytes);
}

void MP4File::AddRtpSampleData(MP4TrackId hintTrackId,
                               MP4SampleId sampleId, uint32_t dataOffset, uint32_t dataLength)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->AddSampleData(
        sampleId, dataOffset, dataLength);
}

void MP4File::AddRtpESConfigurationPacket(MP4TrackId hintTrackId)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->AddESConfigurationPacket();
}

void MP4File::WriteRtpHint(MP4TrackId hintTrackId,
                           MP4Duration duration, bool isSyncSample)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);

    MP4Track* pTrack = m_pTracks[FindTrackIndex(hintTrackId)];

    if (strcmp(pTrack->GetType(), MP4_HINT_TRACK_TYPE)) {
        throw new Exception("track is not a hint track",
                            __FILE__, __LINE__, __FUNCTION__);
    }
    ((MP4RtpHintTrack*)pTrack)->WriteHint(duration, isSyncSample);
}

uint64_t MP4File::ConvertFromMovieDuration(
    MP4Duration duration,
    uint32_t timeScale)
{
    return MP4ConvertTime((uint64_t)duration,
                          GetTimeScale(), timeScale);
}

uint64_t MP4File::ConvertFromTrackTimestamp(
    MP4TrackId trackId,
    MP4Timestamp timeStamp,
    uint32_t timeScale)
{
    return MP4ConvertTime(timeStamp,
                          GetTrackTimeScale(trackId), timeScale);
}

MP4Timestamp MP4File::ConvertToTrackTimestamp(
    MP4TrackId trackId,
    uint64_t timeStamp,
    uint32_t timeScale)
{
    return (MP4Timestamp)MP4ConvertTime(timeStamp,
                                        timeScale, GetTrackTimeScale(trackId));
}

uint64_t MP4File::ConvertFromTrackDuration(
    MP4TrackId trackId,
    MP4Duration duration,
    uint32_t timeScale)
{
    return MP4ConvertTime((uint64_t)duration,
                          GetTrackTimeScale(trackId), timeScale);
}

MP4Duration MP4File::ConvertToTrackDuration(
    MP4TrackId trackId,
    uint64_t duration,
    uint32_t timeScale)
{
    return (MP4Duration)MP4ConvertTime(duration,
                                       timeScale, GetTrackTimeScale(trackId));
}

uint8_t MP4File::ConvertTrackTypeToStreamType(const char* trackType)
{
    uint8_t streamType;

    if (!strcmp(trackType, MP4_OD_TRACK_TYPE)) {
        streamType = MP4ObjectDescriptionStreamType;
    } else if (!strcmp(trackType, MP4_SCENE_TRACK_TYPE)) {
        streamType = MP4SceneDescriptionStreamType;
    } else if (!strcmp(trackType, MP4_CLOCK_TRACK_TYPE)) {
        streamType = MP4ClockReferenceStreamType;
    } else if (!strcmp(trackType, MP4_MPEG7_TRACK_TYPE)) {
        streamType = MP4Mpeg7StreamType;
    } else if (!strcmp(trackType, MP4_OCI_TRACK_TYPE)) {
        streamType = MP4OCIStreamType;
    } else if (!strcmp(trackType, MP4_IPMP_TRACK_TYPE)) {
        streamType = MP4IPMPStreamType;
    } else if (!strcmp(trackType, MP4_MPEGJ_TRACK_TYPE)) {
        streamType = MP4MPEGJStreamType;
    } else {
        streamType = MP4UserPrivateStreamType;
    }

    return streamType;
}

// edit list

char* MP4File::MakeTrackEditName(
    MP4TrackId trackId,
    MP4EditId editId,
    const char* name)
{
    char* trakName = MakeTrackName(trackId, NULL);

    if (m_editName == NULL) {
        m_editName = (char *)malloc(1024);
        if (m_editName == NULL) return NULL;
    }
    snprintf(m_editName, 1024,
             "%s.edts.elst.entries[%u].%s",
             trakName, editId - 1, name);
    return m_editName;
}

MP4EditId MP4File::AddTrackEdit(
    MP4TrackId trackId,
    MP4EditId editId)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);
    return m_pTracks[FindTrackIndex(trackId)]->AddEdit(editId);
}

void MP4File::DeleteTrackEdit(
    MP4TrackId trackId,
    MP4EditId editId)
{
    ProtectWriteOperation(__FILE__, __LINE__, __FUNCTION__);
    m_pTracks[FindTrackIndex(trackId)]->DeleteEdit(editId);
}

uint32_t MP4File::GetTrackNumberOfEdits(
    MP4TrackId trackId)
{
    return GetTrackIntegerProperty(trackId, "edts.elst.entryCount");
}

MP4Duration MP4File::GetTrackEditTotalDuration(
    MP4TrackId trackId,
    MP4EditId editId)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetEditTotalDuration(editId);
}

MP4Timestamp MP4File::GetTrackEditStart(
    MP4TrackId trackId,
    MP4EditId editId)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetEditStart(editId);
}

MP4Timestamp MP4File::GetTrackEditMediaStart(
    MP4TrackId trackId,
    MP4EditId editId)
{
    return GetIntegerProperty(
               MakeTrackEditName(trackId, editId, "mediaTime"));
}

void MP4File::SetTrackEditMediaStart(
    MP4TrackId trackId,
    MP4EditId editId,
    MP4Timestamp startTime)
{
    SetIntegerProperty(
        MakeTrackEditName(trackId, editId, "mediaTime"),
        startTime);
}

MP4Duration MP4File::GetTrackEditDuration(
    MP4TrackId trackId,
    MP4EditId editId)
{
    return GetIntegerProperty(
               MakeTrackEditName(trackId, editId, "segmentDuration"));
}

void MP4File::SetTrackEditDuration(
    MP4TrackId trackId,
    MP4EditId editId,
    MP4Duration duration)
{
    SetIntegerProperty(
        MakeTrackEditName(trackId, editId, "segmentDuration"),
        duration);
}

bool MP4File::GetTrackEditDwell(
    MP4TrackId trackId,
    MP4EditId editId)
{
    return (GetIntegerProperty(
                MakeTrackEditName(trackId, editId, "mediaRate")) == 0);
}

void MP4File::SetTrackEditDwell(
    MP4TrackId trackId,
    MP4EditId editId,
    bool dwell)
{
    SetIntegerProperty(
        MakeTrackEditName(trackId, editId, "mediaRate"),
        (dwell ? 0 : 1));
}

MP4SampleId MP4File::GetSampleIdFromEditTime(
    MP4TrackId trackId,
    MP4Timestamp when,
    MP4Timestamp* pStartTime,
    MP4Duration* pDuration)
{
    return m_pTracks[FindTrackIndex(trackId)]->GetSampleIdFromEditTime(
               when, pStartTime, pDuration);
}

MP4Duration MP4File::GetTrackDurationPerChunk( MP4TrackId trackId )
{
    return m_pTracks[FindTrackIndex(trackId)]->GetDurationPerChunk();
}

void MP4File::SetTrackDurationPerChunk( MP4TrackId trackId, MP4Duration duration )
{
    m_pTracks[FindTrackIndex(trackId)]->SetDurationPerChunk( duration );
}

void MP4File::CopySample(
    MP4File*    srcFile,
    MP4TrackId  srcTrackId,
    MP4SampleId srcSampleId,
    MP4File*    dstFile,
    MP4TrackId  dstTrackId,
    MP4Duration dstSampleDuration )
{
    // Note: we leave it up to the caller to ensure that the
    // source and destination tracks are compatible.
    // i.e. copying audio samples into a video track
    // is unlikely to do anything useful

    uint8_t* pBytes = NULL;
    uint32_t numBytes = 0;
    MP4Duration sampleDuration;
    MP4Duration renderingOffset;
    bool isSyncSample;
    bool hasDependencyFlags;
    uint32_t dependencyFlags;

    srcFile->ReadSample(
         srcTrackId,
         srcSampleId,
         &pBytes,
         &numBytes,
         NULL,
         &sampleDuration,
         &renderingOffset,
         &isSyncSample,
         &hasDependencyFlags,
         &dependencyFlags );

    if( !dstFile )
        dstFile = srcFile;

    if( dstTrackId == MP4_INVALID_TRACK_ID )
        dstTrackId = srcTrackId;

    if( dstSampleDuration != MP4_INVALID_DURATION )
        sampleDuration = dstSampleDuration;

    if( hasDependencyFlags ) {
        dstFile->WriteSampleDependency(
            dstTrackId,
            pBytes,
            numBytes,
            sampleDuration,
            renderingOffset,
            isSyncSample,
            dependencyFlags );
    }
    else {
        dstFile->WriteSample(
            dstTrackId,
            pBytes,
            numBytes,
            sampleDuration,
            renderingOffset,
            isSyncSample );
    }

    free( pBytes );
}

void MP4File::EncAndCopySample(
    MP4File*      srcFile,
    MP4TrackId    srcTrackId,
    MP4SampleId   srcSampleId,
    encryptFunc_t encfcnp,
    uint32_t      encfcnparam1,
    MP4File*      dstFile,
    MP4TrackId    dstTrackId,
    MP4Duration   dstSampleDuration )
{
    // Note: we leave it up to the caller to ensure that the
    // source and destination tracks are compatible.
    // i.e. copying audio samples into a video track
    // is unlikely to do anything useful

    uint8_t* pBytes = NULL;
    uint32_t numBytes = 0;
    uint8_t* encSampleData = NULL;
    uint32_t encSampleLength = 0;
    MP4Duration sampleDuration;
    MP4Duration renderingOffset;
    bool isSyncSample;
    bool hasDependencyFlags;
    uint32_t dependencyFlags;

    ASSERT(srcFile);
    srcFile->ReadSample(
         srcTrackId,
         srcSampleId,
         &pBytes,
         &numBytes,
         NULL,
         &sampleDuration,
         &renderingOffset,
         &isSyncSample,
         &hasDependencyFlags,
         &dependencyFlags );

    if( !dstFile )
        dstFile = srcFile;

    ASSERT(dstFile);

    if( dstTrackId == MP4_INVALID_TRACK_ID )
        dstTrackId = srcTrackId;

    if( dstSampleDuration != MP4_INVALID_DURATION )
        sampleDuration = dstSampleDuration;

    //if( ismacrypEncryptSampleAddHeader( ismaCryptSId, numBytes, pBytes, &encSampleLength, &encSampleData ) != 0)
    if( encfcnp( encfcnparam1, numBytes, pBytes, &encSampleLength, &encSampleData ) != 0 )
        log.errorf("%s(%s,%s) Can't encrypt the sample and add its header %u", 
                   __FUNCTION__, srcFile->GetFilename().c_str(), dstFile->GetFilename().c_str(), srcSampleId );

    if( hasDependencyFlags ) {
        dstFile->WriteSampleDependency(
            dstTrackId,
            pBytes,
            numBytes,
            sampleDuration,
            renderingOffset,
            isSyncSample,
            dependencyFlags );
    }
    else {
        dstFile->WriteSample(
            dstTrackId,
            encSampleData,
            encSampleLength,
            sampleDuration,
            renderingOffset,
            isSyncSample );
    }

    free( pBytes );

    if( encSampleData != NULL )
        free( encSampleData );
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
