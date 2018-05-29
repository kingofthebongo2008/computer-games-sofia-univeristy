#include "pch.h"
#include "frustum_aabb_intersection.h"
#include <algorithm>
#include <array>

namespace computational_geometry
{
    struct plane
    {
        float3 m_n;
        float  m_d = 0.0f;
    };

    struct edge
    {
        float3 m_a;
        float3 m_b;
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
        auto     d  = dot(n, a);
        return  { n, d };
    }


    std::array< plane, 6 > make_face_planes(const frustum& f )
    {
        std::array< plane, 6 > r;

        //Consistency check, these planes should be like the other ones
        plane  near0    = make_plane(f.m_points[frustum_points::NearBottomLeft], f.m_points[frustum_points::NearTopRight], f.m_points[frustum_points::NearBottomRight]);
        plane  far0     = make_plane(f.m_points[frustum_points::FarBottomRight], f.m_points[frustum_points::NearTopRight], f.m_points[frustum_points::FarTopLeft]);
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
    constexpr void get_edge(uint32_t a, uint32_t b)
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

    template <uint32_t edge>
    edge make_edge(const frustum& f)
    {
        uint32_t a = 0;
        uint32_t b = 0;

        get_edge<edge>(index, a, b);
        return { f.m_points[a], f.m_points[b] };
    }

    std::array<edge, 12> make_edges(const frustum& f)
    {
        std::array<edge, 12> r;

        r[0]    = make_edge<0>(f);
        r[1]    = make_edge<1>(f);
        r[2]    = make_edge<2>(f);
        r[3]    = make_edge<3>(f);

        r[4]    = make_edge<4>(f);
        r[5]    = make_edge<5>(f);
        r[6]    = make_edge<6>(f);
        r[7]    = make_edge<7>(f);

        r[8]    = make_edge<8>(f);
        r[9]    = make_edge<9>(f);
        r[10]   = make_edge<10>(f);
        r[11]   = make_edge<11>(f);

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
        r[1] = { f.m_min.m_x, f.m_min.m_y, f.m_max.m_z };
        r[2] = { f.m_min.m_x, f.m_max.m_y, f.m_min.m_z };
        r[3] = { f.m_min.m_x, f.m_max.m_y, f.m_max.m_z };

        r[4] = { f.m_max.m_x, f.m_min.m_y, f.m_min.m_z };
        r[5] = { f.m_max.m_x, f.m_min.m_y, f.m_max.m_z };
        r[6] = { f.m_max.m_x, f.m_max.m_y, f.m_min.m_z };
        r[7] = { f.m_max.m_x, f.m_max.m_y, f.m_max.m_z };

        return r;
    }

    enum class plane_aabb_intersection : uint32_t 
    {
        inside      = 0,
        outside     = 1,
        intersects  = 2
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
            negative_half_plane = negative_half_plane && (dots[i] > 0.0f);
        }

        if (positive_half_plane)
        {   
            return plane_aabb_intersection::outside;
        }

        if (negative_half_plane)
        {
            return plane_aabb_intersection::inside;
        }

        return plane_aabb_intersection::intersects;
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
        uint32_t r_intersections = 0;

        {
            uint32_t r_inside                   = 0;

            //face plane tests
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
            std::array<edge, 12>    edge_lines = make_edges(f);

            


        }
        return r;
    }
}
