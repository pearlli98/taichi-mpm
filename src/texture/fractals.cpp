/*******************************************************************************
    Copyright (c) The Taichi Authors (2016- ). All Rights Reserved.
    The use of this software is governed by the LICENSE file.
*******************************************************************************/

#include <taichi/visual/texture.h>

TC_NAMESPACE_BEGIN

class MengerSponge : public Texture {
 private:
  int limit;

 public:
  void initialize(const Config &config) override {
    Texture::initialize(config);
    limit = config.get("limit", 10);
  }

  bool cut(const Vector3d &c) const {
    for (int i = 0; i < 3; i++) {
      if (c[i] < 1 / 3.0 || c[i] >= 2 / 3.0) {
        return false;
      }
    }
    return true;
  }

  virtual Vector4 sample(const Vector3 &coord) const override {
    Vector3d c = coord.cast<float64>();
    for (int i = 0; i < limit; i++) {
      if (cut(c))
        return Vector4(0.0_f);
      c = fract(c * 3.0);
    }
    return Vector4(1.0_f);
  }
};

TC_IMPLEMENTATION(Texture, MengerSponge, "menger");

TC_NAMESPACE_END