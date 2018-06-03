#include "pch.h"
#include "frustum_aabb_intersection.h"
#include <algorithm>
#include <array>
#include <unordered_set>
#include <functional>
#include <assert.h>


namespace std
{
    template <>
    struct hash<computational_geometry::float3>
    {
        size_t operator()(const computational_geometry::float3& k) const
        {
            // Compute individual hash values for first, second and third
            // http://stackoverflow.com/a/1646913/126995
            size_t res = 17;
            res = res * 31 + hash<float>()(k.m_x);
            res = res * 31 + hash<float>()(k.m_y);
            res = res * 31 + hash<float>()(k.m_z);
            return res;
        }
    };
}

namespace computational_geometry
{
    struct plane
    {
        float3 m_n;
        float  m_d = 0.0f;
    };

    struct edge3d
    {
        float3 m_a;
        float3 m_b;
    };

    struct edge2d
    {
        float2 m_a;
        float2 m_b;
    };

    struct line2d
    {
        float2 m_n;
        float  m_d;
    };

    struct triangle_indexed
    {
        uint8_t m_a;
        uint8_t m_b;
        uint8_t m_c;
        uint8_t m_padding;
    };

    aabb make_aabb(const frustum& f)
    {
        float min_x;
        float min_y;
        float min_z;

        float max_x;
        float max_y;
        float max_z;

        min_x = f.m_points[0].m_x;
        min_y = f.m_points[0].m_y;
        min_z = f.m_points[0].m_z;

        max_x = f.m_points[0].m_x;
        max_y = f.m_points[0].m_y;
        max_z = f.m_points[0].m_z;

        {
            min_x = std::min(f.m_points[1].m_x, min_x);
            min_x = std::min(f.m_points[2].m_x, min_x);
            min_x = std::min(f.m_points[3].m_x, min_x);
            min_x = std::min(f.m_points[4].m_x, min_x);
            min_x = std::min(f.m_points[5].m_x, min_x);
            min_x = std::min(f.m_points[6].m_x, min_x);
            min_x = std::min(f.m_points[7].m_x, min_x);
        }

        {
            min_y = std::min(f.m_points[1].m_y, min_y);
            min_y = std::min(f.m_points[2].m_y, min_y);
            min_y = std::min(f.m_points[3].m_y, min_y);
            min_y = std::min(f.m_points[4].m_y, min_y);
            min_y = std::min(f.m_points[5].m_y, min_y);
            min_y = std::min(f.m_points[6].m_y, min_y);
            min_y = std::min(f.m_points[7].m_y, min_y);
        }

        {
            min_z = std::min(f.m_points[1].m_z, min_z);
            min_z = std::min(f.m_points[2].m_z, min_z);
            min_z = std::min(f.m_points[3].m_z, min_z);
            min_z = std::min(f.m_points[4].m_z, min_z);
            min_z = std::min(f.m_points[5].m_z, min_z);
            min_z = std::min(f.m_points[6].m_z, min_z);
            min_z = std::min(f.m_points[7].m_z, min_z);
        }

        {
            max_x = std::max(f.m_points[1].m_x, max_x);
            max_x = std::max(f.m_points[2].m_x, max_x);
            max_x = std::max(f.m_points[3].m_x, max_x);
            max_x = std::max(f.m_points[4].m_x, max_x);
            max_x = std::max(f.m_points[5].m_x, max_x);
            max_x = std::max(f.m_points[6].m_x, max_x);
            max_x = std::max(f.m_points[7].m_x, max_x);
        }

        {
            max_y = std::max(f.m_points[1].m_y, max_y);
            max_y = std::max(f.m_points[2].m_y, max_y);
            max_y = std::max(f.m_points[3].m_y, max_y);
            max_y = std::max(f.m_points[4].m_y, max_y);
            max_y = std::max(f.m_points[5].m_y, max_y);
            max_y = std::max(f.m_points[6].m_y, max_y);
            max_y = std::max(f.m_points[7].m_y, max_y);
        }

        {
            max_z = std::max(f.m_points[1].m_z, max_z);
            max_z = std::max(f.m_points[2].m_z, max_z);
            max_z = std::max(f.m_points[3].m_z, max_z);
            max_z = std::max(f.m_points[4].m_z, max_z);
            max_z = std::max(f.m_points[5].m_z, max_z);
            max_z = std::max(f.m_points[6].m_z, max_z);
            max_z = std::max(f.m_points[7].m_z, max_z);
        }
        return { {min_x, min_y, min_z}, {max_x, max_y, max_z} };
    }

