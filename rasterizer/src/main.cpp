// C++ include
#include <iostream>

#include <string>

#include <vector>

// Shared rendering utilities
#include "raster.h"


#include <gif.h>
#include <fstream>


#include <Eigen/Geometry>

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in the build!
#include "stb_image_write.h"
#define MATH_PI 3.14159265358979323846


using namespace std;
using namespace Eigen;

// Image height
const int H = 480;

// Camera settings
const double near_plane = 1.5; // AKA focal length
const double far_plane = near_plane * 100;
const double field_of_view = 0.7854; // 45 degrees
const double aspect_ratio = 1.5;
const bool isPerspective = true;
const Vector3d camera_position(0, 0, 3);
const Vector3d camera_gaze(0, 0, -1);
const Vector3d camera_top(0, 1, 0);

// Object
// #define DATA_DIR "../data/"
const std::string data_dir = DATA_DIR;
const std::string mesh_filename(data_dir + "bunny.off");
MatrixXd vertices; // n x 3 matrix (n points)
MatrixXi facets;   // m x 3 matrix (m triangles)

// Material for the object
const Vector3d obj_diffuse_color(0.5, 0.5, 0.5);
const Vector3d obj_specular_color(0.2, 0.2, 0.2);
const double obj_specular_exponent = 256.0;

// Lights
std::vector<Vector3d> lightPositions;
std::vector<Vector3d> lightColors;
// Ambient light
const Vector3d ambient_light(0.3, 0.3, 0.3);

// Fills the different arrays
void setup_scene()
{
    // Loads file
    std::ifstream in(mesh_filename);
    if (!in.good())
    {
        std::cerr << "Invalid file " << mesh_filename << std::endl;
        exit(1);
    }
    std::string token;
    in >> token;
    int nv, nf, ne;
    in >> nv >> nf >> ne;
    vertices.resize(nv, 3);
    facets.resize(nf, 3);
    for (int i = 0; i < nv; ++i)
    {
        in >> vertices(i, 0) >> vertices(i, 1) >> vertices(i, 2);
    }
    for (int i = 0; i < nf; ++i)
    {
        int s;
        in >> s >> facets(i, 0) >> facets(i, 1) >> facets(i, 2);
        assert(s == 3);
    }

    // Lights
    lightPositions.emplace_back(8, 8, 0);
    lightColors.emplace_back(16, 16, 16);

    lightPositions.emplace_back(6, -8, 0);
    lightColors.emplace_back(16, 16, 16);

    lightPositions.emplace_back(4, 8, 0);
    lightColors.emplace_back(16, 16, 16);

    lightPositions.emplace_back(2, -8, 0);
    lightColors.emplace_back(16, 16, 16);

    lightPositions.emplace_back(0, 8, 0);
    lightColors.emplace_back(16, 16, 16);

    lightPositions.emplace_back(-2, -8, 0);
    lightColors.emplace_back(16, 16, 16);

    lightPositions.emplace_back(-4, 8, 0);
    lightColors.emplace_back(16, 16, 16);
}

