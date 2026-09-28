// C++ include
#include <iostream>
#include <string>
#include <vector>

// Shared rendering utilities
#include "utils.h"

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in the build!
#include "stb_image_write.h"

// Shortcut to avoid Eigen:: everywhere, DO NOT USE IN .h
using namespace Eigen;

void raytrace_sphere()
{
    std::cout << "Simple ray tracer, one sphere with orthographic projection" << std::endl;

    const std::string filename("sphere_orthographic.png");
    MatrixXd C = MatrixXd::Zero(800, 800);
    MatrixXd A = MatrixXd::Zero(800, 800);

    const Vector3d camera_origin(0, 0, 3);
    const Vector3d camera_view_direction(0, 0, -1);

    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);

    const double sphere_radius = 0.9;
    const Vector3d sphere_center(0, 0, 0);
    const Vector3d light_position(-1, 1, 1);

    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;
            const Vector3d ray_origin = pixel_center;
            const Vector3d ray_direction = camera_view_direction;

            Vector3d oc = ray_origin - sphere_center;
            double a = ray_direction.dot(ray_direction);
            double b = 2.0 * ray_direction.dot(oc);
            double c = oc.dot(oc) - sphere_radius * sphere_radius;
            double discriminant = b * b - 4 * a * c;

            if (discriminant >= 0)
            {
                double t = (-b - sqrt(discriminant)) / (2.0 * a);
                if (t > 0)
                {
                    Vector3d intersection = ray_origin + t * ray_direction;
                    Vector3d normal = (intersection - sphere_center).normalized();
                    Vector3d light_dir = (light_position - intersection).normalized();
                    double intensity = std::max(0.0, normal.dot(light_dir));
                    C(j, i) = intensity;
                    A(j, i) = 1;
                }
            }
        }
    }
    write_matrix_to_png(C, C, C, A, filename);
}

void raytrace_parallelogram()
{
    std::cout << "Simple ray tracer, one parallelogram with orthographic projection" << std::endl;

    const std::string filename("plane_orthographic.png");
    MatrixXd C = MatrixXd::Zero(800, 800);
    MatrixXd A = MatrixXd::Zero(800, 800);

    const Vector3d camera_origin(0, 0, 3);
    const Vector3d camera_view_direction(0, 0, -1);

    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);

    const Vector3d pgram_origin(-0.5, -0.5, 0);
    const Vector3d pgram_u(1, 0.4, 0);
    const Vector3d pgram_v(0, 0.7, -10);
    const Vector3d normal = pgram_u.cross(pgram_v).normalized();
    const Vector3d light_position(-1, 1, 1);

    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;
            const Vector3d ray_origin = pixel_center;
            const Vector3d ray_direction = camera_view_direction;

            double denom = ray_direction.dot(normal);
            if (fabs(denom) > 1e-6)
            {
                double t = (pgram_origin - ray_origin).dot(normal) / denom;
                if (t > 0)
                {
                    Vector3d intersection = ray_origin + t * ray_direction;
                    Vector3d w = intersection - pgram_origin;
                    double a = pgram_u.dot(pgram_u);
                    double b = pgram_u.dot(pgram_v);
                    double c = pgram_v.dot(pgram_v);
                    double d = pgram_u.dot(w);
                    double e = pgram_v.dot(w);
                    double det = a * c - b * b;
                    double s = (d * c - b * e) / det;
                    double t_param = (a * e - b * d) / det;

                    if (s >= 0 && s <= 1 && t_param >= 0 && t_param <= 1)
                    {
                        Vector3d light_dir = (light_position - intersection).normalized();
                        double intensity = std::max(0.0, normal.dot(light_dir));
                        C(j, i) = intensity;
                        A(j, i) = 1;
                    }
                }
            }
        }
    }
    write_matrix_to_png(C, C, C, A, filename);
}

