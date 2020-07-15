import traceback

from taichi.core import tc_core


class Scene:

  def __init__(self):
    self.c = tc_core.create_scene()

  def add_mesh(self, mesh):
    self.c.add_mesh(mesh.c)

  def __getattr__(self, key):
    return self.c.__getattribute__(key)

  def set_atmosphere_material(self, mat):
    self.c.set_atmosphere_material(mat.c)

  def set_environment_map(self, map, sample_prob=0.5):
    self.c.set_environment_map(map.c, sample_prob)

  def set_camera(self, camera):
    self.c.set_camera(camera.c)

  def __enter__(self):
    pass

  def __exit__(self, exc_type, exc_val, exc_tb):
    if exc_val:
      traceback.print_exception(exc_type, exc_val, exc_tb)
      exit(-1)
    self.finalize()
