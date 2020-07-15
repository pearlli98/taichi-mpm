/*******************************************************************************
    Copyright (c) The Taichi Authors (2016- ). All Rights Reserved.
    The use of this software is governed by the LICENSE file.
*******************************************************************************/

#include "bidirectional_renderer.h"

TC_NAMESPACE_BEGIN

class BDPTRenderer : public BidirectionalRenderer {
 protected:
 public:
  virtual void initialize(const Config &config) override {
    BidirectionalRenderer::initialize(config);
  }

  void render_stage() override {
    int num_samples = width * height / stage_frequency;
    auto func = [&](int i) {
      auto state_sequence = RandomStateSequence(sampler, sample_count + i);
      Path eye_path = trace_eye_path(state_sequence);
      Path light_path = trace_light_path(state_sequence);
      PathContribution pc = connect(eye_path, light_path);
      write_path_contribution(pc);
    };
    ThreadedTaskManager::run(func, 0, num_samples, num_threads);
    sample_count += num_samples;
  }
};

TC_IMPLEMENTATION(Renderer, BDPTRenderer, "bdpt");

TC_NAMESPACE_END