void build_uniform(UniformAttributes& uniform)
{
    // setup uniform
    // setup camera, compute w, u, v
    // compute the camera transformation
    // setup projection matrix

    Vector3d w = -camera_gaze / camera_gaze.norm();
    Vector3d u = camera_top.cross(w) / (camera_top.cross(w)).norm();
    Vector3d v = w.cross(u);
    Matrix4d cameraTransformation;

    cameraTransformation << Vector4d(u[0], u[1], u[2], 0),
        Vector4d(v[0], v[1], v[2], 0),
        Vector4d(w[0], w[1], w[2], 0),
        Vector4d(camera_position(0), camera_position(1), camera_position(2), 1);
    cameraTransformation = cameraTransformation.inverse().eval();

    double t = tan(field_of_view / 2) * near_plane;
    double r = t * aspect_ratio;
    double l = -r;
    double b = -t;
    double n = -near_plane;
    double f = -far_plane;

    Matrix4d morth;

    morth << Vector4d(2 / (r - l), 0, 0, -(r + l) / (r - l)),
        Vector4d(0, 2 / (t - b), 0, -(t + b) / (t - b)),
        Vector4d(0, 0, 2 / (n - f), -(n + f) / (n - f)),
        Vector4d(0, 0, 0, 1);
    morth.transposeInPlace();

    if (isPerspective)
    {
        // setup prespective camera
        Matrix4d mperspective;
        mperspective << Vector4d(n, 0, 0, 0),
            Vector4d(0, n, 0, 0),
            Vector4d(0, 0, n + f, -f * n),
            Vector4d(0, 0, 1, 0);
        mperspective.transposeInPlace();
        uniform.view = morth * mperspective * cameraTransformation;
    }
    else
    {
        uniform.view = morth * cameraTransformation;
    }
}

