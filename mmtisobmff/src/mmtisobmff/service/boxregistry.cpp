/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2016 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1. INTRODUCTION

The "Fraunhofer FDK MPEG-H Software" is software that implements the ISO/MPEG
MPEG-H 3D Audio standard for digital audio or related system features. Patent
licenses for necessary patent claims for the Fraunhofer FDK MPEG-H Software
(including those of Fraunhofer), for the use in commercial products and
services, may be obtained from the respective patent owners individually and/or
from Via LA (www.via-la.com).

Fraunhofer supports the development of MPEG-H products and services by offering
additional software, documentation, and technical advice. In addition, it
operates the MPEG-H Trademark Program to ease interoperability testing of end-
products. Please visit www.mpegh.com for more information.

2. COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

* You must retain the complete text of this software license in redistributions
of the Fraunhofer FDK MPEG-H Software or your modifications thereto in source
code form.

* You must retain the complete text of this software license in the
documentation and/or other materials provided with redistributions of
the Fraunhofer FDK MPEG-H Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of
the Fraunhofer FDK MPEG-H Software and your modifications thereto to recipients
of copies in binary form.

* The name of Fraunhofer may not be used to endorse or promote products derived
from the Fraunhofer FDK MPEG-H Software without prior written permission.

* You may not charge copyright license fees for anyone to use, copy or
distribute the Fraunhofer FDK MPEG-H Software or your modifications thereto.

* Your modified versions of the Fraunhofer FDK MPEG-H Software must carry
prominent notices stating that you changed the software and the date of any
change. For modified versions of the Fraunhofer FDK MPEG-H Software, the term
"Fraunhofer FDK MPEG-H Software" must be replaced by the term "Third-Party
Modified Version of the Fraunhofer FDK MPEG-H Software".

3. No PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software. You may use this Fraunhofer FDK MPEG-H Software or modifications
thereto only for purposes that are authorized by appropriate patent licenses.

4. DISCLAIMER

This Fraunhofer FDK MPEG-H Software is provided by Fraunhofer on behalf of the
copyright holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5. CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - MPEG-H FDK
Am Wolfsmantel 33
91058 Erlangen, Germany
www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
-----------------------------------------------------------------------------*/

/*
 * Project: MPEG-4 ISO Base Media File Format (ISO BMFF) library
 * Content: box registry
 */

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "boxregistry.h"

#define ADD_BOX_REGISTRY(RegEntry)        \
  extern box::CBoxRegistryEntry RegEntry; \
  boxes[RegEntry.fcc] = RegEntry