    plane make_plane(const float3& a, const float3& b, const float3& c)
    {
        float3 u    = b - a;
        float3 v    = c - a;
        float3 n    = normalize(cross(u, v));
        auto     d  = -dot(n, a); //note: if the plane equation is ax+by+cz+d, then this is negative, else positive
        return  { n, d };
    }

    line2d make_line_2d(const float2& a, const float2& b)
    {
        //note: not normalized
        float  x1   = a.m_x;
        float  x2   = b.m_x;

        float  y1   = a.m_y;
        float  y2   = b.m_y;

        float  a_    = y2 - y1;
        float  b_    = x1 - x2;
        float  c_    = x2 * y1 - y2 * x1;

        return { {a_,b_}, c_ };
    }


    std::array< plane, 6 > make_face_planes(const frustum& f )
    {
        std::array< plane, 6 > r;

        //Consistency check, these planes should be like the other ones
        plane  near0    = make_plane(f.m_points[frustum_points::NearBottomLeft], f.m_points[frustum_points::NearTopRight], f.m_points[frustum_points::NearBottomRight]);
        plane  far0     = make_plane(f.m_points[frustum_points::FarBottomRight], f.m_points[frustum_points::FarTopRight], f.m_points[frustum_points::FarBottomLeft]);
        plane  left0    = make_plane(f.m_points[frustum_points::NearBottomLeft],f.m_points[frustum_points:: FarTopLeft], f.m_points[frustum_points::NearTopLeft]);
        plane  right0   = make_plane(f.m_points[frustum_points::NearBottomRight], f.m_points[frustum_points::FarTopRight], f.m_points[frustum_points::FarBottomRight]);
        plane  top0     = make_plane(f.m_points[frustum_points::NearTopRight], f.m_points[frustum_points::FarTopLeft], f.m_points[frustum_points::FarTopRight]);
        plane  bottom0  = make_plane(f.m_points[frustum_points::NearBottomRight], f.m_points[frustum_points::FarBottomLeft], f.m_points[frustum_points::NearBottomLeft]);

        r[frustum_planes::Left]     = left0;
        r[frustum_planes::Right]    = right0;
        r[frustum_planes::Top]      = top0;

        r[frustum_planes::Bottom]   = bottom0;
        r[frustum_planes::Near]     = near0;
        r[frustum_planes::Far]      = far0;

        return r;
    }

    template <uint32_t edge>
    constexpr void get_edge(uint32_t& a, uint32_t& b)
    {
        switch (edge)
        {
            case 0: a = 0; b = 1; break;
            case 1: a = 0; b = 4; break;
            case 2: a = 0; b = 3; break;

            case 3: a = 6; b = 7; break;
            case 4: a = 6; b = 2; break;
            case 5: a = 6; b = 5; break;

            case 6: a = 5; b = 4; break;
            case 7: a = 5; b = 1; break;

            case 8: a = 2; b = 3; break;
            case 9: a = 2; b = 1; break;

            case 10: a = 7; b = 3; break;
            case 11: a = 7; b = 4; break;
        }
    }

    template <uint32_t edge_index>
    edge3d make_edge_3d(const frustum& f)
    {
        uint32_t a = 0;
        uint32_t b = 0;

        get_edge<edge_index>( a, b);
        return { f.m_points[a], f.m_points[b] };
    }

    template <uint32_t edge_index>
    edge3d make_edge_3d(const float3* aabb_points)
    {
        uint32_t a = 0;
        uint32_t b = 0;

        get_edge<edge_index>(a, b);
        return { aabb_points[a], aabb_points[b] };
    }

    std::array<edge3d, 12> make_edges(const frustum& f)
    {
        std::array<edge3d, 12> r;

        r[0]    = make_edge_3d<0>(f);
        r[1]    = make_edge_3d<1>(f);
        r[2]    = make_edge_3d<2>(f);
        r[3]    = make_edge_3d<3>(f);

        r[4]    = make_edge_3d<4>(f);
        r[5]    = make_edge_3d<5>(f);
        r[6]    = make_edge_3d<6>(f);
        r[7]    = make_edge_3d<7>(f);

        r[8]    = make_edge_3d<8>(f);
        r[9]    = make_edge_3d<9>(f);
        r[10]   = make_edge_3d<10>(f);
        r[11]   = make_edge_3d<11>(f);

        return r;
    }

