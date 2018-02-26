#include "software_renderer.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "triangulation.h"

using namespace std;

namespace CMU462 {

// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg(SVG& svg) {
  // set top level transformation
  transformation = canvas_to_screen;

  // draw all elements
  for (size_t i = 0; i < svg.elements.size(); ++i) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(0, 0));
  a.x--;
  a.y--;
  Vector2D b = transform(Vector2D(svg.width, 0));
  b.x++;
  b.y--;
  Vector2D c = transform(Vector2D(0, svg.height));
  c.x--;
  c.y++;
  Vector2D d = transform(Vector2D(svg.width, svg.height));
  d.x++;
  d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();
}

void SoftwareRendererImp::set_sample_rate(size_t sample_rate) {
  // Task 4:
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
}

void SoftwareRendererImp::set_render_target(unsigned char* render_target,
                                            size_t width, size_t height) {
  // Task 4:
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
}

void SoftwareRendererImp::draw_element(SVGElement* element) {
  // Task 5 (part 1):
  // Modify this to implement the transformation stack

  switch (element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }
}

// Primitive Drawing //

void SoftwareRendererImp::draw_point(Point& point) {
  Vector2D p = transform(point.position);
  rasterize_point(p.x, p.y, point.style.fillColor);
}

void SoftwareRendererImp::draw_line(Line& line) {
  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line(p0.x, p0.y, p1.x, p1.y, line.style.strokeColor);
}

void SoftwareRendererImp::draw_polyline(Polyline& polyline) {
  Color c = polyline.style.strokeColor;
  float w = polyline.style.strokeWidth;

  if (c.a != 0) {
    int nPoints = polyline.points.size();
    for (int i = 0; i < nPoints - 1; i++) {
      Vector2D p0 = transform(polyline.points[(i + 0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i + 1) % nPoints]);
      rasterize_line(p0.x, p0.y, p1.x, p1.y, c, w);
    }
  }
}

void SoftwareRendererImp::draw_rect(Rect& rect) {
  Color c;

  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(x, y));
  Vector2D p1 = transform(Vector2D(x + w, y));
  Vector2D p2 = transform(Vector2D(x, y + h));
  Vector2D p3 = transform(Vector2D(x + w, y + h));

  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0) {
    rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
    rasterize_triangle(p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c);
  }

  // draw outline
  c = rect.style.strokeColor;
  if (c.a != 0) {
    rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
    rasterize_line(p1.x, p1.y, p3.x, p3.y, c);
    rasterize_line(p3.x, p3.y, p2.x, p2.y, c);
    rasterize_line(p2.x, p2.y, p0.x, p0.y, c);
  }
}

void SoftwareRendererImp::draw_polygon(Polygon& polygon) {
  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if (c.a != 0) {
    // triangulate
    vector<Vector2D> triangles;
    triangulate(polygon, triangles);

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if (c.a != 0) {
    int nPoints = polygon.points.size();
    for (int i = 0; i < nPoints; i++) {
      Vector2D p0 = transform(polygon.points[(i + 0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i + 1) % nPoints]);
      rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
    }
  }
}

void SoftwareRendererImp::draw_ellipse(Ellipse& ellipse) {
  // Extra credit
}

void SoftwareRendererImp::draw_image(Image& image) {
  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image(p0.x, p0.y, p1.x, p1.y, image.tex);
}

void SoftwareRendererImp::draw_group(Group& group) {
  for (size_t i = 0; i < group.elements.size(); ++i) {
    draw_element(group.elements[i]);
  }
}

// Rasterization //

// The input arguments in the rasterization functions
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point(float x, float y, Color color) {
  // fill in the nearest pixel
  int sx = (int)floor(x);
  int sy = (int)floor(y);

  // check bounds
  if (sx < 0 || sx >= target_w) return;
  if (sy < 0 || sy >= target_h) return;

  // fill sample - NOT doing alpha blending!
  render_target[4 * (sx + sy * target_w)] = (uint8_t)(color.r * 255);
  render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)(color.g * 255);
  render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)(color.b * 255);
  render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)(color.a * 255);
}

void SoftwareRendererImp::rasterize_line(float x0, float y0, float x1, float y1,
                                         Color color, float width) {
  // Task 2:
  // Implement line rasterization

#if 0
  // An possible implementation of drawing with thickness.
  // TODO: A fast implementaton of fllling rect
  Vector2D s(x1 - x0, y1 - y0);
  s /= s.norm();
  s *= 0.6f;
  Vector2D p(-s.y, s.x);
  float xx1 = x0 + p.x;
  float yy1 = y0 + p.y;
  float xx2 = x1 + p.x;
  float yy2 = y1 + p.y;
  float xx3 = x1 - p.x;
  float yy3 = y1 - p.y;
  float xx4 = x0 - p.x;
  float yy4 = y0 - p.y;

  rasterize_line_xiaolinwu(xx1, yy1, xx2, yy2, color, width);
  rasterize_line_xiaolinwu(xx2, yy2, xx3, yy3, color, width);
  rasterize_line_xiaolinwu(xx3, yy3, xx4, yy4, color, width);
  rasterize_line_xiaolinwu(xx4, yy4, xx1, yy1, color, width);
#endif
  rasterize_line_xiaolinwu(x0, y0, x1, y1, color, width);
}

