// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/font/cpdf_cidfont.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include "build/build_config.h"
#include "core/fpdfapi/cmaps/fpdf_cmaps.h"
#include "core/fpdfapi/font/cfx_cttgsubtable.h"
#include "core/fpdfapi/font/cpdf_cid2unicodemap.h"
#include "core/fpdfapi/font/cpdf_cmap.h"
#include "core/fpdfapi/font/cpdf_cmapparser.h"
#include "core/fpdfapi/font/cpdf_fontencoding.h"
#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fxcrt/fixed_uninit_data_vector.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_unicode.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxge/fx_font.h"
#include "third_party/base/check.h"
#include "third_party/base/check_op.h"
#include "third_party/base/containers/span.h"

namespace {

constexpr FX_CodePage kCharsetCodePages[CIDSET_NUM_SETS] = {
    FX_CodePage::kDefANSI,
    FX_CodePage::kChineseSimplified,
    FX_CodePage::kChineseTraditional,
    FX_CodePage::kShiftJIS,
    FX_CodePage::kHangul,
    FX_CodePage::kUTF16LE};

struct CIDTransform {
  uint16_t cid;
  uint8_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t f;
};

constexpr CIDTransform kJapan1VerticalCIDs[] = {
    {97, 129, 0, 0, 127, 55, 0},     {7887, 127, 0, 0, 127, 76, 89},
    {7888, 127, 0, 0, 127, 79, 94},  {7889, 0, 129, 127, 0, 17, 127},
    {7890, 0, 129, 127, 0, 17, 127}, {7891, 0, 129, 127, 0, 17, 127},
    {7892, 0, 129, 127, 0, 17, 127}, {7893, 0, 129, 127, 0, 17, 127},
    {7894, 0, 129, 127, 0, 17, 127}, {7895, 0, 129, 127, 0, 17, 127},
    {7896, 0, 129, 127, 0, 17, 127}, {7897, 0, 129, 127, 0, 17, 127},
    {7898, 0, 129, 127, 0, 17, 127}, {7899, 0, 129, 127, 0, 17, 104},
    {7900, 0, 129, 127, 0, 17, 127}, {7901, 0, 129, 127, 0, 17, 104},
    {7902, 0, 129, 127, 0, 17, 127}, {7903, 0, 129, 127, 0, 17, 127},
    {7904, 0, 129, 127, 0, 17, 127}, {7905, 0, 129, 127, 0, 17, 114},
    {7906, 0, 129, 127, 0, 17, 127}, {7907, 0, 129, 127, 0, 17, 127},
    {7908, 0, 129, 127, 0, 17, 127}, {7909, 0, 129, 127, 0, 17, 127},
    {7910, 0, 129, 127, 0, 17, 127}, {7911, 0, 129, 127, 0, 17, 127},
    {7912, 0, 129, 127, 0, 17, 127}, {7913, 0, 129, 127, 0, 17, 127},
    {7914, 0, 129, 127, 0, 17, 127}, {7915, 0, 129, 127, 0, 17, 114},
    {7916, 0, 129, 127, 0, 17, 127}, {7917, 0, 129, 127, 0, 17, 127},
    {7918, 127, 0, 0, 127, 18, 25},  {7919, 127, 0, 0, 127, 18, 25},
    {7920, 127, 0, 0, 127, 18, 25},  {7921, 127, 0, 0, 127, 18, 25},
    {7922, 127, 0, 0, 127, 18, 25},  {7923, 127, 0, 0, 127, 18, 25},
    {7924, 127, 0, 0, 127, 18, 25},  {7925, 127, 0, 0, 127, 18, 25},
    {7926, 127, 0, 0, 127, 18, 25},  {7927, 127, 0, 0, 127, 18, 25},
    {7928, 127, 0, 0, 127, 18, 25},  {7929, 127, 0, 0, 127, 18, 25},
    {7930, 127, 0, 0, 127, 18, 25},  {7931, 127, 0, 0, 127, 18, 25},
    {7932, 127, 0, 0, 127, 18, 25},  {7933, 127, 0, 0, 127, 18, 25},
    {7934, 127, 0, 0, 127, 18, 25},  {7935, 127, 0, 0, 127, 18, 25},
    {7936, 127, 0, 0, 127, 18, 25},  {7937, 127, 0, 0, 127, 18, 25},
    {7938, 127, 0, 0, 127, 18, 25},  {7939, 127, 0, 0, 127, 18, 25},
    {8720, 0, 129, 127, 0, 19, 102}, {8721, 0, 129, 127, 0, 13, 127},
    {8722, 0, 129, 127, 0, 19, 108}, {8723, 0, 129, 127, 0, 19, 102},
    {8724, 0, 129, 127, 0, 19, 102}, {8725, 0, 129, 127, 0, 19, 102},
    {8726, 0, 129, 127, 0, 19, 102}, {8727, 0, 129, 127, 0, 19, 102},
    {8728, 0, 129, 127, 0, 19, 114}, {8729, 0, 129, 127, 0, 19, 114},
    {8730, 0, 129, 127, 0, 38, 108}, {8731, 0, 129, 127, 0, 13, 108},
    {8732, 0, 129, 127, 0, 19, 108}, {8733, 0, 129, 127, 0, 19, 108},
    {8734, 0, 129, 127, 0, 19, 108}, {8735, 0, 129, 127, 0, 19, 108},
    {8736, 0, 129, 127, 0, 19, 102}, {8737, 0, 129, 127, 0, 19, 102},
    {8738, 0, 129, 127, 0, 19, 102}, {8739, 0, 129, 127, 0, 19, 102},
    {8740, 0, 129, 127, 0, 19, 102}, {8741, 0, 129, 127, 0, 19, 102},
    {8742, 0, 129, 127, 0, 19, 102}, {8743, 0, 129, 127, 0, 19, 102},
    {8744, 0, 129, 127, 0, 19, 102}, {8745, 0, 129, 127, 0, 19, 102},
    {8746, 0, 129, 127, 0, 19, 114}, {8747, 0, 129, 127, 0, 19, 114},
    {8748, 0, 129, 127, 0, 19, 102}, {8749, 0, 129, 127, 0, 19, 102},
    {8750, 0, 129, 127, 0, 19, 102}, {8751, 0, 129, 127, 0, 19, 102},
    {8752, 0, 129, 127, 0, 19, 102}, {8753, 0, 129, 127, 0, 19, 102},
    {8754, 0, 129, 127, 0, 19, 102}, {8755, 0, 129, 127, 0, 19, 102},
    {8756, 0, 129, 127, 0, 19, 102}, {8757, 0, 129, 127, 0, 19, 102},
    {8758, 0, 129, 127, 0, 19, 102}, {8759, 0, 129, 127, 0, 19, 102},
    {8760, 0, 129, 127, 0, 19, 102}, {8761, 0, 129, 127, 0, 19, 102},
    {8762, 0, 129, 127, 0, 19, 102}, {8763, 0, 129, 127, 0, 19, 102},
    {8764, 0, 129, 127, 0, 19, 102}, {8765, 0, 129, 127, 0, 19, 102},
    {8766, 0, 129, 127, 0, 19, 102}, {8767, 0, 129, 127, 0, 19, 102},
    {8768, 0, 129, 127, 0, 19, 102}, {8769, 0, 129, 127, 0, 19, 102},
    {8770, 0, 129, 127, 0, 19, 102}, {8771, 0, 129, 127, 0, 19, 102},
    {8772, 0, 129, 127, 0, 19, 102}, {8773, 0, 129, 127, 0, 19, 102},
    {8774, 0, 129, 127, 0, 19, 102}, {8775, 0, 129, 127, 0, 19, 102},
    {8776, 0, 129, 127, 0, 19, 102}, {8777, 0, 129, 127, 0, 19, 102},
    {8778, 0, 129, 127, 0, 19, 102}, {8779, 0, 129, 127, 0, 19, 114},
    {8780, 0, 129, 127, 0, 19, 108}, {8781, 0, 129, 127, 0, 19, 114},
    {8782, 0, 129, 127, 0, 13, 114}, {8783, 0, 129, 127, 0, 19, 108},
    {8784, 0, 129, 127, 0, 13, 114}, {8785, 0, 129, 127, 0, 19, 108},
    {8786, 0, 129, 127, 0, 19, 108}, {8787, 0, 129, 127, 0, 19, 108},
    {8788, 0, 129, 127, 0, 19, 108}, {8789, 0, 129, 127, 0, 19, 108},
    {8790, 0, 129, 127, 0, 19, 108}, {8791, 0, 129, 127, 0, 19, 108},
    {8792, 0, 129, 127, 0, 19, 108}, {8793, 0, 129, 127, 0, 19, 108},
    {8794, 0, 129, 127, 0, 19, 108}, {8795, 0, 129, 127, 0, 19, 108},
    {8796, 0, 129, 127, 0, 19, 108}, {8797, 0, 129, 127, 0, 19, 108},
    {8798, 0, 129, 127, 0, 19, 108}, {8799, 0, 129, 127, 0, 19, 108},
    {8800, 0, 129, 127, 0, 19, 108}, {8801, 0, 129, 127, 0, 19, 108},
    {8802, 0, 129, 127, 0, 19, 108}, {8803, 0, 129, 127, 0, 19, 108},
    {8804, 0, 129, 127, 0, 19, 108}, {8805, 0, 129, 127, 0, 19, 108},
    {8806, 0, 129, 127, 0, 19, 108}, {8807, 0, 129, 127, 0, 19, 108},
    {8808, 0, 129, 127, 0, 19, 108}, {8809, 0, 129, 127, 0, 19, 108},
    {8810, 0, 129, 127, 0, 19, 108}, {8811, 0, 129, 127, 0, 19, 114},
    {8812, 0, 129, 127, 0, 19, 102}, {8813, 0, 129, 127, 0, 19, 114},
    {8814, 0, 129, 127, 0, 76, 102}, {8815, 0, 129, 127, 0, 13, 121},
    {8816, 0, 129, 127, 0, 19, 114}, {8817, 0, 129, 127, 0, 19, 127},
    {8818, 0, 129, 127, 0, 19, 114}, {8819, 0, 129, 127, 0, 218, 108},
};

// Boundary value to avoid integer overflow when adding 1/64th of the value.
constexpr int kMaxRectTop = 2114445437;

int FTPosToCBoxInt(FT_Pos pos) {
  // Boundary values to avoid integer overflow when multiplied by 1000.
  constexpr FT_Pos kMinCBox = -2147483;
  constexpr FT_Pos kMaxCBox = 2147483;
  return static_cast<int>(std::clamp(pos, kMinCBox, kMaxCBox));
}

#if !BUILDFLAG(IS_WIN)

bool IsValidEmbeddedCharcodeFromUnicodeCharset(CIDSet charset) {
  switch (charset) {
    case CIDSET_GB1:
    case CIDSET_CNS1:
    case CIDSET_JAPAN1:
    case CIDSET_KOREA1:
      return true;

    default:
      return false;
  }
}

wchar_t EmbeddedUnicodeFromCharcode(const fxcmap::CMap* pEmbedMap,
                                    CIDSet charset,
                                    uint32_t charcode) {
  if (!IsValidEmbeddedCharcodeFromUnicodeCharset(charset))
    return 0;

  uint16_t cid = fxcmap::CIDFromCharCode(pEmbedMap, charcode);
  if (!cid)
    return 0;

  pdfium::span<const uint16_t> map =
      CPDF_FontGlobals::GetInstance()->GetEmbeddedToUnicode(charset);
  return cid < map.size() ? map[cid] : 0;
}

uint32_t EmbeddedCharcodeFromUnicode(const fxcmap::CMap* pEmbedMap,
                                     CIDSet charset,
                                     wchar_t unicode) {
  if (!IsValidEmbeddedCharcodeFromUnicodeCharset(charset))
    return 0;

  pdfium::span<const uint16_t> map =
      CPDF_FontGlobals::GetInstance()->GetEmbeddedToUnicode(charset);
  for (uint32_t i = 0; i < map.size(); ++i) {
    if (map[i] == unicode) {
      uint32_t charCode = fxcmap::CharCodeFromCID(pEmbedMap, i);
      if (charCode)
        return charCode;
    }
  }
  return 0;
}

#endif  // !BUILDFLAG(IS_WIN)

void FT_UseCIDCharmap(FXFT_FaceRec* face, CIDCoding coding) {
  int encoding;
  switch (coding) {
    case CIDCoding::kGB:
      encoding = FT_ENCODING_GB2312;
      break;
    case CIDCoding::kBIG5:
      encoding = FT_ENCODING_BIG5;
      break;
    case CIDCoding::kJIS:
      encoding = FT_ENCODING_SJIS;
      break;
    case CIDCoding::kKOREA:
      encoding = FT_ENCODING_JOHAB;
      break;
    default:
      encoding = FT_ENCODING_UNICODE;
  }
  int err = FXFT_Select_Charmap(face, encoding);
  if (err)
    err = FXFT_Select_Charmap(face, FT_ENCODING_UNICODE);
  if (err && face->charmaps)
    FT_Set_Charmap(face, face->charmaps[0]);
}

bool IsMetricForCID(const int* pEntry, uint16_t cid) {
  return pEntry[0] <= cid && pEntry[1] >= cid;
}

void LoadMetricsArray(RetainPtr<const CPDF_Array> pArray,
                      std::vector<int>* result,
                      int nElements) {
  int width_status = 0;
  int iCurElement = 0;
  int first_code = 0;
  int last_code = 0;
  for (size_t i = 0; i < pArray->size(); i++) {
    RetainPtr<const CPDF_Object> pObj = pArray->GetDirectObjectAt(i);
    if (!pObj)
      continue;

    const CPDF_Array* pObjArray = pObj->AsArray();
    if (pObjArray) {
      if (width_status != 1)
        return;
      if (first_code > std::numeric_limits<int>::max() -
                           fxcrt::CollectionSize<int>(*pObjArray)) {
        width_status = 0;
        continue;
      }

      for (size_t j = 0; j < pObjArray->size(); j += nElements) {
        result->push_back(first_code);
        result->push_back(first_code);
        for (int k = 0; k < nElements; k++)
          result->push_back(pObjArray->GetIntegerAt(j + k));
        first_code++;
      }
      width_status = 0;
    } else {
      if (width_status == 0) {
        first_code = pObj->GetInteger();
        width_status = 1;
      } else if (width_status == 1) {
        last_code = pObj->GetInteger();
        width_status = 2;
        iCurElement = 0;
      } else {
        if (!iCurElement) {
          result->push_back(first_code);
          result->push_back(last_code);
        }
        result->push_back(pObj->GetInteger());
        iCurElement++;
        if (iCurElement == nElements)
          width_status = 0;
      }
    }
  }
}

}  // namespace