void raytrace_perspective()
{
    std::cout << "Simple ray tracer, one parallelogram with perspective projection" << std::endl;

    const std::string filename("plane_perspective.png");
    MatrixXd C = MatrixXd::Zero(800, 800);
    MatrixXd A = MatrixXd::Zero(800, 800);

    const Vector3d camera_origin(0, 0, 3);
    const Vector3d image_origin(-1, 1, 1);
    const Vector3d x_displacement(2.0 / C.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / C.rows(), 0);

    const Vector3d pgram_origin(-0.5, -0.5, 0);
    const Vector3d pgram_u(1, 0.4, 0);
    const Vector3d pgram_v(0, 0.7, -10);
    const Vector3d normal = pgram_u.cross(pgram_v).normalized();
    const Vector3d light_position(-1, 1, 1);

    for (unsigned i = 0; i < C.cols(); ++i)
    {
        for (unsigned j = 0; j < C.rows(); ++j)
        {
            const Vector3d pixel_center = image_origin + double(i) * x_displacement + double(j) * y_displacement;
            const Vector3d ray_origin = camera_origin;
            const Vector3d ray_direction = (pixel_center - camera_origin).normalized();

            double denom = ray_direction.dot(normal);
            if (fabs(denom) > 1e-6)
            {
                double t = (pgram_origin - ray_origin).dot(normal) / denom;
                if (t > 0)
                {
                    Vector3d intersection = ray_origin + t * ray_direction;
                    Vector3d w = intersection - pgram_origin;
                    double a = pgram_u.dot(pgram_u);
                    double b = pgram_u.dot(pgram_v);
                    double c = pgram_v.dot(pgram_v);
                    double d = pgram_u.dot(w);
                    double e = pgram_v.dot(w);
                    double det = a * c - b * b;
                    double s = (d * c - b * e) / det;
                    double t_param = (a * e - b * d) / det;

                    if (s >= 0 && s <= 1 && t_param >= 0 && t_param <= 1)
                    {
                        Vector3d light_dir = (light_position - intersection).normalized();
                        double intensity = std::max(0.0, normal.dot(light_dir));
                        C(j, i) = intensity;
                        A(j, i) = 1;
                    }
                }
            }
        }
    }
    write_matrix_to_png(C, C, C, A, filename);
}

void raytrace_shading()
{
    std::cout << "Simple ray tracer, one sphere with different shading" << std::endl;
    const std::string filename("shading.png");
    MatrixXd R = MatrixXd::Zero(512, 512);
    MatrixXd G = MatrixXd::Zero(512, 512);
    MatrixXd B = MatrixXd::Zero(512, 512);
    MatrixXd A = MatrixXd::Zero(512, 512);

    const Vector3d camera_origin(0, 0, 2);
    const Vector3d image_origin(-1, 1, 0);
    const Vector3d x_displacement(2.0 / R.cols(), 0, 0);
    const Vector3d y_displacement(0, -2.0 / R.rows(), 0);

    const Vector3d sphere_center(0, 0, -0.2);
    const double sphere_radius = 0.9;

    const Vector3d diffuse_color(1.0, 0.0, 1.0);  // Magenta
    const Vector3d specular_color(0.0, 0.0, 1.0); // Blue
    const double specular_exponent = 100;
    const double ambient = 0.1;

    const Vector3d light_position(-1, 1, 1);

    for (unsigned i = 0; i < R.cols(); ++i)
    {
        for (unsigned j = 0; j < R.rows(); ++j)
        {
            Vector3d pixel_center = image_origin + i * x_displacement + j * y_displacement;
            Vector3d ray_origin = camera_origin;
            Vector3d ray_direction = (pixel_center - camera_origin).normalized();

            Vector3d oc = ray_origin - sphere_center;
            double a = ray_direction.dot(ray_direction);
            double b = 2.0 * ray_direction.dot(oc);
            double c = oc.dot(oc) - sphere_radius * sphere_radius;
            double discriminant = b * b - 4 * a * c;

            if (discriminant >= 0)
            {
                double t = (-b - sqrt(discriminant)) / (2.0 * a);
                if (t > 0)
                {
                    Vector3d intersection = ray_origin + t * ray_direction;
                    Vector3d normal = (intersection - sphere_center).normalized();
                    Vector3d light_dir = (light_position - intersection).normalized();
                    Vector3d view_dir = (camera_origin - intersection).normalized();
                    Vector3d half_vector = (light_dir + view_dir).normalized();

                    double diffuse = std::max(0.0, normal.dot(light_dir));
                    double specular = pow(std::max(0.0, normal.dot(half_vector)), specular_exponent);

                    Vector3d color = ambient * diffuse_color + diffuse * diffuse_color + specular * specular_color;
                    R(j, i) = std::min(1.0, color(0));
                    G(j, i) = std::min(1.0, color(1));
                    B(j, i) = std::min(1.0, color(2));
                    A(j, i) = 1.0;
                }
            }
        }
    }
    write_matrix_to_png(R, G, B, A, filename);
}


int main()
{
    raytrace_sphere();
    raytrace_parallelogram();
    raytrace_perspective();
    raytrace_shading();
    return 0;
}