void SoftwareRendererImp::rasterize_line_bresenham(float x0, float y0, float x1, float y1,
                                                   Color color, float width) {
  bool steep = abs(x1 - x0) < abs(y1 - y0);
  if (steep) {  // abs(slope) > 1
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int dx = x1 - x0, dy = y1 - y0, y = y0, eps = 0;
  int sign = dy > 0 ? 1 : -1;
  dy = abs(dy);
  for (int x = x0; x <= x1; x++) {
    if (steep) rasterize_point(y, x, color);
    else rasterize_point(x, y, color);
    eps += dy;
    if ((eps << 1) >= dx) {
      y += sign;
      eps -= dx;
    }
  }
}

inline float ipart(float x) {
  return floor(x);
}

inline float round(float x) {
  return ipart(x + 0.5f);
}

inline float fpart(float x) {
  return x - floor(x);
}

inline float rfpart(float x) {
  return 1 - fpart(x);
}

void SoftwareRendererImp::rasterize_line_xiaolinwu(float x0, float y0, float x1, float y1,
                                                   Color color, float width) {
  bool steep = abs(x1 - x0) < abs(y1 - y0);
  if (steep) {  // abs(slope) > 1
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  float dx = x1 - x0;
  float dy = y1 - y0;
  float gradient;
  if (dx == 0.0f)
    gradient = 1.0;
  else
    gradient = dy / dx;

  // handle first endpoint
  float xend = round(x0);
  float yend = y0 + gradient * (xend - x0);
  float xgap = rfpart(x0 + 0.5);
  float xpxl1 = xend;  // this will be used in the main loop
  float ypxl1 = ipart(yend);
  
  if (steep) {
    color.a = rfpart(yend) * xgap;
    rasterize_point(ypxl1, xpxl1, color);
    color.a = fpart(yend) * xgap;
    rasterize_point(ypxl1 + 1, xpxl1, color);
  } else {
    color.a = rfpart(yend) * xgap;
    rasterize_point(xpxl1, ypxl1, color);
    color.a = fpart(yend) * xgap;
    rasterize_point(xpxl1, ypxl1 + 1, color);
  }

  float intery = yend + gradient;  // first y-intersection for the main loop

  // handle first endpoint
  xend = round(x1);
  yend = y1 + gradient * (xend - x1);
  xgap = rfpart(x1 + 0.5);
  float xpxl2 = xend;  // this will be used in the main loop
  float ypxl2 = ipart(yend);
  if (steep) {
    color.a = rfpart(yend) * xgap;
    rasterize_point(ypxl2, xpxl2, color);
    color.a = fpart(yend) * xgap;
    rasterize_point(ypxl2 + 1, xpxl2, color);
  } else {
    color.a = rfpart(yend) * xgap;
    rasterize_point(xpxl2, ypxl2, color);
    color.a = fpart(yend) * xgap;
    rasterize_point(xpxl2, ypxl2 + 1, color);
  }

  if (steep) {
    for (int x = xpxl1 + 1; x <= xpxl2 - 1; ++x) {
      color.a = rfpart(intery);
      rasterize_point(ipart(intery), x, color);
      color.a = fpart(intery);
      rasterize_point(ipart(intery) + 1, x, color);
      intery += gradient;
    }
  } else {
    for (int x = xpxl1 + 1; x <= xpxl2 - 1; ++x) {
      color.a = rfpart(intery);
      rasterize_point(x, ipart(intery), color);
      color.a = fpart(intery);
      rasterize_point(x, ipart(intery) + 1, color);
      intery += gradient;
    }
  }
}

void SoftwareRendererImp::rasterize_triangle(float x0, float y0, float x1,
                                             float y1, float x2, float y2,
                                             Color color) {
  // Task 3:
  // Implement triangle rasterization
}

void SoftwareRendererImp::rasterize_image(float x0, float y0, float x1,
                                          float y1, Texture& tex) {
  // Task 6:
  // Implement image rasterization
}

// resolve samples to render target
void SoftwareRendererImp::resolve(void) {
  // Task 4:
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
  return;
}

}  // namespace CMU462