CPDF_CIDFont::CPDF_CIDFont(CPDF_Document* pDocument,
                           RetainPtr<CPDF_Dictionary> pFontDict)
    : CPDF_Font(pDocument, std::move(pFontDict)) {
  for (size_t i = 0; i < std::size(m_CharBBox); ++i)
    m_CharBBox[i] = FX_RECT(-1, -1, -1, -1);
}

CPDF_CIDFont::~CPDF_CIDFont() = default;

bool CPDF_CIDFont::IsCIDFont() const {
  return true;
}

const CPDF_CIDFont* CPDF_CIDFont::AsCIDFont() const {
  return this;
}

CPDF_CIDFont* CPDF_CIDFont::AsCIDFont() {
  return this;
}

uint16_t CPDF_CIDFont::CIDFromCharCode(uint32_t charcode) const {
  return m_pCMap ? m_pCMap->CIDFromCharCode(charcode)
                 : static_cast<uint16_t>(charcode);
}

bool CPDF_CIDFont::IsVertWriting() const {
  return m_pCMap && m_pCMap->IsVertWriting();
}

WideString CPDF_CIDFont::UnicodeFromCharCode(uint32_t charcode) const {
  WideString str = CPDF_Font::UnicodeFromCharCode(charcode);
  if (!str.IsEmpty())
    return str;
  wchar_t ret = GetUnicodeFromCharCode(charcode);
  return ret ? WideString(ret) : WideString();
}