    std::array<triangle_indexed, 12> make_aabb_triangle_indices()
    {
        std::array<triangle_indexed, 12> r;

        //left
        r[0] = { 4, 0, 3 };
        r[1] = { 4, 3, 7 };

        //right
        r[2] = { 1, 5, 6 };
        r[3] = { 1, 6, 2 };

        //top
        r[4] = { 3, 2, 6 };
        r[5] = { 3, 6, 7 };

        //bottom
        r[6] = { 3, 2, 6 };
        r[7] = { 3, 6, 7 };

        //near
        r[8] = { 0, 1, 2 };
        r[9] = { 0, 2, 3 };

        //far
        r[10] = { 5, 4, 7 };
        r[11] = { 5, 7, 6 };

        return r;
    }

    std::array<edge3d, 12> make_edges(const float3* f)
    {
        std::array<edge3d, 12> r;

        r[0] = make_edge_3d<0>(f);
        r[1] = make_edge_3d<1>(f);
        r[2] = make_edge_3d<2>(f);
        r[3] = make_edge_3d<3>(f);

        r[4] = make_edge_3d<4>(f);
        r[5] = make_edge_3d<5>(f);
        r[6] = make_edge_3d<6>(f);
        r[7] = make_edge_3d<7>(f);

        r[8] = make_edge_3d<8>(f);
        r[9] = make_edge_3d<9>(f);
        r[10] = make_edge_3d<10>(f);
        r[11] = make_edge_3d<11>(f);

        return r;
    }

    std::array<line2d, 12> make_lines(const std::array<edge2d, 12> & e)
    {
        std::array<line2d, 12> r;

        for (auto i = 0U; i < 12; ++i)
        {
            r[i] = make_line_2d(e[i].m_a, e[i].m_b);
        }

        return r;
    }

    //returns true if b is inside a
    bool inside(const aabb& a, const aabb& b)
    {
        float d_min_x = b.m_min.m_x - a.m_min.m_x;
        float d_min_y = b.m_min.m_y - a.m_min.m_y;
        float d_min_z = b.m_min.m_z - a.m_min.m_z;

        float d_max_x = b.m_max.m_x - a.m_max.m_x;
        float d_max_y = b.m_max.m_y - a.m_max.m_y;
        float d_max_z = b.m_max.m_z - a.m_max.m_z;

        return (d_min_x * d_max_x < 0.0f && d_min_y * d_max_y < 0.0f && d_min_z * d_max_z < 0.0f);
    }

    //returns true if b is inside a
    bool outside(const aabb& a, const aabb& b)
    {
        bool d_outside_x = b.m_min.m_x > a.m_max.m_x || b.m_max.m_x < a.m_min.m_x;
        bool d_outside_y = b.m_min.m_y > a.m_max.m_y || b.m_max.m_y < a.m_min.m_y;
        bool d_outside_z = b.m_min.m_z > a.m_max.m_z || b.m_max.m_z < a.m_min.m_z;

        return d_outside_x || d_outside_y || d_outside_z;
    }

    std::vector<float3> make_points(const frustum& f)
    {
        std::vector<float3> r;
        r.reserve(8);
        for (auto i = 0U; i < 8; ++i)
        {
            r.push_back(f.m_points[i]);
        }
        return r;
    }

    std::array<float3, 8> make_points(const aabb& f)
    {
        std::array<float3, 8> r;

        r[0] = { f.m_min.m_x, f.m_min.m_y, f.m_min.m_z };
        r[1] = { f.m_max.m_x, f.m_min.m_y, f.m_min.m_z };
        r[2] = { f.m_max.m_x, f.m_max.m_y, f.m_min.m_z };
        r[3] = { f.m_min.m_x, f.m_max.m_y, f.m_min.m_z };

        r[4] = { f.m_min.m_x, f.m_min.m_y, f.m_max.m_z };
        r[5] = { f.m_max.m_x, f.m_min.m_y, f.m_max.m_z };
        r[6] = { f.m_max.m_x, f.m_max.m_y, f.m_max.m_z };
        r[7] = { f.m_min.m_x, f.m_max.m_y, f.m_max.m_z };

        return r;
    }

    enum class plane_aabb_intersection : uint32_t 
    {
        inside          = 0,
        outside         = 1,
        intersection   = 2
    };

