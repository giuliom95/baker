#ifndef MODEL_HPP
#define MODEL_HPP

#include <vector>

#include "math.hpp"

class Model {
public:
    std::vector<Vec3f> vtxs;
    std::vector<Vec3f> norms;
    std::vector<Vec2f> uvs;

    std::vector<Vec3i> vtris;
    std::vector<Vec3i> ntris;
    std::vector<Vec3i> uvtris;
};

#endif