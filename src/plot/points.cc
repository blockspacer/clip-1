/**
 * This file is part of the "clip" project
 *   Copyright (c) 2018 Paul Asmuth
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "points.h"
#include "data.h"
#include "sexpr.h"
#include "sexpr_conv.h"
#include "sexpr_util.h"
#include "context.h"
#include "color_reader.h"
#include "typographic_map.h"
#include "typographic_reader.h"
#include "layout.h"
#include "marker.h"
#include "scale.h"
#include "graphics/path.h"
#include "graphics/brush.h"
#include "graphics/text.h"
#include "graphics/layout.h"

#include <numeric>

using namespace std::placeholders;
using std::bind;

namespace clip::plotgen {

static const double kDefaultPointSizePT = 4;
static const double kDefaultLabelPaddingEM = 0.2;

struct PlotPointsConfig {
  std::vector<Measure> x;
  std::vector<Measure> y;
  ScaleConfig scale_x;
  ScaleConfig scale_y;
  Color color;
  std::vector<Color> colors;
  Measure size;
  std::vector<Measure> sizes;
  Marker shape;
  std::vector<Marker> shapes;
  std::vector<std::string> labels;
  FontInfo label_font;
  Measure label_padding;
  Measure label_font_size;
  Color label_color;
};

ReturnCode points_draw(
    Context* ctx,
    PlotConfig* plot,
    PlotPointsConfig* config) {
  const auto& clip = plot_get_clip(plot, layer_get(ctx));

  /* convert units */
  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1),
        std::bind(&convert_unit_user, scale_translate_fn(config->scale_x), _1),
        std::bind(&convert_unit_relative, clip.w, _1)
      },
      &*config->x.begin(),
      &*config->x.end());

  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1),
        std::bind(&convert_unit_user, scale_translate_fn(config->scale_y), _1),
        std::bind(&convert_unit_relative, clip.h, _1)
      },
      &*config->y.begin(),
      &*config->y.end());

  convert_units(
      {
        std::bind(&convert_unit_typographic, layer_get_dpi(ctx), layer_get_font_size(ctx), _1)
      },
      &*config->sizes.begin(),
      &*config->sizes.end());

  convert_unit_typographic(
      layer_get_dpi(ctx),
      layer_get_font_size(ctx),
      &config->size);

  /* draw markers */
  for (size_t i = 0; i < config->x.size(); ++i) {
    auto sx = clip.x + config->x[i];
    auto sy = clip.y + config->y[i];

    const auto& color = config->colors.empty()
        ? config->color
        : config->colors[i % config->colors.size()];

    auto size = config->sizes.empty()
        ? config->size
        : config->sizes[i % config->sizes.size()];

    auto shape = config->shapes.empty()
        ? config->shape
        : config->shapes[i % config->shapes.size()];

    if (auto rc = shape(ctx, Point(sx, sy), size, color); !rc) {
      return rc;
    }
  }

  /* draw labels */
  for (size_t i = 0; i < config->labels.size(); ++i) {
    const auto& label_text = config->labels[i];

    auto size = config->sizes.empty()
        ? 0
        : config->sizes[i % config->sizes.size()].value;

    auto label_padding = size * 0.5 + measure_or(
        config->label_padding,
        from_em(kDefaultLabelPaddingEM, config->label_font_size));

    Point p(
        clip.x + config->x[i],
        clip.y + config->y[i] + label_padding);

    TextStyle style;
    style.font = config->label_font;
    style.color = config->label_color;
    style.font_size = config->label_font_size;

    auto ax = HAlign::CENTER;
    auto ay = VAlign::BOTTOM;
    if (auto rc = draw_text(ctx, label_text, p, ax, ay, style); rc != OK) {
      return rc;
    }
  }

  return OK;
}