wchar_t CPDF_CIDFont::GetUnicodeFromCharCode(uint32_t charcode) const {
  switch (m_pCMap->GetCoding()) {
    case CIDCoding::kUCS2:
    case CIDCoding::kUTF16:
      return static_cast<wchar_t>(charcode);
    case CIDCoding::kCID:
      if (!m_pCID2UnicodeMap || !m_pCID2UnicodeMap->IsLoaded())
        return 0;
      return m_pCID2UnicodeMap->UnicodeFromCID(static_cast<uint16_t>(charcode));
    default:
      break;
  }
  if (m_pCID2UnicodeMap && m_pCID2UnicodeMap->IsLoaded() && m_pCMap->IsLoaded())
    return m_pCID2UnicodeMap->UnicodeFromCID(CIDFromCharCode(charcode));

#if BUILDFLAG(IS_WIN)
  wchar_t unicode;
  int charsize = 1;
  if (charcode > 255) {
    charcode = (charcode % 256) * 256 + (charcode / 256);
    charsize = 2;
  }
  size_t ret = FX_MultiByteToWideChar(
      kCharsetCodePages[static_cast<size_t>(m_pCMap->GetCoding())],
      ByteStringView(reinterpret_cast<const char*>(&charcode), charsize),
      pdfium::make_span(&unicode, 1));
  return ret == 1 ? unicode : 0;
#else
  if (!m_pCMap->GetEmbedMap())
    return 0;
  return EmbeddedUnicodeFromCharcode(m_pCMap->GetEmbedMap(),
                                     m_pCMap->GetCharset(), charcode);
#endif
}

