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

/*!
 * @file node_tree.h
 * @brief Implementation of a node tree
 */

#pragma once

// Internal includes
#include <vector>
#include <memory>
#include <cstdint>
#include <numeric>
#include <utility>

// System includes
#include "ilo/version.h"

namespace ilo {
/*!
 * @brief All nodes in a NodeTree implement the Node. NodeTree is the entry point to the tree
 * implementation.
 *
 * The Node contains most of the business logic of a NodeTree. Nodes can either be empty (root node,
 * i.e. NodeTree instance) or encapsulate a value/item (i.e. Elements). @see NodeTree for details.
 *
 * \ingroup NodeTree
 */
template <class itemType, class elementType>
struct Node {
  virtual ~Node() {}

  //! Access element of node at index
  elementType& operator[](size_t i) {
    if (children.size() > i) {
      return *children[i];
    }
    throw std::runtime_error("tree index out of bounds");
  }

  //! Access element of node at index
  const elementType& operator[](size_t i) const {
    if (children.size() > i) {
      return *children[i];
    }
    throw std::runtime_error("tree index out of bounds");
  }

  //! Add a new child at the end of the node
  elementType& addChild(const itemType& ch) {
    children.push_back(std::unique_ptr<elementType>(new elementType(ch)));
    return *children.back();
  }

  //! Remove child from the node at a given index
  void removeChild(size_t at) {
    if (children.size() <= at) {
      throw std::runtime_error("tree index out of bounds");
    }
    children.erase(children.begin() + at);
  }

  //! Get the number of direct children of this node
  size_t childCount() const { return children.size(); }

 private:
  std::vector<std::unique_ptr<elementType>> children;
};

/*!
 * @brief All nodes of a NodeTree except the root are Elements. @see NodeTree
 *
 * \ingroup NodeTree
 */
template <class type>
struct Element : public Node<type, Element<type>> {
  //! Create a new element
  Element(const type& i) : item(i) {}

  //! Item stored in this element
  type item;
};

/*!
 * @brief The NodeTree is the entry point for the node tree implementation.
 *
 * It lends its business logic from Node and has children of type Element, which contain the data in
 * the tree structure. Elements can also have children of type Element and thereby span data (items)
 * in a node tree.
 *
 * The following Diagram shows a tree example with a Root Node and some children. Note that all
 * boxes derive from the base class/struct Node. <PRE>
 * +------------+
 * | Root Node  |
 * | [NodeTree] |
 * +------------+
 *       |
 * +------------+    +------------+
 * | Child 1    |    | Child 1.1  |
 * | [Element]  |----| [Element]  |---- ...
 * +------------+    +------------+
 *       |
 * +------------+    +------------+    +------------+
 * | Child 2    |    | Child 2.1  |    | Child 2.2  |
 * | [Element]  |----| [Element]  |----| [Element]  |---- ...
 * +------------+    +------------+    +------------+
 * </PRE>
 *
 * \ingroup NodeTree
 */
template <class type>
struct NodeTree : public Node<type, Element<type>> {
  //! Definition of a node type
  typedef Node<type, Element<type>> NodeType;

  NodeTree() {}

  NodeTree(const NodeTree&) = delete;
};

/*! \defgroup NodeTree Node tree implementation and helper patterns
 *  @{
 *  Implementation of a node tree. @see NodeTree
 */

//! Visitor that calls predicate p on each node in the node tree starting from node
template <class type, class predicate>
void visitAllOf(
    const Node<type, Element<type>>& node, predicate p,
    decltype(std::declval<predicate>()(std::declval<Element<type>&>()), 1) /*sfinae*/ = 1) {
  const Element<type>* tn = dynamic_cast<const Element<type>*>(&node);
  if (tn != nullptr) {
    p(*tn);
  }

  for (size_t i = 0; i < node.childCount(); ++i) {
    const Element<type>& c = node[i];
    visitAllOf(c, p);
  }
}

//! Visitor that calls predicate p on each node in the node tree starting from node with the current
//! hierarchy level
template <class type, class predicate>
void visitAllOf(const Node<type, Element<type>>& node, predicate p, int32_t level = -1,
                decltype(std::declval<predicate>()(std::declval<Element<type>&>(),
                                                   std::declval<int32_t>()),
                         1) /*sfinae*/
                = 1) {
  const Element<type>* tn = dynamic_cast<const Element<type>*>(&node);
  if (tn != nullptr) {
    ++level;
    p(*tn, level);
  }
  for (size_t i = 0; i < node.childCount(); ++i) {
    const Element<type>& c = node[i];
    visitAllOf(c, p, level);
  }
  if (tn != nullptr) {
    --level;
  }
}

//! Visitor that calls predicate p on each node in the node tree starting from node until p return
//! true
template <class type, class predicate>
bool visitUntil(
    const Node<type, Element<type>>& node, predicate p,
    decltype(std::declval<predicate>()(std::declval<Element<type>&>()), 1) /*sfinae*/ = 1) {
  const Element<type>* tn = dynamic_cast<const Element<type>*>(&node);
  if (tn != nullptr) {
    if (p(*tn)) {
      return true;
    }
  }
  for (size_t i = 0; i < node.childCount(); ++i) {
    const Element<type>& c = node[i];
    {
      if (visitUntil(c, p)) {
        return true;
      }
    }
  }
  return false;
}

//! Visitor that calls predicate p on each node in the node tree starting from node with the current
//! hierarchy level until p return true
template <class type, class predicate>
bool visitUntil(const Node<type, Element<type>>& node, predicate p, int32_t level = -1,
                decltype(std::declval<predicate>()(std::declval<Element<type>&>(),
                                                   std::declval<int32_t>()),
                         1) /*sfinae*/
                = 1) {
  const Element<type>* tn = dynamic_cast<const Element<type>*>(&node);
  if (tn != nullptr) {
    ++level;
    if (p(*tn, level)) {
      --level;
      return true;
    }
  }
  for (size_t i = 0; i < node.childCount(); ++i) {
    const Element<type>& c = node[i];
    {
      if (visitUntil(c, p, level)) {
        if (tn != nullptr) {
          --level;
        }
        return true;
      }
    }
  }
  if (tn != nullptr) {
    --level;
  }
  return false;
}

//! Visitor that calls predicate p on each node in the node tree starting from node and proceeds to
//! children unless p returns true
template <class type, class predicate>
bool visitChildrenUnless(
    const Node<type, Element<type>>& node, predicate p,
    decltype(std::declval<predicate>()(std::declval<Element<type>&>()), 1) /*sfinae*/ = 1) {
  const Element<type>* tn = dynamic_cast<const Element<type>*>(&node);
  if (tn == nullptr || !p(*tn)) {
    for (size_t i = 0; i < node.childCount(); ++i) {
      const Element<type>& c = node[i];
      visitChildrenUnless(c, p);
    }
  }
  return false;
}

/**@}*/
}  // namespace ilo
