////////////////////////////////////////////////////////////////////////////////
// C++ include
#include <iostream>
#include <string>
#include <vector>
#include <limits>

// Shared rendering utilities
#include "utils.h"

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Shortcut to avoid Eigen:: everywhere
using namespace Eigen;



const std::string filename("raytrace.png");

// Camera settings
const double focal_length = 10;
const double field_of_view = 0.7854; // 45 degrees
const double image_z = 5;
const bool is_perspective = true;
const Vector3d camera_position(0, 0, 5);
const double camera_aperture = 0.05;

// Maximum number of recursive calls
const int max_bounce = 5;

// Objects
std::vector<Vector3d> sphere_centers;
std::vector<double> sphere_radii;
std::vector<Matrix3d> parallelograms;

// Object material
const Vector4d obj_ambient_color(0.5, 0.1, 0.1, 0);
const Vector4d obj_diffuse_color(0.5, 0.5, 0.5, 0);
const Vector4d obj_specular_color(0.2, 0.2, 0.2, 0);
const double obj_specular_exponent = 256.0;
const Vector4d obj_reflection_color(0.7, 0.7, 0.7, 0);

// Plane material
Vector4d parallelogram_diffuse_color = obj_diffuse_color;
Vector4d parallelogram_ambient_color = obj_ambient_color;
Vector4d parallelogram_specular_color = obj_specular_color;
Vector4d parallelogram_reflection_color = obj_reflection_color;

// Perlin noise grid
const int grid_size = 20;
std::vector<std::vector<Vector2d>> grid;

// Lights
std::vector<Vector3d> light_positions;
std::vector<Vector4d> light_colors;
const Vector4d ambient_light(0.2, 0.2, 0.2, 0);

