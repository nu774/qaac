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
 * Content: root box class - start for box tree
 */

#pragma once

// System includes
#include <cstdio>
#include <memory>
#include <functional>

// external include
#include "ilo/string_utils.h"
#include "ilo/node_tree.h"
#include "ilo/bytebuffertools.h"

// Internal includes
#include "box/box.h"
#include "box/containerbox.h"
#include "common/logging.h"
#include "mmtisobmff/types.h"

namespace mmt {
namespace isobmff {
using BoxItem = std::shared_ptr<box::IBox>;
using BoxElement = ilo::Element<BoxItem>;
using BoxNode = ilo::Node<BoxItem, BoxElement>;
using BoxTree = ilo::NodeTree<BoxItem>;

template <class type>
std::vector<std::shared_ptr<type>> findAllBoxesWithType(const BoxNode& tree) {
  std::vector<std::shared_ptr<type>> boxlist;
  visitAllOf(tree, [&boxlist](const BoxElement& e) mutable {
    auto value = std::dynamic_pointer_cast<type>(e.item);
    if (value) {
      boxlist.push_back(value);
    }
  });
  return boxlist;
}

template <class type>
std::vector<std::reference_wrapper<const BoxElement>> findAllElementsWithBoxType(
    const BoxNode& tree) {
  std::vector<std::reference_wrapper<const BoxElement>> nodelist;
  visitAllOf(tree, [&nodelist](const BoxElement& e) mutable {
    auto value = std::dynamic_pointer_cast<type>(e.item);
    if (value) {
      nodelist.push_back(ref(e));
    }
  });
  return nodelist;
}

template <class type>
std::shared_ptr<type> findFirstBoxWithType(const BoxNode& tree) {
  std::shared_ptr<type> box(nullptr);
  visitUntil(tree, [&box](const BoxElement& e) mutable {
    auto value = std::dynamic_pointer_cast<type>(e.item);
    if (value) {
      box = value;
      return true;
    }
    return false;
  });
  return box;
}

template <class type>
std::vector<std::shared_ptr<type>> findAllBoxesWithFourccAndType(const BoxNode& tree,
                                                                 const ilo::Fourcc& fcc) {
  std::vector<std::shared_ptr<type>> boxlist;
  visitAllOf(tree, [&boxlist, &fcc](const BoxElement& e) mutable {
    if (e.item->type() == fcc) {
      auto value = std::dynamic_pointer_cast<type>(e.item);
      if (value != nullptr) {
        boxlist.push_back(value);
      }
    }
  });
  return boxlist;
}

template <class type>
std::vector<std::reference_wrapper<const BoxElement>> findAllElementsWithFourccAndBoxType(
    const BoxNode& tree, const ilo::Fourcc& fcc) {
  std::vector<std::reference_wrapper<const BoxElement>> nodelist;
  visitAllOf(tree, [&nodelist, &fcc](const BoxElement& e) mutable {
    if (e.item->type() == fcc) {
      if (std::dynamic_pointer_cast<type>(e.item) != nullptr) {
        nodelist.push_back(ref(e));
      }
    }
  });
  return nodelist;
}

template <class type>
std::vector<std::reference_wrapper<const BoxElement>> findAllElementsWithFourccAndBoxType(
    const BoxNode& tree, const ilo::Fourcc& fcc, const int32_t level) {
  std::vector<std::reference_wrapper<const BoxElement>> nodelist;
  visitAllOf(tree, [&nodelist, &fcc, &level](const BoxElement& e, int32_t currLevel) mutable {
    if (e.item->type() == fcc && currLevel <= level) {
      if (std::dynamic_pointer_cast<type>(e.item) != nullptr) {
        nodelist.push_back(ref(e));
      }
    }
  });
  return nodelist;
}

template <class type>
std::reference_wrapper<const BoxElement> findFirstElementWithFourccAndBoxType(
    const BoxNode& tree, const ilo::Fourcc& fcc) {
  std::vector<std::reference_wrapper<const BoxElement>> nodelist;
  visitUntil(tree, [&nodelist, &fcc](const BoxElement& e) mutable {
    if (e.item->type() == fcc) {
      if (std::dynamic_pointer_cast<type>(e.item) != nullptr) {
        nodelist.push_back(ref(e));
        return true;
      }
    }
    return false;
  });
  ILO_ASSERT(nodelist.size() == 1, "Box element %s not found in tree", ilo::toString(fcc).c_str());
  return nodelist.at(0);
}

template <class type>
std::shared_ptr<type> findFirstBoxWithFourccAndType(const BoxNode& tree, const ilo::Fourcc& fcc) {
  std::shared_ptr<type> box(nullptr);
  visitUntil(tree, [&box, &fcc](const BoxElement& e) {
    if (e.item->type() == fcc) {
      auto value = std::dynamic_pointer_cast<type>(e.item);
      if (value) {
        box = value;
        return true;
      }
    }
    return false;
  });
  return box;
}

template <class box_type, class string_type>
std::shared_ptr<box_type> findChildBoxByPathTokens(std::reference_wrapper<const BoxElement> elem,
                                                   std::deque<string_type> tokens) {
  for (size_t i = 0U; i < elem.get().childCount(); ++i) {
    if (elem.get()[i].item->type() == ilo::toFcc(tokens[0])) {
      tokens.pop_front();
      if (tokens.size() == 0) {
        return std::dynamic_pointer_cast<box_type>(elem.get()[i].item);
      } else {
        return findChildBoxByPathTokens<box_type>(
            std::reference_wrapper<const BoxElement>(ref(elem.get()[i])), tokens);
      }
    }
  }
  return std::shared_ptr<box_type>(nullptr);
}

//! finds the first box in the tree with a matching path specification (e.g. "trak/mdia/hdlr")
template <class box_type>
std::shared_ptr<box_type> findFirstBoxWithPathAndType(const BoxNode& tree,
                                                      const std::string& path) {
  auto tokens = ilo::tokenize(path, '/');
  ILO_ASSERT(tokens.size() > 0, "Path specification invalid: %s", path.c_str());
  auto root = findFirstElementWithFourccAndBoxType<box::IBox>(tree, ilo::toFcc(tokens[0]));
  tokens.pop_front();
  return findChildBoxByPathTokens<box_type>(root, tokens);
}

inline void prettyPrintTree(const BoxNode& tree) {
  ilo::visitAllOf(tree, [](const BoxElement& e, int32_t level) {
    std::string indent(level, '\t');
    std::printf("%s", indent.c_str());
    std::printf("%s (%llu)\n", ilo::toString(e.item->type()).c_str(),
                (unsigned long long)e.item->size());
  });
}

inline uint64_t treeSizeWithoutMdatPayloadInBytes(const BoxNode& tree) {
  uint64_t treeSize = 0;

  for (size_t nodeNr = 0; nodeNr < tree.childCount(); ++nodeNr) {
    treeSize += tree[nodeNr].item->size();
    if (tree[nodeNr].item->type() == ilo::toFcc("mdat")) {
      auto mdatBoxHeaderLength = 8;
      if (tree[nodeNr].item->had64BitSizeInInput()) {
        mdatBoxHeaderLength = 16;
      }
      treeSize -= tree[nodeNr].item->size() - mdatBoxHeaderLength;
    }
  }

  return treeSize;
}

inline void serializeTree(const BoxNode& tree, ilo::ByteBuffer& writeBuffer,
                          ilo::ByteBuffer::iterator& iter) {
  // Write all nodes to buffer
  ilo::visitAllOf(tree,
                  [&writeBuffer, &iter](const BoxElement& e) { e.item->write(writeBuffer, iter); });
}

uint64_t updateSizeAndReturnElementSize(const BoxElement& currentElement);

uint64_t updateSizeAndReturnTotalSize(const BoxTree& tree);

SOverheadInfo calculateOverhead(const BoxTree& tree);

}  // namespace isobmff
}  // namespace mmt
