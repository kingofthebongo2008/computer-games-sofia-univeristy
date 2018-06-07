#include "pch.h"
#include "frustum_aabb_intersection.h"
#include <algorithm>
#include <array>
#include <unordered_set>
#include <functional>
#include <assert.h>
#include <iostream>     // std::cout, std::ios
#include <sstream>      // std::ostringstream



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

    template <uint32_t edge>
    constexpr void get_edge(uint32_t& a, uint32_t& b)
    {
        static_assert(edge < 12);

        switch (edge)
        {
            case 0: a = 0; b = 1; break;    //
            case 1: a = 0; b = 4; break;    //
            case 2: a = 3; b = 0; break;    //

            case 3: a = 7; b = 6; break;    //  
            case 4: a = 6; b = 2; break;    //  
            case 5: a = 5; b = 6; break;    //

            case 6: a = 5; b = 4; break;    //
            case 7: a = 1; b = 5; break;    //

            case 8: a = 2; b = 3; break;    //
            case 9: a = 1; b = 2; break;    // 

            case 10: a = 3; b = 7; break;
            case 11: a = 4; b = 7; break;
        }
    }

    template<uint32_t a, uint32_t b>
    constexpr uint32_t make_tuple_edge()
    {
        return a | (b << 3);
    }

    //template <uint32_t a, uint32_t b>
    constexpr uint32_t get_edge_0(uint32_t a, uint32_t b)
    {
      //  static_assert(a < 8);
        //static_assert(b < 8);
        //static_assert(a != b;)

        auto t = a | (b << 3);

        switch(t)
        {
            case make_tuple_edge<0, 1>(): return 0;

            case make_tuple_edge<0, 4>(): return 1;// : a = 0; b = 4; break;    //
            case make_tuple_edge<3, 0>(): return 2;// : a = 3; b = 0; break;    //

            case make_tuple_edge<7, 6>(): return 3;// : a = 7; b = 6; break;    //  
            case make_tuple_edge<6, 2>(): return 4;// : a = 6; b = 2; break;    //  
            case make_tuple_edge<5, 6>(): return 5;// : a = 5; b = 6; break;    //

            case make_tuple_edge<5, 4>(): return 6;// : a = 5; b = 4; break;    //
            case make_tuple_edge<1, 5>(): return 7;// : a = 1; b = 5; break;    //

            case make_tuple_edge<2, 3>(): return 8;// : a = 2; b = 3; break;    //
            case make_tuple_edge<1, 2>(): return 9;// : a = 1; b = 2; break;    // 

            case make_tuple_edge<3, 7>(): return 10;// : a = 3; b = 7; break;
            case make_tuple_edge<4, 7>(): return 11;// : a = 4; b = 7; break;

            default: __assume(false); return 12;
        }
    }

    template <uint32_t plane> uint32_t get_point_0()
    {
        static_assert(plane < 6);
        
        switch (plane)
        {
            case 0: return 0;
            case 1: return 1;
            case 2: return 2;
            case 3: return 0;
            case 4: return 0;
            case 5: return 4;
        }
    }

    template <uint32_t plane> uint32_t get_point_1()
    {
        static_assert(plane < 6);

        switch (plane)
        {
        case 0: return 3;
        case 1: return 5;
        case 2: return 6;
        case 3: return 4;
        case 4: return 1;
        case 5: return 7;
        }
    }

    template <uint32_t plane> uint32_t get_point_2()
    {
        static_assert(plane < 6);

        switch (plane)
        {
        case 0: return 7;
        case 1: return 6;
        case 2: return 7;
        case 3: return 5;
        case 4: return 2;
        case 5: return 6;
        }
    }

    template <uint32_t plane> uint32_t get_point_3()
    {
        static_assert(plane < 6);

        switch (plane)
        {
        case 0: return 4;
        case 1: return 2;
        case 2: return 3;
        case 3: return 1;
        case 4: return 3;
        case 5: return 5;
        }
    }



    template <uint32_t edge>
    constexpr uint32_t get_left_face()
    {
        static_assert(edge < 12);
        switch (edge)
        {
        case 0: return 4;
        case 1: return 3;
        case 2: return 4;

        case 3: return 2;
        case 4: return 2;
        case 5: return 5;

        case 6: return 5;
        case 7: return 1;

        case 8: return 4;
        case 9: return 4;

        case 10: return 0;
        case 11: return 5;

        default: __assume(false); return 0;
        }
    }

    
    template <uint32_t edge>
    constexpr uint32_t get_right_face()
    {
        static_assert(edge < 12);
        switch (edge)
        {
            case 0: return 3;
            case 1: return 0;
            case 2: return 0;

            case 3: return 5;
            case 4: return 1;
            case 5: return 1;

            case 6: return 3;
            case 7: return 3;

            case 8: return 2;
            case 9: return 1;

            case 10: return 2;
            case 11: return 0;
            default: __assume(false); return 0;
        }
    }

    template <uint32_t plane>
    constexpr uint32_t get_edge_0()
    {
        static_assert(plane < 6);
        switch (plane)
        {
            case 0: return 1;
            case 1: return 4;
            case 2: return 8;

            case 3: return 7;
            case 4: return 8;
            case 5: return 5;
            default: __assume(false); return 0;
        }
    }

    template <uint32_t plane>
    constexpr uint32_t get_edge_1()
    {
        static_assert(plane < 6);
        switch (plane)
        {
            case 0: return 2;
            case 1: return 9;
            case 2: return 4;

            case 3: return 0;
            case 4: return 0;
            case 5: return 6;
            default: __assume(false); return 0;
        }
    }

    template <uint32_t plane>
    constexpr uint32_t get_edge_2()
    {
        static_assert(plane < 6);
        switch (plane)
        {
        case 0: return 11;
        case 1: return 5;
        case 2: return 3;

        case 3: return 1;
        case 4: return 2;
        case 5: return 3;
        default: __assume(false); return 0;
        }
    }

    template <uint32_t plane>
    constexpr uint32_t get_edge_3()
    {
        static_assert(plane < 6);
        switch (plane)
        {
            case 0: return 10;
            case 1: return 7;
            case 2: return 10;

            case 3: return 6;
            case 4: return 9;
            case 5: return 11;

            default: __assume(false); return 0;
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
}