ReturnCode points_configure(
    Context* ctx,
    PlotConfig* plot,
    PlotPointsConfig* c,
    const Expr* expr) {
  /* set defaults from environment */
  c->scale_x = plot->scale_x;
  c->scale_y = plot->scale_y;
  c->color = layer_get(ctx)->foreground_color;
  c->size = from_pt(kDefaultPointSizePT);
  c->shape = marker_create_disk();
  c->label_font = layer_get_font(ctx);
  c->label_font_size = layer_get_font_size(ctx);

  /* parse properties */
  std::vector<std::string> data_x;
  std::vector<std::string> data_y;
  std::vector<std::string> data_colors;
  std::vector<std::string> data_sizes;
  ColorMap color_map;
  MeasureMap size_map;

  auto config_rc = expr_walk_map_wrapped(expr, {
    {"data-x", std::bind(&data_load_strings, _1, &data_x)},
    {"data-y", std::bind(&data_load_strings, _1, &data_y)},
    {"limit-x", std::bind(&expr_to_float64_opt_pair, _1, &c->scale_x.min, &c->scale_x.max)},
    {"limit-x-min", std::bind(&expr_to_float64_opt, _1, &c->scale_x.min)},
    {"limit-x-max", std::bind(&expr_to_float64_opt, _1, &c->scale_x.max)},
    {"limit-y", std::bind(&expr_to_float64_opt_pair, _1, &c->scale_y.min, &c->scale_y.max)},
    {"limit-y-min", std::bind(&expr_to_float64_opt, _1, &c->scale_y.min)},
    {"limit-y-max", std::bind(&expr_to_float64_opt, _1, &c->scale_y.max)},
    {"scale-x", std::bind(&scale_configure_kind, _1, &c->scale_x)},
    {"scale-y", std::bind(&scale_configure_kind, _1, &c->scale_y)},
    {"scale-x-padding", std::bind(&expr_to_float64, _1, &c->scale_x.padding)},
    {"scale-y-padding", std::bind(&expr_to_float64, _1, &c->scale_y.padding)},
    {"shape", std::bind(&marker_configure, _1, &c->shape)},
    {"shapes", std::bind(&marker_configure_list, _1, &c->shapes)},
    {"size", std::bind(&measure_read, _1, &c->size)},
    {"sizes", std::bind(&data_load_strings, _1, &data_sizes)},
    {"size-map", std::bind(&measure_map_read, ctx, _1, &size_map)},
    {"color", std::bind(&color_read, ctx, _1, &c->color)},
    {"colors", std::bind(&data_load_strings, _1, &data_colors)},
    {"color-map", std::bind(&color_map_read, ctx, _1, &color_map)},
    {"labels", std::bind(&data_load_strings, _1, &c->labels)},
    {"label-font", expr_call_string_fn(std::bind(&font_load_best, _1, &c->label_font))},
    {"label-font-size", std::bind(&measure_read, _1, &c->label_font_size)},
    {"label-color", std::bind(&color_read, ctx, _1, &c->label_color)},
    {"label-padding", std::bind(&measure_read, _1, &c->label_padding)},
    {"font", expr_call_string_fn(std::bind(&font_load_best, _1, &c->label_font))},
  });

  if (!config_rc) {
    return config_rc;
  }

  /* scale configuration */
  if (auto rc = data_to_measures(data_x, c->scale_x, &c->x); !rc){
    return rc;
  }

  if (auto rc = data_to_measures(data_y, c->scale_y, &c->y); !rc){
    return rc;
  }

  for (const auto& v : c->x) {
    if (v.unit == Unit::USER) {
      scale_fit(v.value, &c->scale_x);
    }
  }

  for (const auto& v : c->y) {
    if (v.unit == Unit::USER) {
      scale_fit(v.value, &c->scale_y);
    }
  }

  /* check configuration */
  if (c->x.size() != c->y.size()) {
    return error(
        ERROR,
        "the length of the 'data-x' and 'data-y' properties must be equal");
  }

  /* convert color data */
  for (const auto& value : data_colors) {
    Color color;
    if (color_map) {
      if (auto rc = color_map(value, &color); !rc) {
        return rc;
      }
    } else {
      if (auto rc = color.parse(value); !rc) {
        return errorf(
            ERROR,
            "invalid data; can't parse '{}' as a color hex code; maybe you "
            "forgot to set the 'color-map' option?",
            value);
      }
    }

    c->colors.push_back(color);
  }

  /* convert size data */
  for (const auto& value : data_sizes) {
    Measure m;
    if (size_map) {
      if (auto rc = size_map(value, &m); !rc) {
        return rc;
      }
    } else {
      if (auto rc = parse_measure(value, &m); !rc) {
        return rc;
      }
    }

    c->sizes.push_back(m);
  }

  return OK;
}

ReturnCode points_draw(
    Context* ctx,
    PlotConfig* plot,
    const Expr* expr) {
  PlotPointsConfig conf;

  if (auto rc = points_configure(ctx, plot, &conf, expr); !rc) {
    return rc;
  }

  return points_draw(ctx, plot, &conf);
}

ReturnCode points_autorange(
    Context* ctx,
    PlotConfig* plot,
    const Expr* expr) {
  PlotPointsConfig conf;
  return points_configure(ctx, plot, &conf, expr);
}

} // namespace clip::plotgen

