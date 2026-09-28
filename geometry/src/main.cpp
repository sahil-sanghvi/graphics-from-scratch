////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include <complex>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>

#include <Eigen/Dense>
// Shortcut to avoid  everywhere, DO NOT USE IN .h
using namespace Eigen;
////////////////////////////////////////////////////////////////////////////////

const std::string root_path = DATA_DIR;

// Computes the determinant of the matrix whose columns are the vector u and v
double inline det(const Vector2d &u, const Vector2d &v)
{
    return u.x() * v.y() - u.y() * v.x();
}

// Return true iff [a,b] intersects [c,d]
bool intersect_segment(const Vector2d &a, const Vector2d &b, const Vector2d &c, const Vector2d &d)
{
    Vector2d r = b - a;
    Vector2d s = d - c;
    double denom = det(r, s);
    if (denom == 0.0) return false;
    double t = det(c - a, s) / denom;
    double u = det(c - a, r) / denom;
    return (t >= 0 && t <= 1 && u >= 0 && u <= 1);
}

////////////////////////////////////////////////////////////////////////////////

bool is_inside(const std::vector<Vector2d> &poly, const Vector2d &query)
{
    Vector2d min = poly[0], max = poly[0];
    for (const auto& p : poly) {
        min = min.cwiseMin(p);
        max = max.cwiseMax(p);
    }

    Vector2d outside(max.x() + 1, query.y() + 1);
    int count = 0;

    for (size_t i = 0; i < poly.size(); ++i) {
        const Vector2d& a = poly[i];
        const Vector2d& b = poly[(i + 1) % poly.size()];
        if (intersect_segment(query, outside, a, b)) count++;
    }

    return (count % 2) == 1;
}

////////////////////////////////////////////////////////////////////////////////

std::vector<Vector2d> load_xyz(const std::string& filename)
{
    std::vector<Vector2d> points;
    std::ifstream in(filename);

    if (!in) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return points;
    }

    int n;
    in >> n;
    for (int i = 0; i < n; ++i) {
        double x, y, z;
        in >> x >> y >> z;
        points.push_back(Vector2d(x, y));
    }
    return points;
}

void save_xyz(const std::string& filename, const std::vector<Vector2d>& points)
{
    std::ofstream out(filename);
    out << points.size() << std::endl;
    for (const auto& p : points) {
        out << p.x() << " " << p.y() << " 0" << std::endl;
    }
}

std::vector<Vector2d> load_obj(const std::string& filename)
{
    std::ifstream in(filename);
    std::vector<Vector2d> points;
    std::vector<Vector2d> poly;
    char key;
    while (in >> key)
    {
        if (key == 'v')
        {
            double x, y, z;
            in >> x >> y >> z;
            points.push_back(Vector2d(x, y));
        }
        else if (key == 'f')
        {
            std::string line;
            std::getline(in, line);
            std::istringstream ss(line);
            int id;
            while (ss >> id)
            {
                poly.push_back(points[id - 1]);
            }
        }
    }
    return poly;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    const std::string points_path = root_path + "/points.xyz";
    const std::string poly_path = root_path + "/polygon.obj";

    std::vector<Vector2d> points = load_xyz(points_path);

    ////////////////////////////////////////////////////////////////////////////////
    //Point in polygon
    std::vector<Vector2d> poly = load_obj(poly_path);
    std::vector<Vector2d> result;
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (is_inside(poly, points[i]))
        {
            result.push_back(points[i]);
        }
    }
    save_xyz("output.xyz", result);

    return 0;
}