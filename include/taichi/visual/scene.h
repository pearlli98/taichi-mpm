/*******************************************************************************
    Copyright (c) The Taichi Authors (2016- ). All Rights Reserved.
    The use of this software is governed by the LICENSE file.
*******************************************************************************/

#pragma once

#include <taichi/geometry/primitives.h>
#include <taichi/visual/camera.h>
#include <taichi/visual/surface_material.h>
#include <taichi/visual/envmap.h>
#include <taichi/visual/volume_material.h>
#include <taichi/physics/physics_constants.h>
#include <taichi/physics/spectrum.h>
#include <taichi/math/discrete_sampler.h>

#include <map>
#include <deque>

TC_NAMESPACE_BEGIN

struct Photon {
  Vector3 pos, dir;
  real energy;
};

// TODO: Rename Mesh -> Object, and use Mesh for purely geoemtric mesh (without
// material)
class Mesh {
 public:
  Mesh() {
  }

  void initialize(const Config &config);
  void set_material(std::shared_ptr<SurfaceMaterial> material);
  void load_from_file(const std::string &file_path,
                      bool reverse_vertices = false);
  std::vector<Triangle> untransformed_triangles;
  void set_untransformed_triangles(const std::vector<Triangle> &triangles) {
    untransformed_triangles = triangles;
  }
  // bounding box
  BoundingBox get_bounding_box() {
    BoundingBox bb;
    bool is_first_triangle = true;
    for (auto t : untransformed_triangles) {
      if (is_first_triangle) {
        bb.lower_boundary = t.v[0];
        bb.upper_boundary = t.v[0];
        is_first_triangle = false;
      }
      // loop for xyz
      for (int i = 0; i < 3; ++i)
        // loop for each vertex
        for (int j = 0; j < 3; ++j) {
          bb.lower_boundary[i] = std::min(bb.lower_boundary[i], t.v[j][i]);
          bb.upper_boundary[i] = std::max(bb.upper_boundary[i], t.v[j][i]);
        }
    }
    return bb;
  }
  std::vector<Triangle> get_triangles() {
    std::vector<Triangle> triangles;
    for (auto t : untransformed_triangles) {
      triangles.push_back(t.get_transformed(transform));
    }
    return triangles;
    /*
    vector<Triangle> triangles;
    for (auto face : faces) {
        deque<Triangle> sub_divs;
        sub_divs.push_back(Triangle(
            vertices[face.vert_ind[0]],
            vertices[face.vert_ind[1]],
            vertices[face.vert_ind[2]],
            0
            ));
        if (sub_div_limit > 0) {
            while (true) {
                Triangle t = sub_divs.front();
                int i;
                if (t.max_edge_length(i) < sub_div_limit)
                    break;
                Vector3 mid = 0.5f * (t.v[i] + t.v[(i + 1) % 3]);
                sub_divs.pop_front();
                sub_divs.push_back(Triangle(t.v[i], mid, t.v[(i + 2) % 3], 0));
                sub_divs.push_back(Triangle(t.v[(i + 2) % 3], mid, t.v[(i + 1) %
    3], 0));
            }
        }
        for (auto &t : sub_divs) {
            push_triangle(
                t.v[0],
                t.v[1],
                t.v[2],
                triangle_count,
                triangles
                );
        }
    }*/
  }

  bool need_voxelization;
  std::vector<Vector3> vertices;
  std::vector<Vector3> normals;
  std::vector<Vector2> uvs;
  real initial_temperature;
  std::vector<Face> faces;
  Matrix4 transform;
  real emission;
  Vector3 color;
  bool const_temp;
  real sub_div_limit;
  Vector3 emission_color;
  std::shared_ptr<SurfaceMaterial> material;
};

struct IntersectionInfo {
  IntersectionInfo() {
    triangle_id = -1;
    intersected = false;
  }

  bool front;
  bool intersected;
  Vector2 uv;
  Vector2 dt_du;
  Vector2 dt_dv;
  Vector2 tri_coord;
  Vector3 pos, normal, geometry_normal;
  SurfaceMaterial *material = nullptr;
  Matrix3 to_local;
  Matrix3 to_world;
  real dist;
  int triangle_id;
};

class Scene {
 public:
  Scene() {
    this->envmap_sample_prob = 0.0_f;
  }

  void set_camera(std::shared_ptr<Camera> camera) {
    this->camera = camera;
  }

  void set_environment_map(std::shared_ptr<EnvironmentMap> envmap,
                           real sample_prob) {
    this->envmap = envmap;
    this->envmap_sample_prob = sample_prob;
  }

  void set_atmosphere_material(std::shared_ptr<VolumeMaterial> vol_mat) {
    this->atmosphere_material = vol_mat;
  }

  std::shared_ptr<VolumeMaterial> get_atmosphere_material() const {
    return this->atmosphere_material;
  }

  void add_mesh(std::shared_ptr<Mesh> mesh);

  void finalize_geometry();

  void finalize_lighting();

  void finalize();

  void update_light_emission_cdf() {
    light_total_emission = 0;
    light_total_area = 0;
    std::vector<real> emissions;
    for (auto tri : emissive_triangles) {
      real e = tri.area * get_mesh_from_triangle_id(tri.id)->emission;
      light_total_emission += e;
      light_total_area += tri.area;
      emissions.push_back(e);
    }
    light_emission_sampler.initialize(emissions);
  }