void setup_scene()
{
    grid.resize(grid_size + 1);
    for (int i = 0; i < grid_size + 1; ++i)
    {
        grid[i].resize(grid_size + 1);
        for (int j = 0; j < grid_size + 1; ++j)
            grid[i][j] = Vector2d::Random().normalized();
    }

    // Spheres
    sphere_centers.emplace_back(10, 0, 1);
    sphere_radii.emplace_back(1);
    sphere_centers.emplace_back(7, 0.05, -1);
    sphere_radii.emplace_back(1);
    sphere_centers.emplace_back(4, 0.1, 1);
    sphere_radii.emplace_back(1);
    sphere_centers.emplace_back(1, 0.2, -1);
    sphere_radii.emplace_back(1);
    sphere_centers.emplace_back(-2, 0.4, 1);
    sphere_radii.emplace_back(1);
    sphere_centers.emplace_back(-5, 0.8, -1);
    sphere_radii.emplace_back(1);
    sphere_centers.emplace_back(-8, 1.6, 1);
    sphere_radii.emplace_back(1);

    // Plane
    parallelograms.emplace_back();
    parallelograms.back() << -100, 100, -100,
        -1.25, 0, -1.2,
        -100, -100, 100;

    // Lights
    light_positions.emplace_back(8, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
    light_positions.emplace_back(6, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
    light_positions.emplace_back(4, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
    light_positions.emplace_back(2, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
    light_positions.emplace_back(0, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
    light_positions.emplace_back(-2, -8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
    light_positions.emplace_back(-4, 8, 0);
    light_colors.emplace_back(16, 16, 16, 0);
}


// Intersection code


double ray_sphere_intersection(const Vector3d& ray_origin, const Vector3d& ray_direction, int index, Vector3d& p, Vector3d& N)
{
    const Vector3d sphere_center = sphere_centers[index];
    const double sphere_radius = sphere_radii[index];

    Vector3d eo = ray_origin - sphere_center;

    double A = ray_direction.dot(ray_direction);
    double B = 2.0 * ray_direction.dot(eo);
    double C = eo.dot(eo) - sphere_radius * sphere_radius;

    double discriminant = B * B - 4 * A * C;
    if (discriminant < 0) return -1;

    double sqrt_disc = sqrt(discriminant);
    double t1 = (-B - sqrt_disc) / (2 * A);
    double t2 = (-B + sqrt_disc) / (2 * A);

    double t = (t1 >= 0) ? t1 : ((t2 >= 0) ? t2 : -1);
    if (t < 0) return -1;

    p = ray_origin + t * ray_direction;
    N = (p - sphere_center).normalized();
    return t;
}

double ray_parallelogram_intersection(const Vector3d& ray_origin, const Vector3d& ray_direction, int index, Vector3d& p, Vector3d& N)
{
    const Vector3d pgram_origin = parallelograms[index].col(0);
    const Vector3d A = parallelograms[index].col(1);
    const Vector3d B = parallelograms[index].col(2);
    const Vector3d u = A - pgram_origin;
    const Vector3d v = B - pgram_origin;

    Matrix3d M;
    M.col(0) = ray_direction;
    M.col(1) = -u;
    M.col(2) = -v;

    Vector3d rhs = pgram_origin - ray_origin;
    Vector3d solution = M.fullPivLu().solve(rhs);
    double t = solution(0);
    double a = solution(1);
    double b = solution(2);

    if (t < 0 || a < 0 || a > 1 || b < 0 || b > 1) return -1;

    p = ray_origin + t * ray_direction;
    N = u.cross(v).normalized();
    if (N.dot(ray_direction) > 0) N = -N;

    return t;
}

int find_nearest_object(const Vector3d& ray_origin, const Vector3d& ray_direction, Vector3d& p, Vector3d& N)
{
    int closest_index = -1;
    double closest_t = std::numeric_limits<double>::max();
    Vector3d tmp_p, tmp_N;

    for (int i = 0; i < sphere_centers.size(); ++i)
    {
        double t = ray_sphere_intersection(ray_origin, ray_direction, i, tmp_p, tmp_N);
        if (t >= 0 && t < closest_t)
        {
            closest_index = i;
            closest_t = t;
            p = tmp_p;
            N = tmp_N;
        }
    }

    for (int i = 0; i < parallelograms.size(); ++i)
    {
        double t = ray_parallelogram_intersection(ray_origin, ray_direction, i, tmp_p, tmp_N);
        if (t >= 0 && t < closest_t)
        {
            closest_index = sphere_centers.size() + i;
            closest_t = t;
            p = tmp_p;
            N = tmp_N;
        }
    }

    return closest_index;
}

bool is_light_visible(const Vector3d& ray_origin, const Vector3d& ray_direction, const Vector3d& light_position)
{
    Vector3d p_tmp, N_tmp;
    int hit_id = find_nearest_object(ray_origin, ray_direction, p_tmp, N_tmp);

    if (hit_id < 0) return true;

    double dist_to_light = (light_position - ray_origin).norm();
    double dist_to_hit = (p_tmp - ray_origin).norm();

    return dist_to_hit > dist_to_light - 1e-4;
}


// Perlin noise helper funcs


double lerp(double a0, double a1, double w)
{
    return a0 + w * (a1 - a0);
}

double cubic(double w)
{
    return w * w * (3.0 - 2.0 * w);
}

double dotGridGradient(int ix, int iy, double x, double y)
{
    Vector2d gradient = grid[ix][iy];
    double dx = x - double(ix);
    double dy = y - double(iy);
    return dx * gradient[0] + dy * gradient[1];
}

double perlin(double x, double y)
{
    int x0 = int(x);
    int x1 = x0 + 1;
    int y0 = int(y);
    int y1 = y0 + 1;

    double sx = x - double(x0);
    double sy = y - double(y0);
    double u = cubic(sx);
    double v = cubic(sy);

    double n0 = dotGridGradient(x0, y0, x, y);
    double n1 = dotGridGradient(x1, y0, x, y);
    double ix0 = lerp(n0, n1, u);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    double ix1 = lerp(n0, n1, u);

    double value = lerp(ix0, ix1, v);
    return value;
}

// Texture

Vector4d procedural_texture(const double tu, const double tv)
{
    double noise = perlin(tu * grid_size, tv * grid_size);
    noise = (noise + 1.0) / 2.0; // Normalize to [0,1]
    return Vector4d(0.0, noise, 0.0, 0.0); // Only green channel
}


// Raytracer


Vector4d shoot_ray(const Vector3d& ray_origin, const Vector3d& ray_direction, int max_bounce)
{
    Vector3d p, N;
    int nearest_object = find_nearest_object(ray_origin, ray_direction, p, N);

    if (nearest_object < 0) return Vector4d(0, 0, 0, 0);

    Vector4d ambient_color, diff_color, spec_color, refl_color;

    if (nearest_object < sphere_centers.size())
    {
        ambient_color = obj_ambient_color;
        diff_color = obj_diffuse_color;
        spec_color = obj_specular_color;
        refl_color = obj_reflection_color;

        if (nearest_object == 4)
        {
            const double x = p(0) - sphere_centers[nearest_object][0];
            const double y = p(1) - sphere_centers[nearest_object][1];
            const double z = p(2) - sphere_centers[nearest_object][2];
            double tu = acos(z / sphere_radii[nearest_object]) / 3.1415;
            double tv = (3.1415 + atan2(y, x)) / (2 * 3.1415);
            tu = std::min(std::max(tu, 0.0), 1.0);
            tv = std::min(std::max(tv, 0.0), 1.0);
            diff_color = procedural_texture(tu, tv);
        }
    }
    else
    {
        ambient_color = parallelogram_ambient_color;
        diff_color = parallelogram_diffuse_color;
        spec_color = parallelogram_specular_color;
        refl_color = parallelogram_reflection_color;
    }

    const Vector4d ambient = ambient_color.array() * ambient_light.array();
    Vector4d lights_color(0, 0, 0, 0);

    for (int i = 0; i < light_positions.size(); ++i)
    {
        const Vector3d& light_position = light_positions[i];
        const Vector4d& light_color = light_colors[i];

        Vector3d Li = (light_position - p).normalized();
        Vector3d shadow_ray_origin = p + 1e-3 * N;
        Vector3d shadow_ray_dir = (light_position - shadow_ray_origin).normalized();

        if (!is_light_visible(shadow_ray_origin, shadow_ray_dir, light_position))
            continue;

        Vector4d diffuse = diff_color * std::max(Li.dot(N), 0.0);
        Vector3d view_dir = (camera_position - p).normalized();
        Vector3d half_vector = (Li + view_dir).normalized();
        double spec_angle = std::max(N.dot(half_vector), 0.0);
        Vector4d specular = spec_color * pow(spec_angle, obj_specular_exponent);

        Vector3d D = light_position - p;
        lights_color += (diffuse + specular).cwiseProduct(light_color) / D.squaredNorm();
    }

    Vector4d reflection_color(0, 0, 0, 0);
    if (max_bounce > 0)
    {
        Vector3d view_dir = -ray_direction.normalized();
        Vector3d reflect_dir = 2.0 * N.dot(view_dir) * N - view_dir;
        reflect_dir.normalize();

        Vector3d reflect_origin = p + 1e-4 * N;
        reflection_color = shoot_ray(reflect_origin, reflect_dir, max_bounce - 1).cwiseProduct(refl_color);
    }

    Vector4d C = ambient + lights_color + reflection_color;
    C(3) = 1;
    return C;
}

////////////////////////////////////////////////////////////////////////////////

void raytrace_scene()
{
    std::cout << "Simple ray tracer." << std::endl;
    int w = 800, h = 400;
    MatrixXd R = MatrixXd::Zero(w, h), G = MatrixXd::Zero(w, h), B = MatrixXd::Zero(w, h), A = MatrixXd::Zero(w, h);

    double aspect_ratio = double(w) / double(h);
    double image_y = tan(field_of_view / 2.0) * focal_length;
    double image_x = image_y * aspect_ratio;

    const Vector3d image_origin(-image_x, image_y, -image_z);
    const Vector3d x_displacement((2.0 * image_x) / w, 0, 0);
    const Vector3d y_displacement(0, (-2.0 * image_y) / h, 0);

    for (unsigned i = 0; i < w; ++i)
    {
        for (unsigned j = 0; j < h; ++j)
        {
            Vector3d pixel_center = image_origin + (i + 0.5) * x_displacement + (j + 0.5) * y_displacement;
            Vector3d ray_origin, ray_direction;

            if (is_perspective)
            {
                ray_origin = camera_position;
                Vector3d pixel_pos(pixel_center[0], pixel_center[1], -image_z);
                ray_direction = (pixel_pos - camera_position).normalized();
            }
            else
            {
                ray_origin = camera_position + Vector3d(pixel_center[0], pixel_center[1], 0);
                ray_direction = Vector3d(0, 0, -1);
            }

            Vector4d C = shoot_ray(ray_origin, ray_direction, max_bounce);
            R(i, j) = C(0);
            G(i, j) = C(1);
            B(i, j) = C(2);
            A(i, j) = C(3);
        }
    }

    write_matrix_to_png(R, G, B, A, filename);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    setup_scene();
    raytrace_scene();
    return 0;
}