    plane_aabb_intersection intersects(const std::array<float3, 8>& a, const plane& p)
    {
        float dots[8];

        dots[0] = dot(p.m_n, a[0]) + p.m_d;
        dots[1] = dot(p.m_n, a[1]) + p.m_d;
        dots[2] = dot(p.m_n, a[2]) + p.m_d;
        dots[3] = dot(p.m_n, a[3]) + p.m_d;

        dots[4] = dot(p.m_n, a[4]) + p.m_d;
        dots[5] = dot(p.m_n, a[5]) + p.m_d;
        dots[6] = dot(p.m_n, a[6]) + p.m_d;
        dots[7] = dot(p.m_n, a[7]) + p.m_d;

        bool positive_half_plane = true;
        bool negative_half_plane = true;

        for (auto i = 0U; i < 8; ++i)
        {
            positive_half_plane = positive_half_plane && (dots[i] > 0.0f);
            negative_half_plane = negative_half_plane && (dots[i] < 0.0f);
        }

        if (positive_half_plane)
        {   
            return plane_aabb_intersection::outside;
        }

        if (negative_half_plane)
        {
            return plane_aabb_intersection::inside;
        }

        return plane_aabb_intersection::intersection;
    }

    bool outside(const std::array<float2, 8>& a, const line2d& p)
    {
        float dots[8];

        dots[0] = dot(p.m_n, a[0]) + p.m_d;
        dots[1] = dot(p.m_n, a[1]) + p.m_d;
        dots[2] = dot(p.m_n, a[2]) + p.m_d;
        dots[3] = dot(p.m_n, a[3]) + p.m_d;

        dots[4] = dot(p.m_n, a[4]) + p.m_d;
        dots[5] = dot(p.m_n, a[5]) + p.m_d;
        dots[6] = dot(p.m_n, a[6]) + p.m_d;
        dots[7] = dot(p.m_n, a[7]) + p.m_d;

        bool positive_half_plane = true;
        bool negative_half_plane = true;

        for (auto i = 0U; i < 8; ++i)
        {
            positive_half_plane = positive_half_plane && (dots[i] > 0.0f);
        }

        return positive_half_plane;
    }

    template <uint32_t axis> float2 project_point(const float3& p)
    {
        switch (axis)
        {
            case 0: return  {p.m_y, p.m_z };
            case 1: return  {p.m_x, p.m_z };
            case 2: return  {p.m_x, p.m_y };
            default: __assume(false);
        }
    }

    template <uint32_t axis> 
    edge2d project_edge(const edge3d& e)
    {
        return  { project_point<axis>(e.m_a), project_point<axis>(e.m_b) };
    }

    template <uint32_t axis>
    std::array<edge2d, 12> project_edges(const std::array<edge3d, 12>& e)
    {
        std::array<edge2d, 12> r;

        for (auto i = 0U; i < 12; ++i)
        {
            r[i] = project_edge<axis>(e[i]);
        }

        return r;
    }

    template <uint32_t axis>
    std::array<float2, 8> project_points(const std::array<float3, 8>& p)
    {
        std::array<float2, 8> r;

        for (auto i = 0U; i < 8; ++i)
        {
            r[i] = project_point<axis>(p[i]);
        }

        return r;
    }

    bool intersect_segment_plane(const float3& a, const float3& b, const plane& p, float3& r)
    {
        float3 ab   = b - a;
        float  t    = (-p.m_d - dot(p.m_n, a)) / dot(p.m_n, ab);

        if (t > 0.0f && t < 1.0f)
        {
            r = a + t * ab;
            return true;
        }
        else
        {
            return false;
        }
    }

