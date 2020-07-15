/*******************************************************************************
    Copyright (c) The Taichi Authors (2016- ). All Rights Reserved.
    The use of this software is governed by the LICENSE file.
*******************************************************************************/

#include <taichi/image/tone_mapper.h>
#include <taichi/math/array_op.h>
#include <taichi/math/array_3d.h>
#include <taichi/physics/physics_constants.h>

TC_NAMESPACE_BEGIN

class HETMO final : public ToneMapper {
 protected:
  int num_bins;

 public:
  void initialize(const Config &config) override {
    num_bins = config.get<int>("num_bins");
  }

  virtual Array2D<Vector3> apply(const Array2D<Vector3> &inp) override {
    int width = inp.get_width(), height = inp.get_height();
    Array2D<real> lum(inp.get_res());
    for (auto &ind : inp.get_region()) {
      lum[ind] = luminance(inp[ind]);
    }
    auto scale = num_bins / (1e-30f + lum.max());
    std::vector<int> cdf(num_bins, 0);
    for (auto &ind : inp.get_region()) {
      cdf[std::min(num_bins - 1, (int)(scale * lum[ind]))] += 1;
    }
    for (int i = 0; i < num_bins - 1; i++) {
      cdf[i + 1] += cdf[i];
    }
    auto oup = inp;
    for (auto &ind : oup.get_region()) {
      real new_lum = 1.0_f *
                     cdf[std::min(num_bins - 1, (int)(scale * lum[ind]))] /
                     (width * height);
      for (int i = 0; i < 3; i++) {
        oup[ind][i] = inp[ind][i] / (lum[ind] + 1e-30f) * new_lum;
      }
    }
    return oup;
  }
};

TC_IMPLEMENTATION(ToneMapper, HETMO, "he")

class CLAHETMO final : public ToneMapper {
 protected:
  int num_bins;
  int num_slices;
  real contrast_limit;

 public:
  void initialize(const Config &config) override {
    num_bins = config.get<int>("num_bins");
    num_slices = config.get<int>("num_slices");
    contrast_limit = config.get("contrast_limit", 0.0_f);
  }

  virtual Array2D<Vector3> apply(const Array2D<Vector3> &inp) override {
    int width = inp.get_width(), height = inp.get_height();
    int x_slices = num_slices;
    int y_slices = num_slices;
    int x_slice_size = (int)std::ceil(1.0_f * width / num_slices);
    int y_slice_size = (int)std::ceil(1.0_f * height / num_slices);
    Array2D<real> lum(inp.get_res());
    for (auto &ind : inp.get_region()) {
      lum[ind] = luminance(inp[ind]);
    }
    real max_lum = lum.max() + 1e-20f;
    real scale = num_bins / max_lum;
    Array3D<real> histograms(Vector3i(x_slices, y_slices, num_bins));
    for (int i = 0; i < x_slices; i++) {
      for (int j = 0; j < y_slices; j++) {
        int x_start = std::max(0, (i - 1) * x_slice_size);
        int x_end = std::min((i + 2) * x_slice_size, width);
        int y_start = std::max(0, (j - 1) * y_slice_size);
        int y_end = std::min((j + 2) * y_slice_size, height);
        std::vector<int> cdf(num_bins, 0);
        for (int x = x_start; x < x_end; x++) {
          for (int y = y_start; y < y_end; y++) {
            cdf[std::min(num_bins - 1, (int)(scale * lum[x][y]))] += 1;
          }
        }
        int num_pixels = (x_end - x_start) * (y_end - y_start);
        int threshold = int(1.0_f * num_pixels / num_bins * contrast_limit);
        int clipped = 0;
        if (contrast_limit != 0.0_f) {
          for (int k = 0; k < num_bins; k++) {
            if (cdf[k] > threshold) {
              clipped += cdf[k] - threshold;
              cdf[k] = threshold;
            }
          }
          int gain = clipped / num_bins;
          for (int k = 0; k < num_bins; k++) {
            cdf[k] += gain;
          }
        }
        for (int k = 0; k < num_bins - 1; k++) {
          cdf[k + 1] += cdf[k];
        }
        real inv_scale = 1.0_f / num_pixels;
        for (int k = num_bins - 2; k >= 0; k--) {
          cdf[k + 1] = cdf[k];
        }
        cdf[0] = 0;
        for (int k = 0; k < num_bins; k++) {
          histograms[i][j][k] = cdf[k] * inv_scale;
        }
      }
    }

    auto oup = inp;
    for (auto &ind : oup.get_region()) {
      real new_lum = histograms.sample_relative_coord(
          1.0_f * ind.i / width, 1.0_f * ind.j / height, lum[ind] / max_lum);
      for (int i = 0; i < 3; i++) {
        oup[ind][i] = inp[ind][i] / (lum[ind] + 1e-30f) * new_lum;
      }
    }
    return oup;
  }
};

TC_IMPLEMENTATION(ToneMapper, CLAHETMO, "clahe")

TC_NAMESPACE_END