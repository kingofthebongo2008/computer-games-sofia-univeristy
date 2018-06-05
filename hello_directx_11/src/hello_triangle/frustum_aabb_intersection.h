#pragma once

#include <intrin.h>
#include <cstdint>
#include <vector>
#include <array>

namespace computational_geometry
{
    struct float3
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;



        //syntax sugar
        float3 yzx() const
        {
            return { m_y, m_z, m_x };
        }

        float3 zxy() const
        {
            return { m_z, m_x, m_y };
        }

        template <uint32 i> float index() const
        {
            switch (i)
            {
                case 0: return m_x;
                case 1: return m_y;
                case 2: return m_z;
                default:
                return 0.0f;
            }
        }
    };

    struct float2
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
    };

    /////////////////////
    inline bool operator==(const float3& a, const float3& b)
    {
        return ((a.m_x == b.m_x) && (a.m_y == b.m_y) && (a.m_z == b.m_z) );
    }

    inline float3 operator+(const float3& a, const float3& b)
    {
        return { a.m_x + b.m_x, a.m_y + b.m_y, a.m_z + b.m_z };
    }

    inline float3 operator-(const float3& a, const float3& b)
    {
        return { a.m_x - b.m_x, a.m_y - b.m_y, a.m_z - b.m_z };
    }

    inline float3 operator*(const float3& a, float s)
    {
        return { a.m_x * s, a.m_y * s , a.m_z * s };
    }

    inline float3 operator*(float s, const float3& a)
    {
        return { a.m_x * s, a.m_y * s , a.m_z * s };
    }

    inline float3 operator/(const float3& a, float s)
    {
        return { a.m_x / s, a.m_y / s , a.m_z / s };
    }

    inline float3 operator/(float s, const float3& a)
    {
        return {  s / a.m_x, s / a.m_y, s / a.m_z };
    }

    inline float dot(const float3& a, const float3& b)
    {
        return a.m_x * b.m_x + a.m_y * b.m_y + a.m_z * b.m_z;
    }

    inline float3 normalize(const float3& a)
    {
        return a / sqrtf(dot(a, a));
    }


    /////////////////////
    inline float2 operator+(const float2& a, const float2& b)
    {
        return { a.m_x + b.m_x, a.m_y + b.m_y};
    }

    inline float2 operator-(const float2& a, const float2& b)
    {
        return { a.m_x - b.m_x, a.m_y - b.m_y };
    }

    inline float2 operator*(const float2& a, float s)
    {
        return { a.m_x * s, a.m_y * s};
    }

    inline float2 operator*(float s, const float2& a)
    {
        return { a.m_x * s, a.m_y * s};
    }

    inline float2 operator/(const float2& a, float s)
    {
        return { a.m_x / s, a.m_y / s };
    }

    inline float2 operator/(float s, const float2& a)
    {
        return { s / a.m_x, s / a.m_y };
    }

    inline float dot(const float2& a, const float2& b)
    {
        return a.m_x * b.m_x + a.m_y * b.m_y;
    }

    inline float2 normalize(const float2& a)
    {
        return a / sqrtf(dot(a, a));
    }

    ///////////
    inline float3 cross(const float3& a, const float3& b)
    {
        float u1 = a.m_x;
        float u2 = a.m_y;
        float u3 = a.m_z;

        float v1 = b.m_x;
        float v2 = b.m_y;
        float v3 = b.m_z;

        float x = (u2 * v3) - (u3* v2);
        float y = (u3 * v1) - (u1* v3);
        float z = (u1 * v2) - (u2* v1);

        return { x,y,z };
    }

    enum frustum_planes : uint32_t
    {
        Left    = 0,
        Right   = 1,
        Top     = 2,
        Bottom  = 3,
        Near    = 4,
        Far     = 5
    };

    enum frustum_points : uint32_t
    {
        NearBottomLeft     = 0,
        NearBottomRight    = 1,
        NearTopRight       = 2,
        NearTopLeft        = 3,

        FarBottomLeft       = 4,
        FarBottomRight      = 5,
        FarTopRight         = 6,
        FarTopLeft          = 7
    };

    struct frustum
    {
        float3 m_points[8];
    };

    struct aabb
    {
        float3 m_min;
        float3 m_max;
    };

    struct plane
    {
        float3 m_n;
        float  m_d = 0.0f;
    };

    inline plane make_plane(const float3& a, const float3& b, const float3& c)
    {
        float3 u = b - a;
        float3 v = c - a;
        float3 n = normalize(cross(u, v)); //normal points inside
        auto     d = -dot(n, a); //note: if the plane equation is ax+by+cz+d, then this is negative, else positive
        return  { n, d };
    }


    inline std::vector<float3> make_points(const frustum& f)
    {
        std::vector<float3> r;
        r.reserve(8);
        for (auto i = 0U; i < 8; ++i)
        {
            r.push_back(f.m_points[i]);
        }
        return r;
    }

    inline std::array<float3, 8> make_points(const aabb& f)
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

    inline std::array< plane, 6 > make_face_planes(const frustum& f)
    {
        std::array< plane, 6 > r;

        //Consistency check, these planes should be like the other ones
        plane  near0 = make_plane(f.m_points[frustum_points::NearTopRight], f.m_points[frustum_points::NearBottomLeft], f.m_points[frustum_points::NearBottomRight]);
        plane  far0 = make_plane(f.m_points[frustum_points::FarTopRight], f.m_points[frustum_points::FarBottomRight], f.m_points[frustum_points::FarBottomLeft]);
        plane  left0 = make_plane(f.m_points[frustum_points::FarTopLeft], f.m_points[frustum_points::NearBottomLeft], f.m_points[frustum_points::NearTopLeft]);
        plane  right0 = make_plane(f.m_points[frustum_points::FarTopRight], f.m_points[frustum_points::NearBottomRight], f.m_points[frustum_points::FarBottomRight]);
        plane  top0 = make_plane(f.m_points[frustum_points::FarTopLeft], f.m_points[frustum_points::NearTopRight], f.m_points[frustum_points::FarTopRight]);
        plane  bottom0 = make_plane(f.m_points[frustum_points::FarBottomLeft], f.m_points[frustum_points::NearBottomRight], f.m_points[frustum_points::NearBottomLeft]);

        r[frustum_planes::Left] = left0;
        r[frustum_planes::Right] = right0;
        r[frustum_planes::Top] = top0;

        r[frustum_planes::Bottom] = bottom0;
        r[frustum_planes::Near] = near0;
        r[frustum_planes::Far] = far0;

        return r;
    }

    inline std::array< plane, 6 > make_face_planes(const aabb& f)
    {
        std::array< plane, 6 > r;
        std::array< float3, 8> points = make_points(f);

        //Consistency check, these planes should be like the other ones
        plane  near0 = make_plane(points[frustum_points::NearTopRight], points[frustum_points::NearBottomLeft], points[frustum_points::NearBottomRight]);
        plane  far0 = make_plane(points[frustum_points::FarTopRight], points[frustum_points::FarBottomRight], points[frustum_points::FarBottomLeft]);
        plane  left0 = make_plane(points[frustum_points::FarTopLeft], points[frustum_points::NearBottomLeft], points[frustum_points::NearTopLeft]);
        plane  right0 = make_plane(points[frustum_points::FarTopRight], points[frustum_points::NearBottomRight], points[frustum_points::FarBottomRight]);
        plane  top0 = make_plane(points[frustum_points::FarTopLeft], points[frustum_points::NearTopRight], points[frustum_points::FarTopRight]);
        plane  bottom0 = make_plane(points[frustum_points::FarBottomLeft], points[frustum_points::NearBottomRight], points[frustum_points::NearBottomLeft]);

        r[frustum_planes::Left] = left0;
        r[frustum_planes::Right] = right0;
        r[frustum_planes::Top] = top0;

        r[frustum_planes::Bottom] = bottom0;
        r[frustum_planes::Near] = near0;
        r[frustum_planes::Far] = far0;

        return r;
    }

    std::vector< float3 > intersection(const frustum& f, const aabb& b);
    std::vector< float3 > clip(const frustum& f, const aabb& b);


}