uint32_t CPDF_CIDFont::CharCodeFromUnicode(wchar_t unicode) const {
  uint32_t charcode = CPDF_Font::CharCodeFromUnicode(unicode);
  if (charcode)
    return charcode;

  switch (m_pCMap->GetCoding()) {
    case CIDCoding::kUNKNOWN:
      return 0;
    case CIDCoding::kUCS2:
    case CIDCoding::kUTF16:
      return unicode;
    case CIDCoding::kCID: {
      if (!m_pCID2UnicodeMap || !m_pCID2UnicodeMap->IsLoaded())
        return 0;
      uint32_t cid = 0;
      while (cid < 65536) {
        wchar_t this_unicode =
            m_pCID2UnicodeMap->UnicodeFromCID(static_cast<uint16_t>(cid));
        if (this_unicode == unicode)
          return cid;
        cid++;
      }
      break;
    }
    default:
      break;
  }

  if (unicode < 0x80)
    return static_cast<uint32_t>(unicode);
  if (m_pCMap->GetCoding() == CIDCoding::kCID)
    return 0;
#if BUILDFLAG(IS_WIN)
  uint8_t buffer[32];
  size_t ret = FX_WideCharToMultiByte(
      kCharsetCodePages[static_cast<size_t>(m_pCMap->GetCoding())],
      WideStringView(&unicode, 1),
      pdfium::make_span(reinterpret_cast<char*>(buffer), 4));
  if (ret == 1)
    return buffer[0];
  if (ret == 2)
    return buffer[0] * 256 + buffer[1];
#else
  if (m_pCMap->GetEmbedMap()) {
    return EmbeddedCharcodeFromUnicode(m_pCMap->GetEmbedMap(),
                                       m_pCMap->GetCharset(), unicode);
  }
#endif
  return 0;
}

