#pragma once

#include <limits>
#include <tuple>
#include <optional>
#include <vector>

#include "geometry.h"
#include "geometry_functions.h"

namespace uc
{
    namespace math
    {
        namespace convex_clipping
        {
            struct convex_polyhedron
            {
                struct polygon
                {
                    std::vector<uint32_t> m_indices;
                };

                std::vector<float4>     m_points;
                std::vector<polygon>    m_faces;    
            };

            std::optional< convex_polyhedron > clip(const frustum_points& f, const aabb& b);
            std::optional< convex_polyhedron > clip(const convex_polyhedron& f, const aabb& b);

            //move vector facing polygons along the vector up to the clip_body. alpha is the diagonal of the clip_body
            convex_polyhedron convex_hull_with_direction(const convex_polyhedron& body, float4 vector);
            convex_polyhedron convex_hull_with_point(const convex_polyhedron& body, float4 point);
            convex_polyhedron convex_hull_with_direction(const convex_polyhedron& body, float4 vector, const aabb& clip_body);
        }
    }
}


