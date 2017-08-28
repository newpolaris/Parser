#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace client
{
    struct scene
    {
        glm::int32 orthogonal;
        glm::float32 head, pitch, zoom2;
        glm::vec3 pos, lookat, ambient;
    };

    struct object
    {
        std::string name;
        glm::int32 visible, locking, shading, color_type;
        glm::float32 facet;
        glm::vec3 color;
        std::vector<glm::vec3> vertex;
    };

    struct mqo
    {
        scene scene_;
        std::vector<object> objects_;
    };

    bool parse_mqo( const std::string& file_name, client::mqo& mqo);
}