bool CPDF_CIDFont::Load() {
  if (m_pFontDict->GetByteStringFor("Subtype") == "TrueType") {
    LoadGB2312();
    return true;
  }

  RetainPtr<const CPDF_Array> pFonts =
      m_pFontDict->GetArrayFor("DescendantFonts");
  if (!pFonts || pFonts->size() != 1)
    return false;

  RetainPtr<const CPDF_Dictionary> pCIDFontDict = pFonts->GetDictAt(0);
  if (!pCIDFontDict)
    return false;

  m_BaseFontName = pCIDFontDict->GetByteStringFor("BaseFont");
  if ((m_BaseFontName == "CourierStd" || m_BaseFontName == "CourierStd-Bold" ||
       m_BaseFontName == "CourierStd-BoldOblique" ||
       m_BaseFontName == "CourierStd-Oblique") &&
      !IsEmbedded()) {
    m_bAdobeCourierStd = true;
  }

  RetainPtr<const CPDF_Object> pEncoding =
      m_pFontDict->GetDirectObjectFor("Encoding");
  if (!pEncoding)
    return false;

  ByteString subtype = pCIDFontDict->GetByteStringFor("Subtype");
  m_FontType =
      subtype == "CIDFontType0" ? CIDFontType::kType1 : CIDFontType::kTrueType;

  if (!pEncoding->IsName() && !pEncoding->IsStream())
    return false;

  auto* pFontGlobals = CPDF_FontGlobals::GetInstance();
  const CPDF_Stream* pEncodingStream = pEncoding->AsStream();
  if (pEncodingStream) {
    auto pAcc =
        pdfium::MakeRetain<CPDF_StreamAcc>(pdfium::WrapRetain(pEncodingStream));
    pAcc->LoadAllDataFiltered();
    pdfium::span<const uint8_t> span = pAcc->GetSpan();
    m_pCMap = pdfium::MakeRetain<CPDF_CMap>(span);
  } else {
    DCHECK(pEncoding->IsName());
    ByteString cmap = pEncoding->GetString();
    m_pCMap = pFontGlobals->GetPredefinedCMap(cmap);
  }

  RetainPtr<const CPDF_Dictionary> pFontDesc =
      pCIDFontDict->GetDictFor("FontDescriptor");
  if (pFontDesc)
    LoadFontDescriptor(pFontDesc.Get());

  m_Charset = m_pCMap->GetCharset();
  if (m_Charset == CIDSET_UNKNOWN) {
    RetainPtr<const CPDF_Dictionary> pCIDInfo =
        pCIDFontDict->GetDictFor("CIDSystemInfo");
    if (pCIDInfo) {
      m_Charset = CPDF_CMapParser::CharsetFromOrdering(
          pCIDInfo->GetByteStringFor("Ordering").AsStringView());
    }
  }
  if (m_Charset != CIDSET_UNKNOWN) {
    m_pCID2UnicodeMap = pFontGlobals->GetCID2UnicodeMap(m_Charset);
  }
  if (m_Font.GetFaceRec()) {
    if (m_FontType == CIDFontType::kType1)
      FXFT_Select_Charmap(m_Font.GetFaceRec(), FT_ENCODING_UNICODE);
    else
      FT_UseCIDCharmap(m_Font.GetFaceRec(), m_pCMap->GetCoding());
  }
  m_DefaultWidth = pCIDFontDict->GetIntegerFor("DW", 1000);
  RetainPtr<const CPDF_Array> pWidthArray = pCIDFontDict->GetArrayFor("W");
  if (pWidthArray)
    LoadMetricsArray(std::move(pWidthArray), &m_WidthList, 1);

  if (!IsEmbedded())
    LoadSubstFont();

  RetainPtr<const CPDF_Object> pmap =
      pCIDFontDict->GetDirectObjectFor("CIDToGIDMap");
  if (pmap) {
    RetainPtr<const CPDF_Stream> pMapStream(pmap->AsStream());
    if (pMapStream) {
      m_pStreamAcc = pdfium::MakeRetain<CPDF_StreamAcc>(std::move(pMapStream));
      m_pStreamAcc->LoadAllDataFiltered();
    } else if (m_pFontFile && pmap->IsName() &&
               pmap->GetString() == "Identity") {
      m_bCIDIsGID = true;
    }
  }

  CheckFontMetrics();
  if (IsVertWriting()) {
    RetainPtr<const CPDF_Array> pWidth2Array = pCIDFontDict->GetArrayFor("W2");
    if (pWidth2Array)
      LoadMetricsArray(std::move(pWidth2Array), &m_VertMetrics, 3);

    RetainPtr<const CPDF_Array> pDefaultArray =
        pCIDFontDict->GetArrayFor("DW2");
    if (pDefaultArray) {
      m_DefaultVY = pDefaultArray->GetIntegerAt(0);
      m_DefaultW1 = pDefaultArray->GetIntegerAt(1);
    }
  }

  // TODO(thestig): Better identify font types and identify more font types.
  if (m_FontType == CIDFontType::kTrueType && IsEmbedded())
    m_Font.SetFontType(CFX_Font::FontType::kCIDTrueType);

  return true;
}

