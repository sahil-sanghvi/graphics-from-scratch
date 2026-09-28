////////////////////////////////////////////////////////////////////////////////
// C++ include
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <algorithm>
#include <numeric>

// Shared rendering utilities
#include "utils.h"

// Image writing library
#define STB_IMAGE_WRITE_IMPLEMENTATION // Do not include this line twice in the build!
#include "stb_image_write.h"

// Shortcut to avoid Eigen:: everywhere, DO NOT USE IN .h
using namespace Eigen;

////////////////////////////////////////////////////////////////////////////////
// Class to store tree
////////////////////////////////////////////////////////////////////////////////
class AABBTree
{
public:
    class Node
    {
    public:
        AlignedBox3d bbox;
        int parent;   // Index of the parent node (-1 for root)
        int left;     // Index of the left child (-1 for a leaf)
        int right;    // Index of the right child (-1 for a leaf)
        int triangle; // Index of the node triangle (-1 for internal nodes)
    };

    std::vector<Node> nodes;
    int root;

    AABBTree() = default;                           // Default empty constructor
    AABBTree(const MatrixXd& V, const MatrixXi& F); // Build a BVH from an existing mesh

private:
    // builds the bvh recursively
    int build_recursive(const MatrixXd& V, const MatrixXi& F, const MatrixXd& centroids, int from, int to, int parent, std::vector<int>& triangles);
};

////////////////////////////////////////////////////////////////////////////////
// Scene setup, global variables
////////////////////////////////////////////////////////////////////////////////
const std::string data_dir = DATA_DIR;
const std::string filename("raytrace.png");
const std::string mesh_filename(data_dir + "bunny.off");

//Camera settings
const double focal_length = 2;
const double field_of_view = 0.7854; //45 degrees
const bool is_perspective = true;
const Vector3d camera_position(0, 0, 2);

// Triangle Mesh
MatrixXd vertices; // n x 3 matrix (n points)
MatrixXi facets;   // m x 3 matrix (m triangles)
AABBTree bvh;

//Material for the object, same material for all objects
const Vector4d obj_ambient_color(0.0, 0.5, 0.0, 0);
const Vector4d obj_diffuse_color(0.5, 0.5, 0.5, 0);
const Vector4d obj_specular_color(0.2, 0.2, 0.2, 0);
const double obj_specular_exponent = 256.0;
const Vector4d obj_reflection_color(0.7, 0.7, 0.7, 0);

// Precomputed (or otherwise) gradient vectors at each grid node
const int grid_size = 20;
std::vector<std::vector<Vector2d>> grid;

//Lights
std::vector<Vector3d> light_positions;
std::vector<Vector4d> light_colors;
//Ambient light
const Vector4d ambient_light(0.2, 0.2, 0.2, 0);

