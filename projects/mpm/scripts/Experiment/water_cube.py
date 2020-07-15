import taichi as tc

r = 10

if __name__ == '__main__':
  mpm = tc.dynamics.MPM(
    res=(r + 1, r + 1, r + 1),
    frame_dt=0.01,
    base_delta_t=0.0003,
    num_frames=1,)

levelset = mpm.create_levelset()
levelset.add_plane(tc.Vector(0.0, 1, 0), -0.2)
levelset.set_friction(-1)
mpm.set_levelset(levelset, False)

tex = tc.Texture('rect', bounds=(0.3, 0.3, 0.3)) * 8
# tex = tc.Texture('sphere', center=(0.5, 0.45, 0.5), radius=0.1) * 10
# tex = tex.translate(tc.Vector(0.2, 0.45, 0.5))

def position_function(_):
    return tc.Vector(0.5, 0.5, 0.5)

mpm.add_particles(
  type = 'water',
  pd = True,
  density_tex = tex.id,
  initial_velocity = (0, -2, 0),
  density = 1000,
  color=(0.8, 0.7, 1.0),
  initial_position=(0.5, 0.1, 0.5),
  k = 1e5,
  gamma = 7)

# mpm.add_particles(
#   type = 'rigid',
#   pd = True,
#   density = 2500,
#   codimensional=True,
#   friction = 0,
#   scripted_position=tc.function13(position_function),
#   mesh_fn='$mpm/box.obj')  

mpm.simulate(clear_output_directory=True)
