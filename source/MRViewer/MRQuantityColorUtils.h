#pragma once

#include "MRMesh/MRColor.h"
#include "MRViewerFwd.h"

// OCCT
#include <Quantity_Color.hxx>
#include <cstdint>

namespace MR
{
inline bool isColorEqual(const Quantity_Color& a, const Color& b)
{
  // As the Quantity_Color has no alpha channel, we just compare the RGB channels
  return (uint8_t)(a.Red() * 255) == b.r && (uint8_t)(a.Green() * 255) == b.g
         && (uint8_t)(a.Blue() * 255) == b.b;
}

inline Quantity_Color toQuantityColor(const Color& color)
{
  return Quantity_Color(color.r / 255.0, color.g / 255.0, color.b / 255.0, Quantity_TOC_RGB);
}
} // namespace MR