//Fills the different arrays
void setup_scene()
{
    //Loads file
    std::ifstream in(mesh_filename);
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

    //setup tree
    bvh = AABBTree(vertices, facets);

    //Lights
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

////////////////////////////////////////////////////////////////////////////////
// BVH Code
////////////////////////////////////////////////////////////////////////////////

AlignedBox3d bbox_from_triangle(const Vector3d& a, const Vector3d& b, const Vector3d& c)
{
    AlignedBox3d box;
    box.extend(a);
    box.extend(b);
    box.extend(c);
    return box;
}

AABBTree::AABBTree(const MatrixXd& V, const MatrixXi& F)
{
    // Compute the centroids of all the triangles in the input mesh
    MatrixXd centroids(F.rows(), V.cols());
    centroids.setZero();
    for (int i = 0; i < F.rows(); ++i)
    {
        for (int k = 0; k < F.cols(); ++k)
        {
            centroids.row(i) += V.row(F(i, k));
        }
        centroids.row(i) /= F.cols();
    }

    //Vector containing the list of tringle indices
    std::vector<int> triangles(F.rows());
    std::iota(triangles.begin(), triangles.end(), 0);

    root = build_recursive(V, F, centroids, 0, triangles.size(), -1, triangles);
}

int AABBTree::build_recursive(
    const MatrixXd& V,
    const MatrixXi& F,
    const MatrixXd& centroids,
    int from,
    int to,
    int parent,
    std::vector<int>& triangles)
{
    const int n = to - from;
    // reserve space up‐front (optional, but avoids realloctions)
    if (nodes.empty()) nodes.reserve(F.rows() * 2);

    // 1) create node
    int idx = (int)nodes.size();
    nodes.emplace_back();
    Node& node = nodes.back();
    node.parent = parent;

    // 2) leaf?
    if (n == 1)
    {
        int tri = triangles[from];
        node.triangle = tri;
        node.left = node.right = -1;
        // build its bbox
        Vector3d a = V.row(F(tri, 0)).transpose();
        Vector3d b = V.row(F(tri, 1)).transpose();
        Vector3d c = V.row(F(tri, 2)).transpose();
        node.bbox = AlignedBox3d(a, a);
        node.bbox.extend(b);
        node.bbox.extend(c);
        return idx;
    }

    // 3) build centroid‐box
    // start with the first centroid
    Vector3d c0 = centroids.row(triangles[from]).transpose();
    AlignedBox3d cbox(c0, c0);
    for (int i = from + 1; i < to; ++i)
    {
        Vector3d ci = centroids.row(triangles[i]).transpose();
        cbox.extend(ci);
    }

    // 4) pick splitting axis = largest diagonal
    Vector3d ext = cbox.diagonal();
    int axis = 0;
    if (ext.y() > ext.x()) axis = 1;
    if (ext.z() > ext[axis]) axis = 2;

    // 5) partition by that axis
    std::sort(triangles.begin() + from,
        triangles.begin() + to,
        [&](int t0, int t1) {
            return centroids(t0, axis)
                < centroids(t1, axis);
        });
    int mid = from + n / 2;

    // 6) recurse
    node.left = build_recursive(V, F, centroids, from, mid, idx, triangles);
    node.right = build_recursive(V, F, centroids, mid, to, idx, triangles);
    node.triangle = -1; // internal

    // 7) merge children boxes
    const auto& L = nodes[node.left].bbox;
    const auto& R = nodes[node.right].bbox;
    node.bbox = L.merged(R);

    return idx;
}

////////////////////////////////////////////////////////////////////////////////
// Intersection code
////////////////////////////////////////////////////////////////////////////////

double ray_triangle_intersection(const Vector3d& ray_origin,
    const Vector3d& ray_direction,
    const Vector3d& a,
    const Vector3d& b,
    const Vector3d& c,
    Vector3d& p,
    Vector3d& N)
{
    const double EPS = 1e-8;
    Vector3d e1 = b - a, e2 = c - a;
    Vector3d h = ray_direction.cross(e2);
    double det = e1.dot(h);
    if (fabs(det) < EPS) return -1;
    double invDet = 1.0 / det;
    Vector3d s = ray_origin - a;
    double u = invDet * s.dot(h);
    if (u < 0 || u > 1) return -1;
    Vector3d q = s.cross(e1);
    double v = invDet * ray_direction.dot(q);
    if (v < 0 || u + v > 1) return -1;
    double t = invDet * e2.dot(q);
    if (t <= EPS) return -1;
    p = ray_origin + t * ray_direction;
    N = e1.cross(e2).normalized();
    return t;
}
bool ray_box_intersection(
    const Vector3d& O,
    const Vector3d& D,
    const AlignedBox3d& B)
{
    // Slab‐method: intersect 3 pairs of planes
    // Avoid division by zero – use component‐wise inverse
    Vector3d invD = D.cwiseInverse();

    // Compute the t’s where the ray hits the “min” and “max” planes on each axis
    Vector3d t0 = (B.min() - O).cwiseProduct(invD);
    Vector3d t1 = (B.max() - O).cwiseProduct(invD);

    // For each axis take the near‐ and far‐t
    Vector3d tmin = t0.cwiseMin(t1);
    Vector3d tmax = t0.cwiseMax(t1);

    // The ray enters the box at the largest tmin, and leaves at the smallest tmax
    double t_enter = tmin.maxCoeff();
    double t_exit = tmax.minCoeff();

    // We have a hit if we enter before we exit, and exit is ≥ 0
    return (t_enter <= t_exit) && (t_exit >= 0.0);
}


// recursively walk node ‘idx’, updating closest_t/p/N if we hit a leaf closer
bool intersect_node(int idx,
    const Vector3d& O,
    const Vector3d& D,
    double& closest_t,
    Vector3d& outP,
    Vector3d& outN)
{
    if (idx < 0) return false;
    const auto& node = bvh.nodes[idx];

    // 1) AABB‐test
    if (!ray_box_intersection(O, D, node.bbox))
        return false;

    bool hit_something = false;

    // 2) Leaf? test single triangle
    if (node.left < 0 && node.right < 0)
    {
        int tri = node.triangle;
        Vector3d P, N;
        double t = ray_triangle_intersection(
            O, D,
            vertices.row(facets(tri, 0)),
            vertices.row(facets(tri, 1)),
            vertices.row(facets(tri, 2)),
            P, N);

        if (t > 0.0 && t < closest_t)
        {
            closest_t = t;
            outP = P;
            outN = N;
            return true;
        }
        return false;
    }

    // 3) Internal node: recurse both children
    if (intersect_node(node.left, O, D, closest_t, outP, outN)) hit_something = true;
    if (intersect_node(node.right, O, D, closest_t, outP, outN)) hit_something = true;

    return hit_something;
}


//Finds the closest intersecting object returns its index
//In case of intersection it writes into p and N (intersection point and normals)
bool find_nearest_object(const Vector3d& O,
    const Vector3d& D,
    Vector3d& p,
    Vector3d& N)
{
    double closest_t = std::numeric_limits<double>::infinity();
    // walk the tree from its root
    bool hit = intersect_node(bvh.root, O, D, closest_t, p, N);
    return hit;
}

////////////////////////////////////////////////////////////////////////////////
// Raytracer code
////////////////////////////////////////////////////////////////////////////////

Vector4d shoot_ray(const Vector3d& ray_origin, const Vector3d& ray_direction)
{
    //Intersection point and normal, these are output of find_nearest_object
    Vector3d p, N;

    const bool nearest_object = find_nearest_object(ray_origin, ray_direction, p, N);

    if (!nearest_object)
    {
        // Return a transparent color
        return Vector4d(0, 0, 0, 0);
    }

    // Ambient light contribution
    const Vector4d ambient_color = obj_ambient_color.array() * ambient_light.array();

    // Punctual lights contribution (direct lighting)
    Vector4d lights_color(0, 0, 0, 0);
    for (int i = 0; i < light_positions.size(); ++i)
    {
        const Vector3d& light_position = light_positions[i];
        const Vector4d& light_color = light_colors[i];

        Vector4d diff_color = obj_diffuse_color;

        // Diffuse contribution
        const Vector3d Li = (light_position - p).normalized();
        const Vector4d diffuse = diff_color * std::max(Li.dot(N), 0.0);

        // Specular contribution
        const Vector3d Hi = (Li - ray_direction).normalized();
        const Vector4d specular = obj_specular_color * std::pow(std::max(N.dot(Hi), 0.0), obj_specular_exponent);
        // Vector3d specular(0, 0, 0);

        // Attenuate lights according to the squared distance to the lights
        const Vector3d D = light_position - p;
        lights_color += (diffuse + specular).cwiseProduct(light_color) / D.squaredNorm();
    }

    // Rendering equation
    Vector4d C = ambient_color + lights_color;

    //Set alpha to 1
    C(3) = 1;

    return C;
}

////////////////////////////////////////////////////////////////////////////////

void raytrace_scene()
{
    std::cout << "Simple ray tracer." << std::endl;

    int w = 640, h = 480;
    MatrixXd R = MatrixXd::Zero(w, h), G = R, B = R, A = R;

    // compute pixel‐grid half‐extents from FOV + aspect:
    double aspect_ratio = double(w) / double(h);
    double image_y = focal_length * std::tan(field_of_view * 0.5);
    double image_x = aspect_ratio * image_y;

    const Vector3d image_origin(-image_x, image_y, camera_position[2] - focal_length);
    const Vector3d dx(2.0 * image_x / w, 0, 0);
    const Vector3d dy(0, -2.0 * image_y / h, 0);

    for (int i = 0; i < w; ++i) {
        for (int j = 0; j < h; ++j) {
            Vector3d pixel = image_origin + (i + 0.5) * dx + (j + 0.5) * dy;
            Vector3d ro = is_perspective ? camera_position : pixel;
            Vector3d rd = is_perspective
                ? (pixel - camera_position).normalized()
                : Vector3d(0, 0, -1);
            Vector4d C = shoot_ray(ro, rd);
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