  real get_average_emission() const {
    return light_total_emission / light_total_area;
  }

  void update_emission_cdf() {
    emission_cdf.clear();
    total_emission = 0;
    for (auto tri : triangles) {
      real e = tri.area * pow(tri.temperature, 4.0_f);
      emission_cdf.push_back(e);
      total_emission += e;
    }
    real inv_tot = 1.0_f / total_emission;
    for (int i = 0; i < (int)emission_cdf.size() - 1; i++) {
      emission_cdf[i + 1] += emission_cdf[i];
    }
    emission_cdf.push_back(1);
    for (auto &e : emission_cdf) {
      e *= inv_tot;
    }
  }

  std::vector<Triangle> &get_triangles() {
    return triangles;
  }

  Triangle get_triangle(int id) const {
    return triangles[id];
  }

  IntersectionInfo get_intersection_info(int triangle_id, Ray &ray);

  const Triangle &sample_triangle_light_emission(real r, real &pdf) const {
    int e_tid = light_emission_sampler.sample(r, pdf);
    return emissive_triangles[e_tid];
  }

  // We really need a light source class that unifies triangles and envmap
  // now...
  void sample_light_source(real r,
                           real &pdf,
                           const Triangle *&triangle,
                           const EnvironmentMap *&envmap) const {
    if (r < envmap_sample_prob) {
      triangle = nullptr;
      envmap = this->envmap.get();
      pdf = envmap_sample_prob;
    } else {
      real scale = 1 - envmap_sample_prob;
      triangle = &sample_triangle_light_emission(
          (r - envmap_sample_prob) / scale, pdf);
      envmap = nullptr;
      pdf *= scale;
    }
  }

  real get_triangle_pdf(int id) const {
    return (1 - envmap_sample_prob) * triangles[id].area *
           get_mesh_from_triangle_id(id)->emission / light_total_emission;
  }

  real get_environment_map_pdf() const {
    return envmap_sample_prob;
  }

  void sample_photon(Photon &p, real r, real delta_t, real weight) {
    int tid = std::min(
        int(std::lower_bound(emission_cdf.begin(), emission_cdf.end(), r) -
            emission_cdf.begin()),
        (int)triangles.size() - 1);
    Triangle &t = triangles[tid];
    Mesh *mesh = triangle_id_to_mesh[tid];
    weight = t.area / total_triangle_area;
    p.dir = random_diffuse(t.normal);
    p.pos = t.sample_point();
    p.energy = weight * total_emission * delta_t * stefan_boltzmann_constant;
    if (!mesh->const_temp) {
      t.temperature -= p.energy / t.heat_capacity;
      t.temperature = std::max(t.temperature, 0.0_f);
    }
  }

  void recieve_photon(int triangle_id, real energy) {
    int tid = triangle_id;
    Triangle &t = triangles[tid];
    Mesh *mesh = triangle_id_to_mesh[tid];
    if (!mesh->const_temp)
      t.temperature += energy / t.heat_capacity;
  }

  real get_temperature(int triangle_id, real u, real v) {
    TC_ERROR("not implemented");
    return 0.0_f;
    /*
    real cooef[3]{ 1 - u - v, u, v };
    Mesh *mesh = triangle_id_to_mesh[triangle_id];
    real temp = 0;
    for (int i = 0; i < 3; i++) {
        int vertice_index = mesh->faces[triangle_id -
    triangle_id_start[mesh]].vert_ind[i];
        temp += mesh->temperature[vertice_index] * cooef[i];
    }
    return temp;
    */
  }

  Vector3 get_coord(int triangle_id, real u, real v) const {
    real cooef[3]{1 - u - v, u, v};
    Mesh *mesh = triangle_id_to_mesh.find(triangle_id)->second;
    Vector3 temp(0);
    for (int i = 0; i < 3; i++) {
      int vertice_index =
          mesh->faces[triangle_id - triangle_id_start.find(mesh)->second]
              .vert_ind[i];
      temp += mesh->vertices[vertice_index] * cooef[i];
    }
    return temp;
  }

  Mesh *get_mesh_from_triangle_id(int triangle_id) const {
    return triangle_id_to_mesh.find(triangle_id)->second;
  }

  real get_triangle_emission(int triangle_id) const {
    return get_mesh_from_triangle_id(triangle_id)->emission;
  }

  DiscreteSampler light_emission_sampler;
  std::shared_ptr<Camera> camera;
  std::vector<Triangle> triangles;
  std::vector<Triangle> emissive_triangles;
  std::vector<real> emission_cdf;
  real total_emission;
  real light_total_emission;
  real light_total_area;
  std::vector<Mesh> meshes;
  std::map<int, Mesh *> triangle_id_to_mesh;
  std::map<Mesh *, int> triangle_id_start;
  int num_triangles;
  real sub_divide_limit;
  real total_triangle_area;
  int resolution_x, resolution_y;
  std::shared_ptr<VolumeMaterial> atmosphere_material;
  std::shared_ptr<EnvironmentMap> envmap;
  real envmap_sample_prob;
};

TC_NAMESPACE_END