    std::vector< float3 > intersection( const frustum& f, const aabb& b )
    {
        aabb f_abb                              = make_aabb(f);

        std::vector<float3> r;
        r.reserve(12);

        //no intersection
        if ( outside(b, f_abb) )
        {
            return r;
        }

        //all face planes
        if ( inside( b, f_abb) )
        {
            return make_points(f);
        }

        std::array<float3, 8>      points       = make_points(b);
        std::array<plane, 6>     face_planes    = make_face_planes(f);
        uint32_t r_intersections                = 0;    //contains in the bits intersected planes

        //face plane tests
        {
            uint32_t r_inside                   = 0;

            for (auto i = 0U; i < 6; ++i)
            {
                plane_aabb_intersection section = intersects(points, face_planes[i]);

                if (section == plane_aabb_intersection::outside)
                {
                    //no intersection
                    return r;
                }
                else if (section == plane_aabb_intersection::inside)
                {
                    r_inside = r_inside + 1;
                }
                else
                {
                    r_intersections |= (1 << i); // remember the intersection
                }
            }

            //all face planes
            if (r_inside == 6)
            {
                return make_points(f);
            }
        }

        //edge-lines
        {
            std::array<edge3d, 12>    edge_lines  = make_edges(f);

            std::array<edge2d, 12>    edges_x     = project_edges<0>(edge_lines);
            std::array<edge2d, 12>    edges_y     = project_edges<1>(edge_lines);
            std::array<edge2d, 12>    edges_z     = project_edges<2>(edge_lines);

            std::array<float2, 8>   points_x      = project_points<0>(points);
            std::array<float2, 8>   points_y      = project_points<1>(points);
            std::array<float2, 8>   points_z      = project_points<2>(points);

            std::array<line2d, 12>  lines_x       = make_lines(edges_x);
            std::array<line2d, 12>  lines_y       = make_lines(edges_y);
            std::array<line2d, 12>  lines_z       = make_lines(edges_z);

            for (auto i = 0U; i < 12; ++i)
            {
                if (outside(points_x, lines_x[i]))
                {
                    return r;
                }
            }

            for (auto i = 0U; i < 12; ++i)
            {
                if (outside(points_y, lines_y[i]))
                {
                    return r;
                }
            }

            for (auto i = 0U; i < 12; ++i)
            {
                if (outside(points_y, lines_y[i]))
                {
                    return r;
                }
            }
        }

        //test all intersected planes against the aabb edges
        std::array<edge3d, 12>    edge_lines = make_edges(&points[0]);

        std::unordered_set<float3> s;

        for (auto i = 0U; i < 6; ++i)
        {
            if ((r_intersections & (1 << i)) != 0)
            {
                for (auto e = 0; e < 12; ++e)
                {
                    float3 point;

                    if (intersect_segment_plane(edge_lines[e].m_a, edge_lines[e].m_b, face_planes[i], point))
                    {
                        s.insert(point);
                    }
                }
            }
        }

        for (auto&& s0 : s)
        {
            r.push_back(s0);
        }

        return r;
    }

    namespace
    {
        bool any(const float3& a)
        {
            return (a.m_x != 0.0f) || (a.m_y != 0.0f) || (a.m_z != 0.0f);
        }

        bool all(const float3& a)
        {
            return (a.m_x != 0.0f) && (a.m_y != 0.0f) && (a.m_z != 0.0f);
        }

        float3 greater_than_equal(const float3& a, const float3& b)
        {
            float3 r;

            r.m_x = (a.m_x >= b.m_x) ? 1.0f : 0.0f;
            r.m_y = (a.m_y >= b.m_y) ? 1.0f : 0.0f;
            r.m_z = (a.m_z >= b.m_z) ? 1.0f : 0.0f;
            return r;
        }

        float3 mix(const float3& a, const float3& b, float w)
        {
            return a * (1 - w) + (w) * b;
        }
    }

    int clip3(const float3& n, float3& v0, float3& v1, float3& v2, float3& v3)
    {
        float3 dist                 = { dot(v0, n), dot(v1, n), dot(v2, n) };
        const float3 clipEpsilon    = { 0.00001f, 0.00001f, 0.00001f };
        const float3 clipEpsilon2   = { 0.01f, 0.01f, 0.01f };
        const float3 zero           = { 0.0f, 0.0f, 0.0f };

        if (!any(greater_than_equal(dist, clipEpsilon2)))
        {
            // Case 1 ( all clipped )
            return 0;
        }

        if (all(greater_than_equal(dist, -1.0f * clipEpsilon)))
        {
            // Case 2 ( none clipped )
            v3 = v0;
            return 3;
        }

        // There are either 1 or 2 vertices above the clipping plane .
        float3 above = greater_than_equal(dist, zero);
        bool nextIsAbove;

        // Find the CCW - most vertex above the plane .
        if (above.index<1>() !=0.0f && !( above.index<0>()!=0.0f ) )
        {
            // Cycle once CCW . Use v3 as a temp
            nextIsAbove = above.index<2>();
            v3 = v0; v0 = v1; v1 = v2; v2 = v3;
            dist = dist.yzx();
        }

        else if (above.index<2>() != 0.0f && !(above.index<1>()!=0.0f))
        {
            // Cycle once CW . Use v3 as a temp .
            nextIsAbove = above.index<0>();
            v3 = v2; v2 = v1; v1 = v0; v0 = v3;
            dist = dist.zxy();
        }
        else
        {
            nextIsAbove = above.index<1>();
        }

        // We always need to clip v2 - v0 .
        v3 = mix(v0, v2, dist.index<0>() / (dist.index<0>() - dist.index<2>()));
        if (nextIsAbove)
        {
            // Case 3
            v2 = mix(v1, v2, dist.index<1>() / (dist.index<1>() - dist.index<2>()));
            return 4;
        }
        else
        {
            // Case 4
            v1 = mix(v0, v1, dist.index<0>() / (dist.index<0>() - dist.index<1>()));
            v2 = v3; v3 = v0;
            return 3;
        }
    }