FX_RECT CPDF_CIDFont::GetCharBBox(uint32_t charcode) {
  if (charcode < 256 && m_CharBBox[charcode].right != -1)
    return m_CharBBox[charcode];

  FX_RECT rect;
  bool bVert = false;
  int glyph_index = GlyphFromCharCode(charcode, &bVert);
  FXFT_FaceRec* face = m_Font.GetFaceRec();
  if (face) {
    if (FXFT_Is_Face_Tricky(face)) {
      int err =
          FT_Load_Glyph(face, glyph_index, FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
      if (!err) {
        FT_Glyph glyph;
        err = FT_Get_Glyph(face->glyph, &glyph);
        if (!err) {
          FT_BBox cbox;
          FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &cbox);
          const int xMin = FTPosToCBoxInt(cbox.xMin);
          const int xMax = FTPosToCBoxInt(cbox.xMax);
          const int yMin = FTPosToCBoxInt(cbox.yMin);
          const int yMax = FTPosToCBoxInt(cbox.yMax);
          const int pixel_size_x = face->size->metrics.x_ppem;
          const int pixel_size_y = face->size->metrics.y_ppem;
          if (pixel_size_x == 0 || pixel_size_y == 0) {
            rect = FX_RECT(xMin, yMax, xMax, yMin);
          } else {
            rect =
                FX_RECT(xMin * 1000 / pixel_size_x, yMax * 1000 / pixel_size_y,
                        xMax * 1000 / pixel_size_x, yMin * 1000 / pixel_size_y);
          }
          rect.top = std::min(rect.top,
                              static_cast<int>(FXFT_Get_Face_Ascender(face)));
          rect.bottom = std::max(
              rect.bottom, static_cast<int>(FXFT_Get_Face_Descender(face)));
          FT_Done_Glyph(glyph);
        }
      }
    } else {
      int err = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE);
      if (err == 0) {
        rect = FX_RECT(TT2PDF(FXFT_Get_Glyph_HoriBearingX(face), face),
                       TT2PDF(FXFT_Get_Glyph_HoriBearingY(face), face),
                       TT2PDF(FXFT_Get_Glyph_HoriBearingX(face) +
                                  FXFT_Get_Glyph_Width(face),
                              face),
                       TT2PDF(FXFT_Get_Glyph_HoriBearingY(face) -
                                  FXFT_Get_Glyph_Height(face),
                              face));
        if (rect.top <= kMaxRectTop)
          rect.top += rect.top / 64;
        else
          rect.top = std::numeric_limits<int>::max();
      }
    }
  }
  if (!m_pFontFile && m_Charset == CIDSET_JAPAN1) {
    uint16_t cid = CIDFromCharCode(charcode);
    const uint8_t* pTransform = GetCIDTransform(cid);
    if (pTransform && !bVert) {
      CFX_Matrix matrix(CIDTransformToFloat(pTransform[0]),
                        CIDTransformToFloat(pTransform[1]),
                        CIDTransformToFloat(pTransform[2]),
                        CIDTransformToFloat(pTransform[3]),
                        CIDTransformToFloat(pTransform[4]) * 1000,
                        CIDTransformToFloat(pTransform[5]) * 1000);
      rect = matrix.TransformRect(CFX_FloatRect(rect)).GetOuterRect();
    }
  }
  if (charcode < 256)
    m_CharBBox[charcode] = rect;

  return rect;
}

int CPDF_CIDFont::GetCharWidthF(uint32_t charcode) {
  if (charcode < 0x80 && m_bAnsiWidthsFixed)
    return (charcode >= 32 && charcode < 127) ? 500 : 0;

  uint16_t cid = CIDFromCharCode(charcode);
  size_t size = m_WidthList.size();
  const int* pList = m_WidthList.data();
  for (size_t i = 0; i < size; i += 3) {
    const int* pEntry = pList + i;
    if (IsMetricForCID(pEntry, cid))
      return pEntry[2];
  }
  return m_DefaultWidth;
}

int16_t CPDF_CIDFont::GetVertWidth(uint16_t cid) const {
  size_t vertsize = m_VertMetrics.size() / 5;
  if (vertsize) {
    const int* pTable = m_VertMetrics.data();
    for (size_t i = 0; i < vertsize; i++) {
      const int* pEntry = pTable + (i * 5);
      if (IsMetricForCID(pEntry, cid))
        return static_cast<int16_t>(pEntry[2]);
    }
  }
  return m_DefaultW1;
}

CFX_Point16 CPDF_CIDFont::GetVertOrigin(uint16_t cid) const {
  size_t vertsize = m_VertMetrics.size() / 5;
  if (vertsize) {
    const int* pTable = m_VertMetrics.data();
    for (size_t i = 0; i < vertsize; i++) {
      const int* pEntry = pTable + (i * 5);
      if (IsMetricForCID(pEntry, cid)) {
        return {static_cast<int16_t>(pEntry[3]),
                static_cast<int16_t>(pEntry[4])};
      }
    }
  }
  int width = m_DefaultWidth;
  size_t size = m_WidthList.size();
  const int* pList = m_WidthList.data();
  for (size_t i = 0; i < size; i += 3) {
    const int* pEntry = pList + i;
    if (IsMetricForCID(pEntry, cid)) {
      width = pEntry[2];
      break;
    }
  }
  return {static_cast<int16_t>(width / 2), m_DefaultVY};
}

