/*******************************************************************************
    Copyright (c) The Taichi Authors (2016- ). All Rights Reserved.
    The use of this software is governed by the LICENSE file.
*******************************************************************************/

#pragma once

#include <taichi/common/util.h>
#include <taichi/math/math.h>

TC_NAMESPACE_BEGIN
#undef RGB

class RGB {
 public:
  real r, g, b;

  RGB() {
    r = g = b = 0.0;
  }

  RGB(real r, real g, real b) : r(r), g(g), b(b) {
  }

  operator Vector3() {
    return Vector3(r / 255.0f, g / 255.0f, b / 255.0f);
  }

  void append_to_string(std::string &str) {
    str.push_back((char)int(clamp(r, 0.0_f, 1.0_f) * 255.0));
    str.push_back((char)int(clamp(g, 0.0_f, 1.0_f) * 255.0));
    str.push_back((char)int(clamp(b, 0.0_f, 1.0_f) * 255.0));
  }
};

TC_NAMESPACE_END
