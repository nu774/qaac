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
 * Content: factory class(es)
 */

// Needed to make PRIu64 from inttypes.h for older C-Libs
#define __STDC_FORMAT_MACROS

// System includes
#include <stdexcept>
#include <inttypes.h>

// External includes
#include "ilo/node_tree.h"
#include "ilo/common_types.h"
#include "ilo/string_utils.h"

// Internal includes
#include "factory.h"
#include "boxregistry.h"
#include "servicesingleton.h"
#include "common/logging.h"
#include "common/bytebuffertools_extension.h"
#include "box/invalidbox.h"
#include "box/unknownbox.h"
#include "box/containerbox.h"

namespace mmt {
namespace isobmff {
std::shared_ptr<box::IBox> CBoxFactory::createBox(
    ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end) const {
  BoxSizeType boxSizeType = tools::getBoxSizeAndType(begin, end);

  auto registry = CServiceLocatorSingleton::instance().lock()->getService<IBoxRegistry>().lock();
  ILO_LOG_INFO("creating box of type %s with size %" PRIu64,
               ilo::toString(boxSizeType.type).c_str(), boxSizeType.size);

  box::CBoxRegistryEntry registryEntry;
  try {
    registryEntry = registry->entry(boxSizeType.type);
  } catch (const std::exception&) {
    ILO_LOG_WARNING("unknown box (%s) - skipping", ilo::toString(boxSizeType.type).c_str());
    return std::make_shared<box::CUnknownBox>(begin, end);
  }

  auto boxStart = begin;  // need later in case of a parsing error to create the invalid box
  try {
    return registryEntry.parseCreate(begin, end);
  } catch (const std::exception&) {
    ILO_LOG_WARNING("error at parsing (%s) - skipping", ilo::toString(boxSizeType.type).c_str());
    begin = boxStart;
    return std::make_shared<box::CInvalidBox>(begin, end);
  }
}

std::shared_ptr<box::IBox> CBoxFactory::createBox(
    const box::CBox::SBoxWriteConfig& boxWriteConfig) const {
  const ilo::Fourcc& fcc = boxWriteConfig.getType();

  auto registry = CServiceLocatorSingleton::instance().lock()->getService<IBoxRegistry>().lock();

  ILO_LOG_INFO("creating box of type %s", ilo::toString(fcc).c_str());

  box::CBoxRegistryEntry registryEntry;

  try {
    registryEntry = registry->entry(fcc);
  } catch (const std::exception&) {
    ILO_LOG_WARNING("unknown box (%s)", ilo::toString(fcc).c_str());
    auto& config = static_cast<const box::CUnknownBox::SUnknownBoxWriteConfig&>(boxWriteConfig);
    return std::make_shared<box::CUnknownBox>(config);
  }

  return registryEntry.writeCreate(boxWriteConfig);
}

static ilo::ByteBuffer::const_iterator boxEnd(const ilo::ByteBuffer::const_iterator& begin,
                                              const ilo::ByteBuffer::const_iterator& end) {
  BoxSizeType boxSizeType = tools::getBoxSizeAndType(begin, end);

  if (static_cast<size_t>(end - begin) < boxSizeType.size) {
    ILO_LOG_WARNING("Box size is bigger than remaining buffer - reading might fail");
    return end;
  }

  return begin + static_cast<size_t>(boxSizeType.size);
}

void CNodeFactory::createNode(BoxTree::NodeType& addTo, ilo::ByteBuffer::const_iterator& begin,
                              const ilo::ByteBuffer::const_iterator& end) const {
  auto chopEnd = boxEnd(begin, end);
  ilo::ByteBuffer::const_iterator chopBegin = begin;

  auto boxfac = CServiceLocatorSingleton::instance().lock()->getService<IBoxFactory>().lock();
  auto box = boxfac->createBox(begin, chopEnd);
  if (box->size() != static_cast<uint64_t>(chopEnd - chopBegin)) {
    ILO_LOG_WARNING("Box size mismatch");
  }

  BoxTree::NodeType& current_node = addTo.addChild(box);

  auto registry = CServiceLocatorSingleton::instance().lock()->getService<IBoxRegistry>().lock();
  if (begin != chopEnd) {
    ILO_ASSERT(registry->isContainer(box), "box was not fully parsed: %s",
               ilo::toString(box->type()).c_str());
    if (!registry->isContainer(box)) {
      ILO_LOG_WARNING("box was not fully parsed: %s", ilo::toString(box->type()).c_str());
    } else {
      ILO_LOG_INFO("Container found");
      while (begin != chopEnd) {
        createNode(current_node, begin, chopEnd);
      }
    }
  }
  begin = chopEnd;
}

std::reference_wrapper<BoxElement> CNodeFactory::createNode(
    BoxTree::NodeType& addTo, const BoxWriteConfig& boxWriteConfig) const {
  auto boxfac = CServiceLocatorSingleton::instance().lock()->getService<IBoxFactory>().lock();
  auto box = boxfac->createBox(boxWriteConfig);
  return ref(addTo.addChild(box));
}

void CNodeFactory::replaceNode(BoxElement& toBeReplaced,
                               const BoxWriteConfig& boxWriteConfig) const {
  auto boxfac = CServiceLocatorSingleton::instance().lock()->getService<IBoxFactory>().lock();
  auto box = boxfac->createBox(boxWriteConfig);
  toBeReplaced.item = box;
}

}  // namespace isobmff
}  // namespace mmt