void simple_render(Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic>& frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;

    program.VertexShader = [](const VertexAttributes& va,
        const UniformAttributes& uniform)
        {
            // fill the shader
            VertexAttributes outVA;
            outVA.position = uniform.view * va.position;
            return outVA;
        };

    program.FragmentShader = [](const VertexAttributes& va,
        const UniformAttributes& uniform)
        {
            // fill the shader
            return FragmentAttributes(1, 0, 0);
        };

    program.BlendingShader = [](const FragmentAttributes& fa,
        const FrameBufferAttributes& previous)
        {
            // fill the shader
            return FrameBufferAttributes(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
        };

    std::vector<VertexAttributes> vertex_attributes;
    // build the vertex attributes from vertices and facets
    for (int i = 0; i < facets.rows(); i++)
    {
        Vector3i r = facets.row(i);
        for (int j = 0; j < 3; j++)
        {
            // Vector3d vertex = vertices.row(r(j));
            VertexAttributes va(vertices(r(j), 0), vertices(r(j), 1), vertices(r(j), 2));
            vertex_attributes.push_back(va);
        }
    }
    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

Matrix4d compute_rotation(const double alpha)
{
    // Compute the rotation matrix of angle alpha on the y axis around the object barycenter
    Matrix4d result;
    result << Vector4d(cos(alpha), 0, -sin(alpha), 0),
        Vector4d(0, 1, 0, 0),
        Vector4d(sin(alpha), 0, cos(alpha), 0),
        Vector4d(0, 0, 0, 1);

    return result;
}

void wireframe_render(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic>& frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;

    Matrix4d trafo = compute_rotation(alpha);
    uniform.transform = trafo;

    program.VertexShader = [](const VertexAttributes& va,
        const UniformAttributes& uniform)
        {
            // fill the shader
            // return va;
            VertexAttributes outVA;
            outVA.position = uniform.view * uniform.transform * va.position;
            return outVA;
        };

    program.FragmentShader = [](const VertexAttributes& va,
        const UniformAttributes& uniform)
        {
            // fill the shader
            return FragmentAttributes(1, 0, 0);
        };

    program.BlendingShader = [](const FragmentAttributes& fa,
        const FrameBufferAttributes& previous)
        {
            // fill the shader
            return FrameBufferAttributes(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
        };

    std::vector<VertexAttributes> vertex_attributes;

    // generate the vertex attributes for the edges and rasterize the lines
    // use the transformation matrix
    for (int i = 0; i < facets.rows(); ++i)
    {
        Vector3i r = facets.row(i);
        for (int j = 0; j < 3; ++j)
        {
            VertexAttributes va(vertices(r(j), 0), vertices(r(j), 1), vertices(r(j), 2));
            vertex_attributes.push_back(va);
            VertexAttributes va2(vertices(r((j + 1) % 3), 0), vertices(r((j + 1) % 3), 1), vertices(r((j + 1) % 3), 2));
            vertex_attributes.push_back(va2);
        }
    }

    rasterize_lines(program, uniform, vertex_attributes, 0.5, frameBuffer);
}

void get_shading_program(Program& program)
{
    program.VertexShader = [](const VertexAttributes& va,
        const UniformAttributes& uniform)
        {
            // transform the position and the normal
            // compute the correct lighting
            // return va;
            VertexAttributes outVA;
            outVA.position = uniform.view * uniform.transform * va.position;
            outVA.normal = uniform.view * va.normal;
            Vector4d transform_normal = uniform.transform * va.normal;
            // Vector4d light = uniform.light;
            Vector3d lights_color(0, 0, 0);
            for (int i = 0; i < lightPositions.size(); i++)
            {
                const Vector3d& light_position = lightPositions[i];
                const Vector3d& light_color = lightColors[i];
                Vector3d position(outVA.position(0), outVA.position(1), outVA.position(2));
                Vector3d normal(transform_normal(0), transform_normal(1), transform_normal(2));
                const Vector3d Li = (light_position - position).normalized();
                const Vector3d diffuse = obj_diffuse_color * std::max(Li.dot(normal), 0.0);
                Vector3d v = (camera_position - position).normalized();
                const Vector3d h = (((camera_position - position).normalized()) + Li) / (((camera_position - position).normalized()) + Li).norm();
                const Vector3d specular = obj_specular_color * std::pow(std::max(normal.dot(h), 0.0), obj_specular_exponent);
                const Vector3d direction = light_position - position;
                lights_color += (diffuse + specular).cwiseProduct(light_color) / direction.squaredNorm();
            }
            outVA.color = lights_color + ambient_light;
            return outVA;
        };

    program.FragmentShader = [](const VertexAttributes& va,
        const UniformAttributes& uniform)
        {
            // create the correct fragment
            // return FragmentAttributes(1, 0, 0);
            FragmentAttributes out_fa = FragmentAttributes(va.color[0], va.color[1], va.color[2]);
            if (isPerspective)
            {
                out_fa.position = va.position;
            }
            else
            {
                out_fa.position = va.position.cwiseProduct(Vector4d(1, 1, -1, 1));
            }
            return out_fa;
        };

    program.BlendingShader = [](const FragmentAttributes& fa,
        const FrameBufferAttributes& previous)
        {
            // implement the depth check
            // return FrameBufferAttributes(fa.color[0], fa.color[1], fa.color[2], fa.color[3]);
            if (fa.position[2] < previous.depth)
            {
                FrameBufferAttributes out_fb = FrameBufferAttributes(fa.color[0] * 255, fa.color[1] * 255, fa.color[2] * 255, fa.color[3] * 255);
                out_fb.depth = fa.position[2];
                return out_fb;
            }
            else
            {
                return previous;
            }
        };
}

void flat_shading(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic>& frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;
    get_shading_program(program);
    Eigen::Matrix4d trafo = compute_rotation(alpha);
    uniform.transform = trafo;

    std::vector<VertexAttributes> vertex_attributes;
    // compute the normals
    // set material colors

    for (int i = 0; i < facets.rows(); i++)
    {
        Vector3i r = facets.row(i);
        Vector3d a = vertices.row(r(0));
        Vector3d b = vertices.row(r(1));
        Vector3d c = vertices.row(r(2));
        Vector3d normal = (b - a).cross(c - a).normalized();
        Vector4d normal4 = Vector4d(normal(0), normal(1), normal(2), 0);
        for (int k = 0; k < 3; k++)
        {
            VertexAttributes va(vertices(r(k), 0), vertices(r(k), 1), vertices(r(k), 2));
            va.normal = normal4;
            vertex_attributes.push_back(va);
        }
    }

    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

void pv_shading(const double alpha, Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic>& frameBuffer)
{
    UniformAttributes uniform;
    build_uniform(uniform);
    Program program;
    get_shading_program(program);

    Eigen::Matrix4d trafo = compute_rotation(alpha);
    uniform.transform = trafo;

    // compute the vertex normals as vertex normal average

    std::vector<VertexAttributes> vertex_attributes;
    // create vertex attributes
    // set material colors

    Eigen::MatrixXd vertex_normal;
    vertex_normal.resize(vertices.rows(), 3);
    vertex_normal.setZero();
    for (int i = 0; i < facets.rows(); i++)
    {
        Vector3i r = facets.row(i);
        Vector3d a = vertices.row(r(0));
        Vector3d b = vertices.row(r(1));
        Vector3d c = vertices.row(r(2));
        Vector3d normal = (b - a).cross(c - a).normalized();
        vertex_normal.row(r(0)) += normal;
        vertex_normal.row(r(1)) += normal;
        vertex_normal.row(r(1)) += normal;
    }
    for (int i = 0; i < facets.rows(); i++)
    {
        Vector3i r = facets.row(i);
        for (int j = 0; j < 3; j++)
        {
            VertexAttributes v(vertices(r(j), 0), vertices(r(j), 1), vertices(r(j), 2));
            Vector3d n = vertex_normal.row(r(j)).normalized();
            Vector4d normal4(n(0), n(1), n(2), 0);
            v.normal = normal4;
            vertex_attributes.push_back(v);
        }
    }

    rasterize_triangles(program, uniform, vertex_attributes, frameBuffer);
}

int main(int argc, char* argv[])
{
    setup_scene();

    int W = H * aspect_ratio;
    Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic> frameBuffer(W, H);
    vector<uint8_t> image;

    simple_render(frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("simple.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);
    frameBuffer = Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic>(W, H);

    wireframe_render(0, frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("wireframe.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);
    frameBuffer = Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic>(W, H);

    flat_shading(0, frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("flat_shading.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);
    frameBuffer = Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic>(W, H);

    pv_shading(0, frameBuffer);
    framebuffer_to_uint8(frameBuffer, image);
    stbi_write_png("pv_shading.png", frameBuffer.rows(), frameBuffer.cols(), 4, image.data(), frameBuffer.rows() * 4);
    frameBuffer = Eigen::Matrix<FrameBufferAttributes, Eigen::Dynamic, Eigen::Dynamic>(W, H);

    // add the animation
    GifWriter gif;
    GifBegin(&gif, "wireframe_rotate.gif", frameBuffer.rows(), frameBuffer.cols(), 5);
    for (int i = 0; i < 360; i++)
    {
        frameBuffer.setConstant(FrameBufferAttributes());
        wireframe_render(i * MATH_PI / 180, frameBuffer);
        framebuffer_to_uint8(frameBuffer, image);
        GifWriteFrame(&gif, image.data(), frameBuffer.rows(), frameBuffer.cols(), 5);
    }
    GifEnd(&gif);
    GifBegin(&gif, "flat_shading_rotate.gif", frameBuffer.rows(), frameBuffer.cols(), 5);
    for (int i = 0; i < 360; i++)
    {
        frameBuffer.setConstant(FrameBufferAttributes());
        flat_shading(i * MATH_PI / 180, frameBuffer);
        framebuffer_to_uint8(frameBuffer, image);
        GifWriteFrame(&gif, image.data(), frameBuffer.rows(), frameBuffer.cols(), 5);
    }
    GifEnd(&gif);
    GifBegin(&gif, "pv_shading_rotate.gif", frameBuffer.rows(), frameBuffer.cols(), 5);
    for (int i = 0; i < 360; i++)
    {
        frameBuffer.setConstant(FrameBufferAttributes());
        pv_shading(i * MATH_PI / 180, frameBuffer);
        framebuffer_to_uint8(frameBuffer, image);
        GifWriteFrame(&gif, image.data(), frameBuffer.rows(), frameBuffer.cols(), 5);
    }
    GifEnd(&gif);

    return 0;
}