int CPDF_CIDFont::GetGlyphIndex(uint32_t unicode, bool* pVertGlyph) {
  if (pVertGlyph)
    *pVertGlyph = false;

  FXFT_FaceRec* face = m_Font.GetFaceRec();
  int index = FT_Get_Char_Index(face, unicode);
  if (unicode == pdfium::unicode::kBoxDrawingsLightVerical)
    return index;

  if (!index || !IsVertWriting())
    return index;

  if (m_pTTGSUBTable)
    return GetVerticalGlyph(index, pVertGlyph);

  static constexpr uint32_t kGsubTag =
      CFX_FontMapper::MakeTag('G', 'S', 'U', 'B');
  unsigned long length = 0;
  int error = FT_Load_Sfnt_Table(face, kGsubTag, 0, nullptr, &length);
  if (error || !length) {
    return index;
  }

  FixedUninitDataVector<uint8_t> sub_data(length);
  error = FT_Load_Sfnt_Table(face, kGsubTag, 0, sub_data.writable_span().data(),
                             nullptr);
  if (error) {
    return index;
  }

  // CFX_CTTGSUBTable parses the data and stores all the values in its structs.
  // It does not store pointers into `sub_data`.
  m_pTTGSUBTable = std::make_unique<CFX_CTTGSUBTable>(sub_data.span());
  return GetVerticalGlyph(index, pVertGlyph);
}

int CPDF_CIDFont::GetVerticalGlyph(int index, bool* pVertGlyph) {
  uint32_t vindex = m_pTTGSUBTable->GetVerticalGlyph(index);
  if (!vindex)
    return index;

  index = vindex;
  if (pVertGlyph)
    *pVertGlyph = true;
  return index;
}

int CPDF_CIDFont::GlyphFromCharCode(uint32_t charcode, bool* pVertGlyph) {
  if (pVertGlyph)
    *pVertGlyph = false;

  if (!m_pFontFile && (!m_pStreamAcc || m_pCID2UnicodeMap)) {
    uint16_t cid = CIDFromCharCode(charcode);
    wchar_t unicode = 0;
    if (m_bCIDIsGID) {
#if BUILDFLAG(IS_APPLE)
      if (FontStyleIsSymbolic(m_Flags))
        return cid;

      WideString uni_str = UnicodeFromCharCode(charcode);
      if (uni_str.IsEmpty())
        return cid;

      unicode = uni_str[0];
#else
      return cid;
#endif
    } else {
      if (cid && m_pCID2UnicodeMap && m_pCID2UnicodeMap->IsLoaded())
        unicode = m_pCID2UnicodeMap->UnicodeFromCID(cid);
      if (unicode == 0)
        unicode = GetUnicodeFromCharCode(charcode);
      if (unicode == 0) {
        WideString unicode_str = UnicodeFromCharCode(charcode);
        if (!unicode_str.IsEmpty())
          unicode = unicode_str[0];
      }
    }
    FXFT_FaceRec* face = m_Font.GetFaceRec();
    if (unicode == 0) {
      if (!m_bAdobeCourierStd)
        return charcode ? static_cast<int>(charcode) : -1;

      charcode += 31;
      bool bMSUnicode = UseTTCharmapMSUnicode(face);
      bool bMacRoman = !bMSUnicode && UseTTCharmapMacRoman(face);
      FontEncoding base_encoding = FontEncoding::kStandard;
      if (bMSUnicode)
        base_encoding = FontEncoding::kWinAnsi;
      else if (bMacRoman)
        base_encoding = FontEncoding::kMacRoman;
      const char* name =
          GetAdobeCharName(base_encoding, std::vector<ByteString>(), charcode);
      if (!name)
        return charcode ? static_cast<int>(charcode) : -1;

      int index = 0;
      uint16_t name_unicode = UnicodeFromAdobeName(name);
      if (!name_unicode)
        return charcode ? static_cast<int>(charcode) : -1;

      if (base_encoding == FontEncoding::kStandard)
        return FT_Get_Char_Index(face, name_unicode);

      if (base_encoding == FontEncoding::kWinAnsi) {
        index = FT_Get_Char_Index(face, name_unicode);
      } else {
        DCHECK_EQ(base_encoding, FontEncoding::kMacRoman);
        uint32_t maccode = CharCodeFromUnicodeForFreetypeEncoding(
            FT_ENCODING_APPLE_ROMAN, name_unicode);
        index = maccode ? FT_Get_Char_Index(face, maccode)
                        : FT_Get_Name_Index(face, (FT_String*)name);
      }
      if (index == 0 || index == 0xffff)
        return charcode ? static_cast<int>(charcode) : -1;
      return index;
    }
    if (m_Charset == CIDSET_JAPAN1) {
      if (unicode == '\\') {
        unicode = '/';
#if !BUILDFLAG(IS_APPLE)
      } else if (unicode == 0xa5) {
        unicode = 0x5c;
#endif
      }
    }
    if (!face)
      return unicode;

    int err = FXFT_Select_Charmap(face, FT_ENCODING_UNICODE);
    if (err) {
      int i;
      for (i = 0; i < face->num_charmaps; i++) {
        uint32_t ret = CharCodeFromUnicodeForFreetypeEncoding(
            FXFT_Get_Charmap_Encoding(face->charmaps[i]),
            static_cast<wchar_t>(charcode));
        if (ret == 0)
          continue;
        FT_Set_Charmap(face, face->charmaps[i]);
        unicode = static_cast<wchar_t>(ret);
        break;
      }
      if (i == face->num_charmaps && i) {
        FT_Set_Charmap(face, face->charmaps[0]);
        unicode = static_cast<wchar_t>(charcode);
      }
    }
    if (face->charmap) {
      int index = GetGlyphIndex(unicode, pVertGlyph);
      return index != 0 ? index : -1;
    }
    return unicode;
  }

  if (!m_Font.GetFaceRec())
    return -1;

  uint16_t cid = CIDFromCharCode(charcode);
  if (!m_pStreamAcc) {
    if (m_FontType == CIDFontType::kType1)
      return cid;
    if (m_pFontFile && m_pCMap->IsDirectCharcodeToCIDTableIsEmpty())
      return cid;

    FT_CharMap charmap = m_Font.GetFaceRec()->charmap;
    if (!charmap || m_pCMap->GetCoding() == CIDCoding::kUNKNOWN)
      return cid;

    if (FXFT_Get_Charmap_Encoding(charmap) == FT_ENCODING_UNICODE) {
      WideString unicode_str = UnicodeFromCharCode(charcode);
      if (unicode_str.IsEmpty())
        return -1;

      charcode = unicode_str[0];
    }
    return GetGlyphIndex(charcode, pVertGlyph);
  }
  uint32_t byte_pos = cid * 2;
  if (byte_pos + 2 > m_pStreamAcc->GetSize())
    return -1;

  pdfium::span<const uint8_t> span = m_pStreamAcc->GetSpan().subspan(byte_pos);
  return span[0] * 256 + span[1];
}