    std::vector< float3 > clip1(const frustum& f, const aabb& b)
    {
        std::array<triangle_indexed, 12> indices    = make_aabb_triangle_indices();
        std::array<plane, 6>             planes     = make_face_planes(f);
        std::array<float3, 8>            points     = make_points(b);

        std::vector<float3>              r;

        r.reserve(24);

        std::unordered_set<float3> s;

        for (auto i = 0U; i < 12; ++i)
        {
            float3 va = points[indices[i].m_a];
            float3 vb = points[indices[i].m_b];
            float3 vc = points[indices[i].m_c];

            for (auto j = 0U; j < 6; ++j)
            {
                float3 d    = { planes[j].m_d, planes[j].m_d, planes[j].m_d };
                float3 v0   = va + d;
                float3 v1   = vb + d;
                float3 v2   = vc + d;
                float3 v3   = { 0,0,0 };

                int32_t clipped   = clip3(-1.0f * planes[j].m_n, v0, v1, v2, v3);

                if (clipped > 0)
                {
                    v0 = v0 - d;
                    v1 = v1 - d;
                    v2 = v2 - d;
                    v3 = v3 - d;

                    s.insert(v0);
                    r.push_back(v0);

                    if (clipped > 1)
                    {
                        s.insert(v1);
                        r.push_back(v1);
                    }

                    if (clipped > 2)
                    {
                        s.insert(v2);
                        r.push_back(v2);
                    }

                    if (clipped > 3)
                    {
                        s.insert(v3);
                        r.push_back(v3);
                    }
                }
            }
        }
        r.clear();
        for (auto&& s0 : s)
        {
            bool inside = true;

            for (auto j = 0U; j < 6; ++j)
            {
                float d = dot(planes[j].m_n, s0) + planes[j].m_d;

                inside = inside && (d <= 0.0f);
            }

            if (inside)
            {
                r.push_back(s0);
            }
        }

        return r;
    }

    struct closed_convex_clipper
    {
        struct vertex
        {
            float3 m_point;
            float  m_distance       = 0.0f;
            int    m_occurs         = 0;
            bool   m_visible        = true;
        };

        struct edge
        {
            std::vector<int32_t  >  m_faces;
            std::array<int32_t, 2>  m_vertices;
            bool                    m_visible   = true;
        };

        struct face
        {
            std::vector<int32_t>    m_edges;
            bool    m_visible       = true;

            plane                   m_plane;
        };

        std::vector<vertex>         m_vertices;
        std::vector<edge>           m_edges;
        std::vector<face>           m_faces;

        std::tuple<int32_t, int32_t> process_vertices(const plane& p)
        {
            int32_t positive = 0;
            int32_t negative = 0;

            for (auto&& v : m_vertices)
            {
                const float epsilon = 0.00001f;

                if (v.m_visible)
                {
                    v.m_distance = dot(p.m_n, v.m_point) + p.m_d;

                    if (v.m_distance >= epsilon)
                    {
                        positive = positive + 1;
                    }
                    else if (v.m_distance <= -epsilon)
                    {
                        negative = negative + 1;
                        v.m_visible = false;
                    }
                    else
                    {
                        v.m_distance = 0.0f;
                    }
                }
            }

            return std::make_tuple(negative, positive);
        }

