/**
 * This file is part of the "plotfx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include "graphics/layer.h"
#include "common/document.h"

namespace plotfx {

struct LegendConfig {
  enum kVerticalPosition {
    LEGEND_TOP = 0,
    LEGEND_BOTTOM = 1
  };

  enum kHorizontalPosition {
    LEGEND_LEFT = 0,
    LEGEND_RIGHT = 1
  };

  enum kPlacement {
    LEGEND_INSIDE = 0,
    LEGEND_OUTSIDE = 1
  };

  LegendConfig();

  void addEntry(
      const std::string& name,
      const Colour& color,
      const std::string& shape = "circle");

  Colour text_colour;
  Colour border_colour;
  FontInfo font;
  Measure padding_horiz;
  Measure padding_vert;
  Measure padding_item_horiz;
  Measure padding_item_vert;
  kVerticalPosition vert_pos;
  kHorizontalPosition horiz_pos;
  kPlacement placement;
  const std::string title;
  std::vector<std::tuple<std::string, Colour, std::string>> entries;
};

void renderOutsideLegends(Layer* target, const Rectangle& clip);

void renderInsideLegends(Layer* target, const Rectangle& clip);

void renderRightLegend(
    Layer* target,
    const Rectangle& clip,
    LegendConfig* legend,
    double horiz_padding,
    bool bottom,
    bool outside);

void renderLeftLegend(
    Layer* target,
    const Rectangle& clip,
    LegendConfig* legend,
    double horiz_padding,
    bool bottom,
    bool outside);

ReturnCode legend_configure(
    const Document& doc,
    const plist::PropertyList& plist,
    LegendConfig* config);

ReturnCode legend_draw(
    const LegendConfig& legend,
    const Rectangle& bbox,
    Layer* layer);

} // namespace plotfx