uint32_t CPDF_CIDFont::GetNextChar(ByteStringView pString,
                                   size_t* pOffset) const {
  return m_pCMap->GetNextChar(pString, pOffset);
}

int CPDF_CIDFont::GetCharSize(uint32_t charcode) const {
  return m_pCMap->GetCharSize(charcode);
}

size_t CPDF_CIDFont::CountChar(ByteStringView pString) const {
  return m_pCMap->CountChar(pString);
}

int CPDF_CIDFont::AppendChar(char* str, uint32_t charcode) const {
  return m_pCMap->AppendChar(str, charcode);
}

bool CPDF_CIDFont::IsUnicodeCompatible() const {
  if (m_pCID2UnicodeMap && m_pCID2UnicodeMap->IsLoaded() && m_pCMap->IsLoaded())
    return true;
  return m_pCMap->GetCoding() != CIDCoding::kUNKNOWN;
}

void CPDF_CIDFont::LoadSubstFont() {
  FX_SAFE_INT32 safeStemV(m_StemV);
  safeStemV *= 5;
  m_Font.LoadSubst(m_BaseFontName, m_FontType == CIDFontType::kTrueType,
                   m_Flags, safeStemV.ValueOrDefault(FXFONT_FW_NORMAL),
                   m_ItalicAngle, kCharsetCodePages[m_Charset],
                   IsVertWriting());
}

// static
float CPDF_CIDFont::CIDTransformToFloat(uint8_t ch) {
  return (ch < 128 ? ch : ch - 255) * (1.0f / 127);
}

void CPDF_CIDFont::LoadGB2312() {
  m_BaseFontName = m_pFontDict->GetByteStringFor("BaseFont");
  m_Charset = CIDSET_GB1;

  auto* pFontGlobals = CPDF_FontGlobals::GetInstance();
  m_pCMap = pFontGlobals->GetPredefinedCMap("GBK-EUC-H");
  m_pCID2UnicodeMap = pFontGlobals->GetCID2UnicodeMap(m_Charset);
  RetainPtr<const CPDF_Dictionary> pFontDesc =
      m_pFontDict->GetDictFor("FontDescriptor");
  if (pFontDesc)
    LoadFontDescriptor(pFontDesc.Get());

  if (!IsEmbedded())
    LoadSubstFont();
  CheckFontMetrics();
  m_bAnsiWidthsFixed = true;
}

const uint8_t* CPDF_CIDFont::GetCIDTransform(uint16_t cid) const {
  if (m_Charset != CIDSET_JAPAN1 || m_pFontFile)
    return nullptr;

  const auto* pBegin = std::begin(kJapan1VerticalCIDs);
  const auto* pEnd = std::end(kJapan1VerticalCIDs);
  const auto* pTransform = std::lower_bound(
      pBegin, pEnd, cid,
      [](const CIDTransform& entry, uint16_t cid) { return entry.cid < cid; });

  return (pTransform < pEnd && cid == pTransform->cid) ? &pTransform->a
                                                       : nullptr;
}
