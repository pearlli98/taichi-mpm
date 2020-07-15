/*******************************************************************************
    Copyright (c) The Taichi Authors (2016- ). All Rights Reserved.
    The use of this software is governed by the LICENSE file.
*******************************************************************************/

#pragma once

#include "taichi/dynamics/fluid2d/euler_liquid.h"

TC_NAMESPACE_BEGIN

class EulerSmoke : public EulerLiquid {
 protected:
  virtual void emit(real delta_t);

  virtual void substep(real delta_t) override;

  Array<real> temperature;
  real buoyancy_alpha;
  real buoyancy_beta;

 public:
  EulerSmoke() {
  }

  virtual void apply_external_forces(real delta_t) override;

  virtual void initialize(const Config &config) override;
};

TC_NAMESPACE_END