namespace mmt {
namespace isobmff {
static void registerBoxes(CBoxRegistry::CRegistryMap& boxes) {
  ADD_BOX_REGISTRY(ftypBoxRegistryEntry);
  ADD_BOX_REGISTRY(stypBoxRegistryEntry);
  ADD_BOX_REGISTRY(moofBoxRegistryEntry);
  ADD_BOX_REGISTRY(moovBoxRegistryEntry);
  ADD_BOX_REGISTRY(trakBoxRegistryEntry);
  ADD_BOX_REGISTRY(mdiaBoxRegistryEntry);
  ADD_BOX_REGISTRY(edtsBoxRegistryEntry);
  ADD_BOX_REGISTRY(avcCBoxRegistryEntry);
  ADD_BOX_REGISTRY(btrtBoxRegistryEntry);
  ADD_BOX_REGISTRY(hdlrBoxRegistryEntry);
  ADD_BOX_REGISTRY(hvcCBoxRegistryEntry);
  ADD_BOX_REGISTRY(mdhdBoxRegistryEntry);
  ADD_BOX_REGISTRY(mdatBoxRegistryEntry);
  ADD_BOX_REGISTRY(mfhdBoxRegistryEntry);
  ADD_BOX_REGISTRY(mhaCBoxRegistryEntry);
  ADD_BOX_REGISTRY(esdsBoxRegistryEntry);
  ADD_BOX_REGISTRY(mmpuBoxRegistryEntry);
  ADD_BOX_REGISTRY(mvhdBoxRegistryEntry);
  ADD_BOX_REGISTRY(sidxBoxRegistryEntry);
  ADD_BOX_REGISTRY(smhdBoxRegistryEntry);
  ADD_BOX_REGISTRY(vmhdBoxRegistryEntry);
  ADD_BOX_REGISTRY(stcoBoxRegistryEntry);
  ADD_BOX_REGISTRY(co64BoxRegistryEntry);
  ADD_BOX_REGISTRY(stscBoxRegistryEntry);
  ADD_BOX_REGISTRY(stz2BoxRegistryEntry);
  ADD_BOX_REGISTRY(stszBoxRegistryEntry);
  ADD_BOX_REGISTRY(sttsBoxRegistryEntry);
  ADD_BOX_REGISTRY(stssBoxRegistryEntry);
  ADD_BOX_REGISTRY(tfdtBoxRegistryEntry);
  ADD_BOX_REGISTRY(tfhdBoxRegistryEntry);
  ADD_BOX_REGISTRY(tkhdBoxRegistryEntry);
  ADD_BOX_REGISTRY(trunBoxRegistryEntry);
  ADD_BOX_REGISTRY(trexBoxRegistryEntry);
  ADD_BOX_REGISTRY(minfBoxRegistryEntry);
  ADD_BOX_REGISTRY(dinfBoxRegistryEntry);
  ADD_BOX_REGISTRY(stblBoxRegistryEntry);
  ADD_BOX_REGISTRY(mvexBoxRegistryEntry);
  ADD_BOX_REGISTRY(trafBoxRegistryEntry);
  ADD_BOX_REGISTRY(drefBoxRegistryEntry);
  ADD_BOX_REGISTRY(urlBoxRegistryEntry);
  ADD_BOX_REGISTRY(stsdBoxRegistryEntry);
  ADD_BOX_REGISTRY(cttsBoxRegistryEntry);
  ADD_BOX_REGISTRY(mhm1BoxRegistryEntry);
  ADD_BOX_REGISTRY(mhm2BoxRegistryEntry);
  ADD_BOX_REGISTRY(mha1BoxRegistryEntry);
  ADD_BOX_REGISTRY(mha2BoxRegistryEntry);
  ADD_BOX_REGISTRY(hvc1BoxRegistryEntry);
  ADD_BOX_REGISTRY(hev1BoxRegistryEntry);
  ADD_BOX_REGISTRY(avc1BoxRegistryEntry);
  ADD_BOX_REGISTRY(avc3BoxRegistryEntry);
  ADD_BOX_REGISTRY(mp4aBoxRegistryEntry);
  ADD_BOX_REGISTRY(elstBoxRegistryEntry);
  ADD_BOX_REGISTRY(sgpdBoxRegistryEntry);
  ADD_BOX_REGISTRY(sbgpBoxRegistryEntry);
  ADD_BOX_REGISTRY(udtaBoxRegistryEntry);
  ADD_BOX_REGISTRY(ludtBoxRegistryEntry);
  ADD_BOX_REGISTRY(tlouBoxRegistryEntry);
  ADD_BOX_REGISTRY(alouBoxRegistryEntry);
  ADD_BOX_REGISTRY(iodsBoxRegistryEntry);
  ADD_BOX_REGISTRY(jxsmBoxRegistryEntry);
  ADD_BOX_REGISTRY(jpviBoxRegistryEntry);
  ADD_BOX_REGISTRY(jxplBoxRegistryEntry);
  ADD_BOX_REGISTRY(colrBoxRegistryEntry);
  ADD_BOX_REGISTRY(jpvsBoxRegistryEntry);
  ADD_BOX_REGISTRY(jxsHBoxRegistryEntry);
  ADD_BOX_REGISTRY(mhaPBoxRegistryEntry);
  ADD_BOX_REGISTRY(vvc1BoxRegistryEntry);
  ADD_BOX_REGISTRY(vvi1BoxRegistryEntry);
  ADD_BOX_REGISTRY(vvcCBoxRegistryEntry);
}

CBoxRegistry::CBoxRegistry() {
  registerBoxes(m_boxes);
}

bool CBoxRegistry::isContainer(const std::shared_ptr<box::IBox>& box) const {
  auto boxreg = m_boxes.find(box->type());
  if (boxreg != m_boxes.end())
    return boxreg->second.containerType == box::CContainerType::isContainer;
  return false;
}
}  // namespace isobmff
}  // namespace mmt