        void process_edges()
        {
            auto edges_to_process = m_edges.size();

            for (auto i = 0U; i < edges_to_process;++i)
            {
                auto&& e = m_edges[i];

                if (e.m_visible)
                {
                    const auto& v0 = m_vertices[e.m_vertices[0]];
                    const auto& v1 = m_vertices[e.m_vertices[0]];

                    float d0 = v0.m_distance;
                    float d1 = v1.m_distance;

                    if (d0 <= 0.0f && d1 <= 0.0f)
                    {
                        //edge is culled, remove edge from faces sharing it

                        for (auto&& fi : e.m_faces)
                        {
                            auto&& f = m_faces[fi];

                            std::remove_if(f.m_edges.begin(), f.m_edges.end(), [i](int32_t e0)
                            {
                                return i == e0;
                            });

                            if (f.m_edges.empty())
                            {
                                f.m_visible = false;
                            }
                        }

                        e.m_visible = false;
                    }

                    if (d0 >= 0.0f && d1 >= 0.0f)
                    {
                        //edge is on the non negative site, retain the edge
                        continue;
                    }

                    //the edge is splite by the plane in two. compute the point of intersection
                    //if the old edge is <v0,v1> and I is the intersection point, the new edge is
                    //is <v0, I>  when d0>0 or <I, v1> when d1 > 0

                    float t = d0 / (d0 - d1);
                    float3 point = (1.0f - t) * v0.m_point + t * v1.m_point;

                    vertex v;
                    v.m_point = point;
                    m_vertices.push_back(v);

                    if (d0 > 0.0f)
                    {
                        e.m_vertices[1] = static_cast<int32_t>(m_vertices.size() - 1);
                    }
                    else
                    {
                        e.m_vertices[0] = static_cast<int32_t>(m_vertices.size() - 1);
                    }
                }
            }
        }

        std::tuple<bool, int32_t, int32_t> get_open_polyline(const face& f)
        {
            //count the number of occurences of each vertex in the polyline
            //the resulting 'occurs' values must be between 1 and 2
            for (auto e : f.m_edges)
            {
                m_vertices[m_edges[e].m_vertices[0]].m_occurs++;
                m_vertices[m_edges[e].m_vertices[1]].m_occurs++;
            }

            //determine if the polyline is open
            int32_t start   = -1;
            int32_t end     = -1;


            for (auto e : f.m_edges)
            {
                int32_t i0 = m_edges[e].m_vertices[0];
                int32_t i1 = m_edges[e].m_vertices[1];

                if (m_vertices[i0].m_occurs == 1)
                {
                    if (start == -1)
                    {
                        start = i0;
                    }
                    else if ( end == -1)
                    {
                        end = i0;
                    }
                }

                if (m_vertices[i1].m_occurs == 1)
                {
                    if (start == -1)
                    {
                        start = i1;
                    }
                    else if (end == -1)
                    {
                        end = i1;
                    }
                }

                return std::make_tuple(start != -1, start, end);
            }
        }

        void process_faces()
        {
            auto faces_to_process = m_faces.size();

            for (auto i =0U; i < faces_to_process;++i)
            {
                auto& f = m_faces[i];
                if (f.m_visible)
                {
                    // the edge is culled. if the edge is exactly on the clip plane,
                    // it is possible that a visible triangle shares it.
                    //the edge will be added during the face loop

                    for (auto e : f.m_edges)
                    {
                        m_vertices[m_edges[e].m_vertices[0]].m_occurs = 0;
                        m_vertices[m_edges[e].m_vertices[1]].m_occurs = 0;
                    }

                    auto t          = get_open_polyline(f);
                    bool is_open    = std::get<0>(t);
                    int32_t start   = std::get<1>(t);
                    int32_t end     = std::get<2>(t);

                    //polyline is open, close it
                    if (is_open)
                    {
                        edge e;

                        e.m_faces.push_back(i);
                        e.m_vertices[0] = start;
                        e.m_vertices[1] = end;

                        m_edges.push_back(e);

                        auto index = static_cast<int32_t>(m_edges.size() - 1);
                        f.m_edges.push_back(index);
                    }
                }
            }
        }

        float3 get_normal(const std::vector<int32_t> vi)
        {
            float3 normal;
            auto   vi_to_process = vi.size();

            for (auto i = 0U; i < vi_to_process-1; ++i)
            {
                normal = normal + cross(m_vertices[i].m_point, m_vertices[i + 1].m_point);
            }

            return normalize(normal);
        }

