/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "../nsSBCharSetProber.h"
#include "../nsSBCharSetProber-generated.h"
#include "../nsLanguageDetector.h"
#include "../nsLanguageDetector-generated.h"

/********* Language model for: Slovak *********/

/**
 * Generated by BuildLangModel.py
 * On: 2022-12-15 00:27:35.474305
 **/

/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const unsigned char Iso_8859_2_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  1, 20, 15, 12,  2, 30, 28, 16,  4, 17,  9, 10, 11,  3,  0, /* 4X */
   13, 40,  5,  6,  7, 14,  8, 36, 37, 21, 19,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  1, 20, 15, 12,  2, 30, 28, 16,  4, 17,  9, 10, 11,  3,  0, /* 6X */
   13, 40,  5,  6,  7, 14,  8, 36, 37, 21, 19,SYM,SYM,SYM,SYM,CTR, /* 7X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 8X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 9X */
  SYM, 67,SYM, 51,SYM, 31, 54,SYM,SYM, 27, 59, 32, 68,SYM, 29, 69, /* AX */
  SYM, 70,SYM, 51,SYM, 31, 54,SYM,SYM, 27, 59, 32, 71,SYM, 29, 72, /* BX */
   43, 18, 63, 56, 34, 41, 50, 48, 25, 24, 62, 42, 45, 23, 57, 38, /* CX */
   73, 65, 39, 33, 35, 58, 44,SYM, 46, 49, 26, 74, 47, 22, 75, 55, /* DX */
   43, 18, 63, 56, 34, 41, 50, 48, 25, 24, 62, 42, 45, 23, 57, 38, /* EX */
   76, 65, 39, 33, 35, 58, 44,SYM, 46, 49, 26, 77, 47, 22, 78,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Windows_1250_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  1, 20, 15, 12,  2, 30, 28, 16,  4, 17,  9, 10, 11,  3,  0, /* 4X */
   13, 40,  5,  6,  7, 14,  8, 36, 37, 21, 19,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  1, 20, 15, 12,  2, 30, 28, 16,  4, 17,  9, 10, 11,  3,  0, /* 6X */
   13, 40,  5,  6,  7, 14,  8, 36, 37, 21, 19,SYM,SYM,SYM,SYM,CTR, /* 7X */
  SYM,ILL,SYM,ILL,SYM,SYM,SYM,SYM,ILL,SYM, 27,SYM, 54, 32, 29, 79, /* 8X */
  ILL,SYM,SYM,SYM,SYM,SYM,SYM,SYM,ILL,SYM, 27,SYM, 54, 32, 29, 80, /* 9X */
  SYM,SYM,SYM, 51,SYM, 81,SYM,SYM,SYM,SYM, 59,SYM,SYM,SYM,SYM, 82, /* AX */
  SYM,SYM,SYM, 51,SYM,SYM,SYM,SYM,SYM, 83, 59,SYM, 31,SYM, 31, 84, /* BX */
   43, 18, 63, 56, 34, 41, 50, 48, 25, 24, 62, 42, 45, 23, 57, 38, /* CX */
   85, 65, 39, 33, 35, 58, 44,SYM, 46, 49, 26, 86, 47, 22, 87, 55, /* DX */
   43, 18, 63, 56, 34, 41, 50, 48, 25, 24, 62, 42, 45, 23, 57, 38, /* EX */
   88, 65, 39, 33, 35, 58, 44,SYM, 46, 49, 26, 89, 47, 22, 90,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Ibm852_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  1, 20, 15, 12,  2, 30, 28, 16,  4, 17,  9, 10, 11,  3,  0, /* 4X */
   13, 40,  5,  6,  7, 14,  8, 36, 37, 21, 19,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  1, 20, 15, 12,  2, 30, 28, 16,  4, 17,  9, 10, 11,  3,  0, /* 6X */
   13, 40,  5,  6,  7, 14,  8, 36, 37, 21, 19,SYM,SYM,SYM,SYM,CTR, /* 7X */
   48, 47, 24, 63, 34, 49, 50, 48, 51, 42, 58, 58, 57, 91, 34, 50, /* 8X */
   24, 41, 41, 35, 44, 31, 31, 54, 54, 44, 47, 32, 32, 51,SYM, 25, /* 9X */
   18, 23, 33, 26, 92, 93, 29, 29, 62, 62,SYM, 94, 25, 59,SYM,SYM, /* AX */
  SYM,SYM,SYM,SYM,SYM, 18, 63, 45, 59,SYM,SYM,SYM,SYM, 95, 96,SYM, /* BX */
  SYM,SYM,SYM,SYM,SYM,SYM, 56, 56,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* CX */
   97, 98, 38, 42, 38, 39, 23, 57, 45,SYM,SYM,SYM,SYM, 99, 49,SYM, /* DX */
   33, 55, 35, 65, 65, 39, 27, 27, 43, 26, 43,100, 22, 22,101,SYM, /* EX */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,102, 46, 46,SYM,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const unsigned char Mac_Centraleurope_CharToOrderMap[] =
{
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,RET,CTR,CTR,RET,CTR,CTR, /* 0X */
  CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR,CTR, /* 1X */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, /* 2X */
  NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,NUM,SYM,SYM,SYM,SYM,SYM,SYM, /* 3X */
  SYM,  1, 20, 15, 12,  2, 30, 28, 16,  4, 17,  9, 10, 11,  3,  0, /* 4X */
   13, 40,  5,  6,  7, 14,  8, 36, 37, 21, 19,SYM,SYM,SYM,SYM,SYM, /* 5X */
  SYM,  1, 20, 15, 12,  2, 30, 28, 16,  4, 17,  9, 10, 11,  3,  0, /* 6X */
   13, 40,  5,  6,  7, 14,  8, 36, 37, 21, 19,SYM,SYM,SYM,SYM,CTR, /* 7X */
   34, 53, 53, 24,103, 44, 47, 18,104, 25, 34, 25, 50, 50, 24,105, /* 8X */
  106, 38, 23, 38, 52, 52,107, 33,108, 35, 44, 60, 26, 45, 45, 47, /* 9X */
  SYM,SYM, 62,SYM,SYM,SYM,SYM, 55,SYM,SYM,SYM, 62,SYM,SYM,109,110, /* AX */
  111,112,SYM,SYM,113,114,SYM,SYM, 51,115,116, 31, 31, 41, 41, 66, /* BX */
   66, 65,SYM,SYM, 65, 39,SYM,SYM,SYM,SYM,SYM, 39, 58, 60, 58, 64, /* CX */
  SYM,SYM,SYM,SYM,SYM,SYM,SYM,SYM, 64, 43, 43, 46,SYM,SYM, 46,117, /* DX */
  118, 27,SYM,SYM, 27, 54, 54, 18, 32, 32, 23, 29, 29, 61, 33, 35, /* EX */
   61, 49, 26, 49,119,120,121,122, 22, 22,123,124, 51,125,126,SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const int Unicode_Char_size = 88;
static const unsigned int Unicode_CharOrder[] =
{
   65,  1,  66, 20,  67, 15,  68, 12,  69,  2,  70, 30,  71, 28, 72, 16,
   73,  4,  74, 17,  75,  9,  76, 10,  77, 11,  78,  3,  79,  0, 80, 13,
   81, 40,  82,  5,  83,  6,  84,  7,  85, 14,  86,  8,  87, 36, 88, 37,
   89, 21,  90, 19,  97,  1,  98, 20,  99, 15, 100, 12, 101,  2,102, 30,
  103, 28, 104, 16, 105,  4, 106, 17, 107,  9, 108, 10, 109, 11,110,  3,
  111,  0, 112, 13, 113, 40, 114,  5, 115,  6, 116,  7, 117, 14,118,  8,
  119, 36, 120, 37, 121, 21, 122, 19, 193, 18, 196, 34, 201, 24,203, 42,
  205, 23, 211, 33, 212, 35, 218, 26, 221, 22, 225, 18, 228, 34,233, 24,
  235, 42, 237, 23, 243, 33, 244, 35, 250, 26, 253, 22, 268, 25,269, 25,
  270, 38, 271, 38, 313, 41, 314, 41, 317, 31, 318, 31, 327, 39,328, 39,
  340, 43, 341, 43, 352, 27, 353, 27, 356, 32, 357, 32, 381, 29,382, 29,
};


/* Model Table:
 * Total considered sequences: 1391 / 1936
 * - Positive sequences: first 769 (0.9950122209740424)
 * - Probable sequences: next 247 (1016-769) (0.003989961669689679)
 * - Neutral sequences: last 920 (0.000997817356267916)
 * - Negative sequences: 545 (off-ratio)
 * Negative sequences: TODO
 */
static const PRUint8 SlovakLangModel[] =
{
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,3,
   0,1,2,3,0,3,3,3,3,3,0,1,0,0,3,3,2,3,2,0,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,3,3,3,
   0,2,1,3,2,3,3,3,3,3,3,0,0,0,3,3,3,3,2,0,1,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
   0,2,0,3,3,3,3,3,3,3,3,2,0,0,3,3,3,3,1,0,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,2,3,3,3,3,3,
   3,3,3,3,3,3,3,3,3,0,0,3,0,1,2,1,2,0,1,0,1,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,
   0,3,3,3,1,3,3,3,3,2,3,3,0,0,2,3,0,1,2,0,0,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,3,3,
   3,3,3,3,3,3,3,3,3,0,3,3,0,3,2,2,1,2,1,0,2,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,2,3,3,
   2,3,3,2,3,0,2,0,3,2,3,2,1,3,3,0,0,1,2,0,1,0,
  3,3,3,3,3,3,3,3,3,3,3,3,2,2,3,3,3,2,3,3,3,3,
   3,3,3,2,3,2,2,0,2,1,0,3,0,0,2,0,2,2,0,1,1,1,
  3,3,3,3,3,3,3,3,1,3,3,2,2,3,3,3,3,1,3,3,3,3,
   3,3,3,3,3,3,2,2,0,1,0,1,3,3,0,0,3,3,0,1,1,2,
  3,3,3,3,3,3,3,3,3,2,3,3,3,2,3,3,3,2,3,2,1,3,
   3,3,3,2,3,1,1,2,2,2,0,3,1,3,2,0,0,3,0,1,2,1,
  3,3,3,3,3,1,3,3,3,3,3,3,3,3,3,3,3,2,3,2,3,3,
   3,3,3,3,3,3,3,2,3,0,0,3,2,1,2,0,0,2,1,0,0,0,
  3,3,3,3,3,3,3,3,1,3,3,3,3,3,3,3,2,2,3,1,3,3,
   2,3,3,2,3,1,0,1,2,1,0,2,3,3,2,1,1,1,1,0,1,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
   3,3,3,3,3,2,3,3,2,3,0,3,0,3,3,0,0,3,0,3,2,1,
  3,3,3,3,3,3,3,3,2,3,3,2,2,3,3,3,3,1,3,0,1,3,
   2,3,3,2,3,3,1,0,1,1,1,1,3,3,1,0,0,2,0,2,2,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,3,3,3,3,3,
   0,1,2,3,1,3,3,3,2,2,2,1,0,0,1,3,3,1,1,0,1,0,
  3,3,3,3,3,3,3,3,2,3,3,2,2,2,3,3,3,1,2,2,1,3,
   1,3,3,0,3,0,0,0,1,0,1,1,0,0,0,0,0,1,2,0,1,0,
  3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,2,0,1,3,0,2,3,
   3,2,3,1,3,3,1,0,1,3,0,1,1,2,2,0,0,2,2,2,1,3,
  3,3,3,3,3,3,3,3,3,3,2,3,3,3,3,3,3,2,3,3,3,1,
   0,3,1,3,3,3,1,1,2,2,1,2,1,0,1,0,0,1,0,0,1,0,
  0,0,1,3,0,3,3,3,3,3,3,3,3,3,3,3,3,3,0,3,3,0,
   0,1,0,3,0,3,2,3,1,3,1,0,0,0,0,1,1,2,0,0,0,0,
  3,3,3,3,3,3,3,2,3,3,3,3,3,3,3,2,3,3,3,2,3,3,
   3,3,2,1,3,3,2,0,0,2,3,3,2,0,2,0,1,2,0,0,1,0,
  3,3,3,3,3,3,3,2,3,3,3,3,3,1,3,3,2,3,3,2,3,3,
   3,3,3,3,3,2,0,1,0,2,0,1,0,2,2,0,0,1,0,1,1,0,
  3,3,3,3,2,3,3,3,3,3,3,3,3,3,3,3,3,3,0,3,3,1,
   0,0,0,3,2,3,2,3,2,1,3,0,0,0,2,1,0,3,1,0,0,0,
  0,0,0,3,0,3,3,3,3,3,3,3,3,2,1,3,2,1,0,3,3,0,
   0,0,0,2,0,3,0,2,0,1,0,0,0,0,0,0,0,2,0,0,0,0,
  2,2,0,3,2,3,3,3,3,3,3,3,3,3,0,3,3,3,0,3,3,0,
   0,0,0,3,0,3,1,3,1,2,3,0,0,0,0,0,0,2,0,0,0,0,
  2,1,2,3,1,3,2,3,2,3,3,3,3,1,1,3,3,1,0,3,1,0,
   0,0,0,3,0,2,3,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,
  3,3,3,3,3,2,3,3,0,3,3,2,0,0,3,2,0,0,1,0,2,0,
   0,3,1,0,2,3,0,0,0,2,0,1,0,0,0,0,0,2,0,0,0,1,
  2,1,0,3,0,3,3,3,3,3,3,3,3,3,0,3,3,0,0,3,3,0,
   0,0,0,3,0,3,1,3,1,1,3,0,0,0,0,0,0,0,0,0,0,0,
  3,3,3,3,3,2,3,3,3,3,3,2,0,3,3,1,2,0,2,0,0,0,
   0,3,1,1,0,3,0,0,0,2,3,0,0,0,0,0,0,1,0,0,1,0,
  3,3,3,3,3,3,3,2,2,3,3,3,2,1,3,2,3,2,3,1,2,3,
   0,1,3,0,2,0,2,0,1,0,0,3,0,1,2,0,0,0,1,0,1,0,
  3,3,3,3,3,1,3,0,0,3,2,1,3,0,3,2,1,1,3,0,3,0,
   0,3,2,0,1,3,0,0,0,0,0,0,0,0,0,0,3,2,0,0,0,0,
  3,3,3,2,3,3,3,3,0,2,3,2,1,1,3,2,1,1,3,1,1,3,
   1,3,3,0,1,1,2,0,3,0,0,2,0,0,0,1,0,0,0,0,0,0,
  3,3,0,3,0,2,3,2,3,3,0,3,0,1,3,1,1,1,0,0,3,0,
   0,0,0,1,3,2,1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,
  3,3,0,1,2,3,1,1,0,2,1,1,2,0,2,1,1,0,1,0,1,0,
   0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,0,3,1,3,1,3,0,1,2,3,3,3,0,1,1,2,0,3,3,0,
   0,0,0,0,0,0,3,1,2,0,1,0,0,0,0,0,0,0,0,0,0,0,
  0,1,0,2,1,2,3,3,0,1,1,1,2,0,0,1,0,0,0,3,0,0,
   0,0,0,3,0,0,1,0,0,0,3,0,1,0,0,0,0,0,0,0,0,0,
  0,0,0,1,0,3,3,2,3,2,3,1,3,0,0,2,0,2,0,3,3,0,
   0,0,0,1,0,0,0,3,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
  3,3,3,2,3,2,2,2,0,1,2,1,1,0,2,0,2,0,0,0,0,2,
   0,1,1,0,0,0,1,0,2,0,0,0,1,0,3,0,0,0,0,0,0,0,
  2,3,3,2,3,0,0,3,1,1,1,1,1,3,2,2,1,0,0,0,0,1,
   0,2,0,0,0,0,0,0,3,0,0,0,0,0,0,2,0,0,0,0,0,0,
  3,3,0,1,0,0,1,1,0,0,0,1,0,0,2,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  3,3,0,0,0,0,1,0,0,1,0,2,1,0,3,0,1,0,1,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,1,2,0,2,0,1,0,1,0,0,0,0,0,3,0,0,0,0,0,0,1,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,2,0,
   0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,2,0,0,0,0,
  0,0,0,1,0,2,2,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,1,0,0,0,2,0,0,0,1,0,0,0,0,0,0,0,0,1,0,
   0,0,0,0,0,2,0,1,0,0,0,0,0,0,0,0,0,3,0,0,0,0,
};


const SequenceModel Iso_8859_2SlovakModel =
{
  Iso_8859_2_CharToOrderMap,
  SlovakLangModel,
  44,
  (float)0.9990021826437321,
  PR_TRUE,
  "ISO-8859-2",
  "sk"
};

const SequenceModel Windows_1250SlovakModel =
{
  Windows_1250_CharToOrderMap,
  SlovakLangModel,
  44,
  (float)0.9990021826437321,
  PR_TRUE,
  "WINDOWS-1250",
  "sk"
};

const SequenceModel Ibm852SlovakModel =
{
  Ibm852_CharToOrderMap,
  SlovakLangModel,
  44,
  (float)0.9990021826437321,
  PR_TRUE,
  "IBM852",
  "sk"
};

const SequenceModel Mac_CentraleuropeSlovakModel =
{
  Mac_Centraleurope_CharToOrderMap,
  SlovakLangModel,
  44,
  (float)0.9990021826437321,
  PR_TRUE,
  "MAC-CENTRALEUROPE",
  "sk"
};

const LanguageModel SlovakModel =
{
  "sk",
  Unicode_CharOrder,
  88,
  SlovakLangModel,
  44,
  6,
  (float)0.4358970456502265,
  27,
  (float)0.03317555638659628,
};