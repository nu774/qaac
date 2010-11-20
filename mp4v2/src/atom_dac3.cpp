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
 * See ETSI TS 102 366 V1.2.1 Annex F for how to put Ac3 in MP4.
 *
 * Contributor(s):
 *      Edward Groenendaal      egroenen@cisco.com
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////


MP4DAc3Atom::MP4DAc3Atom()
        : MP4Atom("dac3")
{
    AddProperty( new MP4BitfieldProperty("fscod", 2)); /* 0 */
    AddProperty( new MP4BitfieldProperty("bsid", 5)); /* 1 */
    AddProperty( new MP4BitfieldProperty("bsmod", 3)); /* 2 */
    AddProperty( new MP4BitfieldProperty("acmod", 3)); /* 3 */
    AddProperty( new MP4BitfieldProperty("lfeon", 1)); /* 4 */
    AddProperty( new MP4BitfieldProperty("bit_rate_code", 5)); /* 5 */
    AddProperty( new MP4BitfieldProperty("reserved", 5)); /* 6 */
    m_pProperties[6]->SetReadOnly(true);
}

void MP4DAc3Atom::Generate()
{
    MP4Atom::Generate();
 

}

/*
 * Normalling the Dump of the atom looks like the following, we want to spruce
 * this up a bit to give more information.
 *
 *      type = dac3
 *       fscod = 0 (0x0) <2 bits>
 *       bsid = 8 (0x08) <5 bits>
 *       bsmod = 0 (0x0) <3 bits>
 *       acmod = 7 (0x7) <3 bits>
 *       lfeon = 1 (0x1) <1 bits>
 *       bit_rate_code = 15 (0x0f) <5 bits>
 *       reserved = 0 (0x00) <5 bits>
 *
 * into:
 *      type = dac3
 *       fscod = 0 (0x0) <2 bits> [48 kHz]
 *       bsid = 8 (0x08) <5 bits>
 *       bsmod = 0 (0x0) <3 bits> [Main audio service: complete main (CM)]
 *       acmod = 7 (0x7) <3 bits> [3/2 (L, C, R, SL, SR)]
 *       lfeon = 1 (0x1) <1 bits> [ENABLED]
 *       bit_rate_code = 15 (0x0f) <5 bits> [448 kbit/s]
 *       reserved = 0 (0x00) <5 bits>
 *
 */
