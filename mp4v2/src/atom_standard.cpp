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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *      Bill May (from others work).
 */

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4StandardAtom::MP4StandardAtom (MP4File &file, const char *type) : MP4Atom(file, type)
{
    /*
     * This is a big if else loop.  Make sure that you don't break it
     * when adding new atoms, or you will set the unknown type flag
     *
     * Try to keep it in alphabetical order - it should only be called
     * 1 time per atom, so it's not that urgent.
     */
    if (ATOMID(type) == ATOMID("bitr")) {
        AddProperty( /* 0 */
            new MP4Integer32Property(*this, "avgBitrate"));

        AddProperty( /* 1 */
            new MP4Integer32Property(*this, "maxBitrate"));

    } else if (ATOMID(type) == ATOMID("btrt")) {
        AddProperty( new MP4Integer32Property(*this, "bufferSizeDB")); /* 0 */
        AddProperty( new MP4Integer32Property(*this, "avgBitrate"));   /* 1 */
        AddProperty( new MP4Integer32Property(*this, "maxBitrate"));   /* 2 */
    } else if (ATOMID(type) == ATOMID("burl")) {
        AddProperty( new MP4StringProperty(*this, "base_url"));
        /*
         * c???
         */
    } else if (ATOMID(type) == ATOMID("co64")) {
        AddVersionAndFlags();

        MP4Integer32Property* pCount =
            new MP4Integer32Property(*this, "entryCount");
        AddProperty(pCount);

        MP4TableProperty* pTable = new MP4TableProperty(*this, "entries", pCount);
        AddProperty(pTable);

        pTable->AddProperty(
            new MP4Integer64Property(*this, "chunkOffset"));
    } else if (ATOMID(type) == ATOMID("ctts")) {
        AddVersionAndFlags();

        MP4Integer32Property* pCount =
            new MP4Integer32Property(*this, "entryCount");
        AddProperty(pCount);

        MP4TableProperty* pTable = new MP4TableProperty(*this, "entries", pCount);
        AddProperty(pTable);

        pTable->AddProperty(new MP4Integer32Property(pTable->GetParentAtom(), "sampleCount"));
        pTable->AddProperty(new MP4Integer32Property(pTable->GetParentAtom(), "sampleOffset"));
        /*
         * d???
         */
    } else if (ATOMID(type) == ATOMID("dinf")) {
        ExpectChildAtom("dref", Required, OnlyOne);

    } else if (ATOMID(type) == ATOMID("dimm")) {
        AddProperty( // bytes of immediate data
            new MP4Integer64Property(*this, "bytes"));

    } else if (ATOMID(type) == ATOMID("dmax")) {
        AddProperty( // max packet duration
            new MP4Integer32Property(*this, "milliSecs"));

    } else if (ATOMID(type) == ATOMID("dmed")) {
        AddProperty( // bytes sent from media data
            new MP4Integer64Property(*this, "bytes"));

    } else if (ATOMID(type) == ATOMID("drep")) {
        AddProperty( // bytes of repeated data
            new MP4Integer64Property(*this, "bytes"));
        /*
         * e???
         */
    } else if (ATOMID(type) == ATOMID("edts")) {
        ExpectChildAtom("elst", Required, OnlyOne);

    } else if (ATOMID(type) == ATOMID("esds")) {
        AddVersionAndFlags();
        AddProperty(
            new MP4DescriptorProperty(*this, NULL, MP4ESDescrTag, 0,
                                      Required, OnlyOne));
        /*
         * f???
         */
    } else if (ATOMID(type) == ATOMID("frma")) {
        AddProperty( /* 0 */
            new MP4Integer32Property(*this, "data-format"));
        /*
         * g???
         */
    } else if (ATOMID(type) == ATOMID("gmhd")) {
        ExpectChildAtom("gmin", Required, OnlyOne);
        ExpectChildAtom("tmcd", Optional, OnlyOne);
        ExpectChildAtom("text", Optional, OnlyOne);
    } else if (ATOMID(type) == ATOMID("hmhd")) {
        AddVersionAndFlags();

        AddProperty(new MP4Integer16Property(*this, "maxPduSize"));
        AddProperty(new MP4Integer16Property(*this, "avgPduSize"));
        AddProperty(new MP4Integer32Property(*this, "maxBitRate"));
        AddProperty(new MP4Integer32Property(*this, "avgBitRate"));
        AddProperty(new MP4Integer32Property(*this, "slidingAvgBitRate"));
        /*
         * i???
         */
    } else if (ATOMID(type) == ATOMID("iKMS")) {
        AddVersionAndFlags(); /* 0, 1 */
        MP4StringProperty* pProp = new MP4StringProperty(*this, "kms_URI");
        AddProperty(pProp); /* 2 */

    } else if (ATOMID(type) == ATOMID("iSFM")) {
        AddVersionAndFlags(); /* 0, 1 */
        AddProperty( /* 2 */
            new MP4BitfieldProperty(*this, "selective-encryption", 1));
        AddProperty( /* 3 */
            new MP4BitfieldProperty(*this, "reserved", 7));
        AddProperty( /* 4 */
            new MP4Integer8Property(*this, "key-indicator-length"));
        AddProperty( /* 5 */
            new MP4Integer8Property(*this, "IV-length"));

    } else if (ATOMID(type) == ATOMID("ilst")) {
        ExpectChildAtom("\251nam", Optional, OnlyOne); /* name */
        ExpectChildAtom("\251ART", Optional, OnlyOne); /* artist */
        ExpectChildAtom("\251wrt", Optional, OnlyOne); /* writer */
        ExpectChildAtom("\251alb", Optional, OnlyOne); /* album */
        ExpectChildAtom("\251day", Optional, OnlyOne); /* date */
        ExpectChildAtom("\251too", Optional, OnlyOne); /* tool */
        ExpectChildAtom("\251cmt", Optional, OnlyOne); /* comment */
        ExpectChildAtom("\251gen", Optional, OnlyOne); /* custom genre */
        ExpectChildAtom("trkn", Optional, OnlyOne); /* tracknumber */
        ExpectChildAtom("disk", Optional, OnlyOne); /* disknumber */
        ExpectChildAtom("gnre", Optional, OnlyOne); /* genre (ID3v1 index + 1) */
        ExpectChildAtom("cpil", Optional, OnlyOne); /* compilation */
        ExpectChildAtom("tmpo", Optional, OnlyOne); /* BPM */
        ExpectChildAtom("covr", Optional, OnlyOne); /* cover art */
        ExpectChildAtom("aART", Optional, OnlyOne); /* album artist */
        ExpectChildAtom("----", Optional, Many); /* ---- free form */
        ExpectChildAtom("pgap", Optional, OnlyOne); /* part of gapless album */
        ExpectChildAtom("tvsh", Optional, OnlyOne); /* TV show */
        ExpectChildAtom("tvsn", Optional, OnlyOne); /* TV season */
        ExpectChildAtom("tven", Optional, OnlyOne); /* TV episode number */
        ExpectChildAtom("tvnn", Optional, OnlyOne); /* TV network name */
        ExpectChildAtom("tves", Optional, OnlyOne); /* TV epsidoe */
        ExpectChildAtom("desc", Optional, OnlyOne); /* description */
        ExpectChildAtom("ldes", Optional, OnlyOne); /* long description */
        ExpectChildAtom("soal", Optional, OnlyOne); /* sort album */
        ExpectChildAtom("soar", Optional, OnlyOne); /* sort artist */
        ExpectChildAtom("soaa", Optional, OnlyOne); /* sort album artist */
        ExpectChildAtom("sonm", Optional, OnlyOne); /* sort name */
        ExpectChildAtom("soco", Optional, OnlyOne); /* sort composer */
        ExpectChildAtom("sosn", Optional, OnlyOne); /* sort show */
        ExpectChildAtom("hdvd", Optional, OnlyOne); /* HD video */
        ExpectChildAtom("©enc", Optional, OnlyOne); /* Encoded by */
        ExpectChildAtom("pcst", Optional, OnlyOne); /* Podcast flag */
        ExpectChildAtom("keyw", Optional, OnlyOne); /* Keywords (for podcasts?) */
        ExpectChildAtom("catg", Optional, OnlyOne); /* Category (for podcasts?) */
        ExpectChildAtom("purl", Optional, OnlyOne); /* Podcast URL */
        ExpectChildAtom("egid", Optional, OnlyOne); /* Podcast episode global unique ID */
        ExpectChildAtom("rtng", Optional, OnlyOne); /* Content Rating */
        ExpectChildAtom("stik", Optional, OnlyOne); /* MediaType */
        ExpectChildAtom("\251grp", Optional, OnlyOne); /* Grouping */
        ExpectChildAtom("\251lyr", Optional, OnlyOne); /* Lyrics */
        ExpectChildAtom("cprt", Optional, OnlyOne); /* Copyright */
        ExpectChildAtom("apID", Optional, OnlyOne); /* iTunes Account */
        ExpectChildAtom("akID", Optional, OnlyOne); /* iTunes Account Type */
        ExpectChildAtom("sfID", Optional, OnlyOne); /* iTunes Country */
        ExpectChildAtom("cnID", Optional, OnlyOne); /* Content ID */
        ExpectChildAtom("atID", Optional, OnlyOne); /* Artist ID */
        ExpectChildAtom("plID", Optional, OnlyOne); /* Playlist ID */
        ExpectChildAtom("geID", Optional, OnlyOne); /* Genre ID */
        ExpectChildAtom("cmID", Optional, OnlyOne); /* Composer ID */
        ExpectChildAtom("xid ", Optional, OnlyOne); /* XID */

    }  else if (ATOMID(type) == ATOMID("imif")) {
        AddVersionAndFlags();
        AddProperty(new MP4DescriptorProperty(*this, "ipmp_desc", MP4IPMPDescrTag,
                                              MP4IPMPDescrTag, Required, Many));
    } else if (ATOMID(type) == ATOMID("iods")) {
        AddVersionAndFlags();
        AddProperty(
            new MP4DescriptorProperty(*this, NULL, MP4FileIODescrTag,
                                      MP4FileODescrTag,
                                      Required, OnlyOne));
        /*
         * m???
         */
    } else if (ATOMID(type) == ATOMID("maxr")) {
        AddProperty(new MP4Integer32Property(*this, "granularity"));
        AddProperty(new MP4Integer32Property(*this, "bytes"));

    } else if (ATOMID(type) == ATOMID("mdia")) {
        ExpectChildAtom("mdhd", Required, OnlyOne);
        ExpectChildAtom("hdlr", Required, OnlyOne);
        ExpectChildAtom("minf", Required, OnlyOne);

    } else if (ATOMID(type) == ATOMID("meta")) { // iTunes
        AddVersionAndFlags(); /* 0, 1 */
        ExpectChildAtom("hdlr", Required, OnlyOne);
        ExpectChildAtom("ilst", Required, OnlyOne);

    } else if (ATOMID(type) == ATOMID("mfhd")) {
        AddVersionAndFlags();   /* 0, 1 */
        AddProperty( /* 2 */
            new MP4Integer32Property(*this, "sequenceNumber"));

    } else if (ATOMID(type) == ATOMID("minf")) {
        ExpectChildAtom("vmhd", Optional, OnlyOne);
        ExpectChildAtom("smhd", Optional, OnlyOne);
        ExpectChildAtom("hmhd", Optional, OnlyOne);
        ExpectChildAtom("nmhd", Optional, OnlyOne);
        ExpectChildAtom("gmhd", Optional, OnlyOne);
        ExpectChildAtom("dinf", Required, OnlyOne);
        ExpectChildAtom("stbl", Required, OnlyOne);

    } else if (ATOMID(type) == ATOMID("moof")) {
        ExpectChildAtom("mfhd", Required, OnlyOne);
        ExpectChildAtom("traf", Optional, Many);

    } else if (ATOMID(type) == ATOMID("moov")) {
        ExpectChildAtom("mvhd", Required, OnlyOne);
        ExpectChildAtom("iods", Optional, OnlyOne);
        ExpectChildAtom("trak", Required, Many);
        ExpectChildAtom("udta", Optional, Many);
        ExpectChildAtom("mvex", Optional, OnlyOne);

    } else if (ATOMID(type) == ATOMID("mvex")) {
        ExpectChildAtom("trex", Required, Many);

        /*
         * n???
         */
    } else if (ATOMID(type) == ATOMID("nmhd")) {
        AddVersionAndFlags();

    } else if (ATOMID(type) == ATOMID("nump")) {
        AddProperty( // packets sent
            new MP4Integer64Property(*this, "packets"));
        /*
          * o???
          */
    } else if (ATOMID(type) == ATOMID("odkm")) {
        AddVersionAndFlags();
        ExpectChildAtom("ohdr", Required, OnlyOne);
        /*
         * p???
         */
    } else if (ATOMID(type) == ATOMID("payt")) {
        AddProperty(new MP4Integer32Property(*this, "payloadNumber"));
        AddProperty(new MP4StringProperty(*this, "rtpMap", Counted));

    } else if (ATOMID(type) == ATOMID("pinf")) {
        ExpectChildAtom("frma", Required, OnlyOne);
    } else if (ATOMID(type) == ATOMID("pmax")) {
        AddProperty( // max packet size
            new MP4Integer32Property(*this, "bytes"));
    } else if (ATOMID(type) == ATOMID("schi")) {
        // not sure if this is child atoms or table of boxes
        // get clarification on spec 9.1.2.5
        ExpectChildAtom("odkm", Optional, OnlyOne);
        ExpectChildAtom("iKMS", Optional, OnlyOne);
        ExpectChildAtom("iSFM", Optional, OnlyOne);

    } else if (ATOMID(type) == ATOMID("schm")) {
        AddVersionAndFlags(); /* 0, 1 */
        AddProperty( /* 2 */
            new MP4Integer32Property(*this, "scheme_type"));
        AddProperty( /* 3 */
            new MP4Integer32Property(*this, "scheme_version"));
        // browser URI if flags set, TODO

    } else if (ATOMID(type) == ATOMID("sinf")) {
        ExpectChildAtom("frma", Required, OnlyOne);
        ExpectChildAtom("imif", Optional, OnlyOne);
        ExpectChildAtom("schm", Optional, OnlyOne);
        ExpectChildAtom("schi", Optional, OnlyOne);

    } else if (ATOMID(type) == ATOMID("smhd")) {
        AddVersionAndFlags();
        AddReserved(*this, "reserved", 4);

    } else if (ATOMID(type) == ATOMID("snro")) {
        AddProperty(new MP4Integer32Property(*this, "offset"));

    } else if (ATOMID(type) == ATOMID("stco")) {
        AddVersionAndFlags();

        MP4Integer32Property* pCount = new MP4Integer32Property(*this, "entryCount");
        AddProperty(pCount);

        MP4TableProperty* pTable = new MP4TableProperty(*this, "entries", pCount);
        AddProperty(pTable);

        pTable->AddProperty(new MP4Integer32Property(pTable->GetParentAtom(), "chunkOffset"));

    } else if (ATOMID(type) == ATOMID("stsh")) {
        AddVersionAndFlags();

        MP4Integer32Property* pCount = new MP4Integer32Property(*this, "entryCount");
        AddProperty(pCount);

        MP4TableProperty* pTable = new MP4TableProperty(*this, "entries", pCount);
        AddProperty(pTable);

        pTable->AddProperty(new MP4Integer32Property(pTable->GetParentAtom(), "shadowedSampleNumber"));
        pTable->AddProperty(new MP4Integer32Property(pTable->GetParentAtom(), "syncSampleNumber"));

    } else if (ATOMID(type) == ATOMID("stss")) {
        AddVersionAndFlags();

        MP4Integer32Property* pCount = new MP4Integer32Property(*this, "entryCount");
        AddProperty(pCount);

        MP4TableProperty* pTable = new MP4TableProperty(*this, "entries", pCount);
        AddProperty(pTable);

        pTable->AddProperty(new MP4Integer32Property(pTable->GetParentAtom(), "sampleNumber"));

    } else if (ATOMID(type) == ATOMID("stts")) {
        AddVersionAndFlags();
        MP4Integer32Property* pCount = new MP4Integer32Property(*this, "entryCount");
        AddProperty(pCount);

        MP4TableProperty* pTable = new MP4TableProperty(*this, "entries", pCount);
        AddProperty(pTable);

        pTable->AddProperty(new MP4Integer32Property(pTable->GetParentAtom(), "sampleCount"));
        pTable->AddProperty(new MP4Integer32Property(pTable->GetParentAtom(), "sampleDelta"));
    } else if (ATOMID(type) == ATOMID("tims")) {
        AddProperty(
            new MP4Integer32Property(*this, "timeScale"));

    } else if (ATOMID(type) == ATOMID("tmin")) {
        AddProperty( // min relative xmit time
            new MP4Integer32Property(*this, "milliSecs"));

    } else if (ATOMID(type) == ATOMID("tmax")) {
        AddProperty( // max relative xmit time
            new MP4Integer32Property(*this, "milliSecs"));

    } else if (ATOMID(type) == ATOMID("traf")) {
        ExpectChildAtom("tfhd", Required, OnlyOne);
        ExpectChildAtom("trun", Optional, Many);

    } else if (ATOMID(type) == ATOMID("trak")) {
        ExpectChildAtom("tkhd", Required, OnlyOne);
        ExpectChildAtom("tref", Optional, OnlyOne);
        ExpectChildAtom("edts", Optional, OnlyOne);
        ExpectChildAtom("mdia", Required, OnlyOne);
        ExpectChildAtom("udta", Optional, Many);

    } else if (ATOMID(type) == ATOMID("tref")) {
        ExpectChildAtom("chap", Optional, OnlyOne);
        ExpectChildAtom("dpnd", Optional, OnlyOne);
        ExpectChildAtom("hint", Optional, OnlyOne);
        ExpectChildAtom("ipir", Optional, OnlyOne);
        ExpectChildAtom("mpod", Optional, OnlyOne);
        ExpectChildAtom("sync", Optional, OnlyOne);

    } else if (ATOMID(type) == ATOMID("trex")) {
        AddVersionAndFlags();   /* 0, 1 */
        AddProperty( /* 2 */
            new MP4Integer32Property(*this, "trackId"));
        AddProperty( /* 3 */
            new MP4Integer32Property(*this, "defaultSampleDesriptionIndex"));
        AddProperty( /* 4 */
            new MP4Integer32Property(*this, "defaultSampleDuration"));
        AddProperty( /* 5 */
            new MP4Integer32Property(*this, "defaultSampleSize"));
        AddProperty( /* 6 */
            new MP4Integer32Property(*this, "defaultSampleFlags"));

    } else if (ATOMID(type) == ATOMID("trpy") ||
               ATOMID(type) == ATOMID("tpyl")) {
        AddProperty( // bytes sent including RTP headers
            new MP4Integer64Property(*this, "bytes"));

    } else if (ATOMID(type) == ATOMID("tsro")) {
        AddProperty(
            new MP4Integer32Property(*this, "offset"));
    } else if (ATOMID(type) == ATOMID("wave")) {
        ExpectChildAtom("esds", Required, OnlyOne);
    } else {
        /*
         * default - unknown type
         */
        SetUnknownType(true);
    }
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