        std::vector<int32_t> get_ordered_vertices(int32_t fi)
        {
            //copy edge indices into fixed continious memory for sorting
            std::vector<int32_t> edges = m_faces[fi].m_edges;


            //bubble sort to arrange edge in continious order

            int32_t i0 = 0;
            int32_t i1 = 1;
            int32_t choice = 1;

            for ( i0 = 0, i1 = 1, choice = 1; i1 < edges.size() - 1; i0 = i1, i1++ ) 
            {
                int32_t current = m_edges[i0].m_vertices[choice];

                for (auto j = i1; j < edges.size(); ++j)
                {
                    if (m_edges[edges[j]].m_vertices[0] == current)
                    {
                        std::swap( edges[i1], edges[j] );
                        choice = 1;
                    }

                    if (m_edges[edges[j]].m_vertices[1] == current)
                    {
                        std::swap(edges[i1], edges[j]);
                        choice = 0;
                    }
                }
            }

            std::vector<int32_t> r;
            r.resize(edges.size() + 1);

            //add the first two vertices
            r[0] = m_edges[edges[0]].m_vertices[0];
            r[1] = m_edges[edges[0]].m_vertices[1];

            //add the rest
            for ( auto i = 1U; i < edges.size(); ++i )
            {
                if (m_edges[i].m_vertices[0] == r[i])
                {
                    r[i + 1] = m_edges[i].m_vertices[1];
                }
                else
                {
                    r[i + 1] = m_edges[i].m_vertices[0];
                }
            }

            return r;
        }

        std::vector<int32_t> get_ordered_faces()
        {
            std::vector<int32_t> r;
            auto faces_to_process = m_faces.size();

            for (auto i = 0U; i < faces_to_process; ++i)
            {
                const auto& f = m_faces[i];
                if (f.m_visible)
                {

                    //get the ordered vertices for a face. the first and the last
                    //element of the array are the same since the polyline is closed
                    auto vertices = get_ordered_vertices(i);

                    //push back the size of the elements
                    r.push_back(static_cast<int32_t>(vertices.size() - 1));

                    //the convention is that the vertices should be counterclockwise
                    //ordered when viewed from the negative side of the plane of the face.
                    //if you need the opposite convention, switch the inequality
                    //in the if else statement

                    if (dot(m_faces[i].m_plane.m_n, get_normal(vertices)) > 0.0f)
                    {
                        //clockwise
                        for (auto j = vertices.size() - 2; j >= 0; j--)
                        {
                            r.push_back(vertices[j]);
                        }
                    }
                    else
                    {
                        //counterclockwise
                        for (auto j = 0; i <= vertices.size() - 2; j++)
                        {
                            r.push_back(vertices[j]);
                        }
                    }
                }

                return r;
            }
        }

        int32_t     clip(const plane& p)
        {
            //vertex processing
            {
                auto v        = process_vertices(p);
                auto negative = std::get<0>(v);
                auto positive = std::get<1>(v);

                if (negative == 0)
                {
                    return +1; //no clipping
                }

                if (positive == 0)
                {
                    return -1; //all clipped
                }
            }

            //edges processing
            process_edges();

            //faces processing
            process_faces();

            return 0;
        }

        std::tuple< std::vector<float3>, std::vector<int32_t> > convert()
        {
            std::vector<float3>     point;
            std::vector<int32_t>    vmap(m_vertices.size(), -1);

            //copy the visible attributes into the table
            for (auto i = 0U; i < m_vertices.size(); ++i)
            {
                if (m_vertices[i].m_visible)
                {
                    vmap[i] = static_cast<int32_t>( point.size() );
                    point.push_back(m_vertices[i].m_point);
                }
            }

            //order the vertices for all the faces. the output array has a
            //sequence of subarrays, each subarray having first element sorting
            //the number of vertices in the face, the remaining elements storing
            //the vertex indices for that face in the correct order. The indices
            //are relative to the m_vertices vector

            std::vector<int32_t> faces = get_ordered_faces();

            //map the vertex indices to those of the new table

            for (auto i = 0U; i < faces.size(); ++i)
            {
                int32_t index_count = faces[i];
                i = i + 1;

                for (auto j = 0U; j < index_count; ++j)
                {
                    faces[i] = vmap[faces[i]];

                    assert(faces[i] != -1);
                    i++;
                }
            }

            return make_tuple(std::move(point), std::move(faces));
        }
    };

    closed_convex_clipper make_clipper(const frustum& f)
    {
        return closed_convex_clipper();
    }

    closed_convex_clipper make_clipper(const aabb& b)
    {
        return closed_convex_clipper();
    }

    std::vector< float3 > clip(const frustum& f, const aabb& b)
    {
        std::array<triangle_indexed, 12> indices    = make_aabb_triangle_indices();
        std::array<plane, 6>             planes     = make_face_planes(f);
        std::array<float3, 8>            points     = make_points(b);

        std::vector<float3>              r;

        r.reserve(24);

        plane p = planes[frustum_planes::Far];

        for ( auto i = 0; i < 8; ++i )
        {

        }

        return r;
    }
}