void MP4DAc3Atom::Dump(FILE* pFile, uint8_t indent, bool dumpImplicits)
{
  
    MP4BitfieldProperty* fscodProp = ((MP4BitfieldProperty*)m_pProperties[0]);
    MP4BitfieldProperty* bsidProp = ((MP4BitfieldProperty*)m_pProperties[1]);
    MP4BitfieldProperty* bsmodProp = ((MP4BitfieldProperty*)m_pProperties[2]);
    MP4BitfieldProperty* acmodProp = ((MP4BitfieldProperty*)m_pProperties[3]);
    MP4BitfieldProperty* lfeonProp = ((MP4BitfieldProperty*)m_pProperties[4]);
    MP4BitfieldProperty* brcProp = ((MP4BitfieldProperty*)m_pProperties[5]);
    MP4BitfieldProperty* resProp = ((MP4BitfieldProperty*)m_pProperties[6]);
    
    Indent(pFile, indent++);

    fprintf(pFile, "type = dac3\n");

    if (fscodProp) { 
        uint64_t fscod = 0xFF;
        const char* fscodString; 
        const char* fscods[] = {
            "48", "44.1", "32", "Reserved",
        };

        Indent(pFile, indent);

        fscod = fscodProp->GetValue();

        if (fscod < (sizeof(fscods) / sizeof(fscods[0]))) {
            fscodString = fscods[fscod];
        } else {
            fscodString = "Invalid value";
        }

        uint8_t hexWidth = fscodProp->GetNumBits() / 4;
        if (hexWidth == 0 || (fscodProp->GetNumBits() % 4)) {
            hexWidth++;
        }

        fprintf(pFile, 
                "fscod = %" PRIu64 " (0x%0*" PRIx64 ") <%u bits> [%s kHz]\n",
                fscod, (int)hexWidth, fscod, fscodProp->GetNumBits(), fscodString);
    }
    if (bsidProp)  bsidProp->Dump(pFile, indent, dumpImplicits);

    if (bsmodProp) { 
        uint64_t bsmod = 0xFF;
        const char* bsmodString; 
        const char* bsmods[] = {
            "Main audio service: complete main (CM)",
            "Main audio srrvice: music and effects (ME)",
            "Associated service: visually impaired (VI)",
            "Associated service: hearing impaired (HI)",
            "Associated service: dialogue (D)",
            "Associated service: commentary (C)",
            "Associated service: emergency (E)",
            "Associated service: voice over (VO) or Main audio service: karaoke",
        };

        Indent(pFile, indent);

        bsmod = bsmodProp->GetValue();

        if (bsmod < (sizeof(bsmods) / sizeof(bsmods[0]))) {
            bsmodString = bsmods[bsmod];
        } else {
            bsmodString = "Invalid value";
        }

        uint8_t hexWidth = bsmodProp->GetNumBits() / 4;
        if (hexWidth == 0 || (bsmodProp->GetNumBits() % 4)) {
            hexWidth++;
        }

        fprintf(pFile, 
                "bsmod = %" PRIu64 " (0x%0*" PRIx64 ") <%u bits> [%s]\n",
                bsmod, (int)hexWidth, bsmod, bsmodProp->GetNumBits(), bsmodString);
    }
    
    if (acmodProp) { 
        uint64_t acmod = 0xFF;
        const char* acmodString; 

        const char* acmods[] = {
            "1 + 1 (Ch1, Ch2)",
            "1/0 (C)",
            "2/0 (L, R)",
            "3/0 (L, C, R)",
            "2/1 (L, R, S)",
            "3/1 (L, C, R, S)",
            "2/2 (L, R, SL, SR)",
            "3/2 (L, C, R, SL, SR)",
        };

        Indent(pFile, indent);

        acmod = acmodProp->GetValue();

        if (acmod < (sizeof(acmods) / sizeof(acmods[0]))) {
            acmodString = acmods[acmod];
        } else {
            acmodString = "Invalid value";
        }

        uint8_t hexWidth = acmodProp->GetNumBits() / 4;
        if (hexWidth == 0 || (acmodProp->GetNumBits() % 4)) {
            hexWidth++;
        }

        fprintf(pFile, 
                "acmod = %" PRIu64 " (0x%0*" PRIx64 ") <%u bits> [%s]\n",
                acmod, (int)hexWidth, acmod, acmodProp->GetNumBits(), acmodString);
    }

    if (lfeonProp) {
        uint64_t lfeon = lfeonProp->GetValue();
        uint8_t hexWidth = lfeonProp->GetNumBits() / 4;
        
        Indent(pFile, indent);

        if (hexWidth == 0 || (lfeonProp->GetNumBits() % 4)) {
            hexWidth++;
        }
        
        fprintf(pFile, 
                "lfeon = %" PRIu64 " (0x%0*" PRIx64 ") <%u bits> [%s]\n",
                lfeon, (int)hexWidth, lfeon, 
                lfeonProp->GetNumBits(), lfeon ? "ENABLED" : "DISABLED"); 
    }
    
    if (brcProp) {
        uint32_t bit_rate_codes[] = {
            32,
            40, 
            48, 
            56, 
            64,
            80,
            96, 
            112, 
            128, 
            160, 
            192, 
            224, 
            256, 
            320, 
            384, 
            448, 
            512, 
            576, 
            640,
        };
        uint64_t bit_rate_code = brcProp->GetValue();
        uint32_t bit_rate;

        Indent(pFile, indent);

        if (bit_rate_code < (sizeof(bit_rate_codes) / sizeof(bit_rate_codes[0]))) {
            bit_rate = bit_rate_codes[bit_rate_code];
        } else {
            bit_rate = 0;
        }

        uint8_t hexWidth = brcProp->GetNumBits() / 4;
        if (hexWidth == 0 || (brcProp->GetNumBits() % 4)) {
            hexWidth++;
        }
        
        fprintf(pFile, 
                "bit_rate_code = %" PRIu64 " (0x%0*" PRIx64 ") <%u bits> [%" PRIu32 " kbit/s]\n",
                bit_rate_code, (int)hexWidth, bit_rate_code, 
                brcProp->GetNumBits(), bit_rate); 
    }
    if (resProp) resProp->Dump(pFile, indent, dumpImplicits);
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
