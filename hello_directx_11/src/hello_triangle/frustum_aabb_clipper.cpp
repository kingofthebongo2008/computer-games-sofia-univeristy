#include "pch.h"
#include "frustum_aabb_intersection.h"
#include <algorithm>
#include <array>
#include <vector>

namespace computational_geometry
{
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
            std::array<int32_t, 2>  m_vertices = { -1, -1 };
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
                    const auto& v1 = m_vertices[e.m_vertices[1]];

                    float d0 = v0.m_distance;
                    float d1 = v1.m_distance;

                    if (d0 <= 0.0f && d1 <= 0.0f)
                    {
                        //edge is culled, remove edge from faces sharing it

                        for (auto&& fi : e.m_faces)
                        {
                            auto&& f = m_faces[fi];

                            f.m_edges.erase(std::remove_if(f.m_edges.begin(), f.m_edges.end(), [i](int32_t e0)
                            {
                                return i == e0;
                            }));

                            if (f.m_edges.empty())
                            {
                                f.m_visible = false;
                            }
                        }

                        e.m_visible = false;
                        continue;
                    }

                    if (d0 >= 0.0f && d1 >= 0.0f)
                    {
                        //edge is on the non negative site, retain the edge
                        continue;
                    }

                    //the edge is split by the plane in two. compute the point of intersection
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
            //count the number of occurrences of each vertex in the polygon line
            //the resulting 'occurs' values must be between 1 and 2

            for (auto&& e : f.m_edges)
            {
                m_vertices[m_edges[e].m_vertices[0]].m_occurs++;
                m_vertices[m_edges[e].m_vertices[1]].m_occurs++;
            }

            //determine if the polygon line is open
            int32_t start   = -1;
            int32_t end     = -1;


            for (auto&& e : f.m_edges)
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
            }

            return std::make_tuple(start != -1, start, end);
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

                    for (auto&& e : f.m_edges)
                    {
                        m_vertices[m_edges[e].m_vertices[0]].m_occurs = 0;
                        m_vertices[m_edges[e].m_vertices[1]].m_occurs = 0;
                    }

                    auto t          = get_open_polyline(f);
                    bool is_open    = std::get<0>(t);
                    int32_t start   = std::get<1>(t);
                    int32_t end     = std::get<2>(t);

                    //polygon line is open, close it
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
            //copy edge indices into fixed continuous memory for sorting
            std::vector<int32_t> edges = m_faces[fi].m_edges;

            //bubble sort to arrange edge in continuous order
            int32_t i0 = 0;
            int32_t i1 = 1;
            int32_t choice = 1;

            for ( i0 = 0, i1 = 1, choice = 1; i1 < edges.size() - 1; ) 
            {
                int32_t current = m_edges[ edges[i0] ].m_vertices[choice];

                for (auto j = i1; j < edges.size(); ++j)
                {
                    if (m_edges[edges[j]].m_vertices[0] == current)
                    {
                        std::swap( edges[i1], edges[j] );
                        choice = 1;
                        break;
                    }

                    if (m_edges[edges[j]].m_vertices[1] == current)
                    {
                        std::swap(edges[i1], edges[j]);
                        choice = 0;
                        break;
                    }
                }

                i0 = i1;
                i1 = i1 + 1;
            }

            std::vector<int32_t> r;
            r.resize(edges.size() + 1);

            //add the first two vertices
            r[0] = m_edges[edges[0]].m_vertices[0];
            r[1] = m_edges[edges[0]].m_vertices[1];

            //add the rest
            for ( auto i = 1U; i < edges.size(); ++i )
            {
                if (m_edges[ edges[i] ].m_vertices[0] == r[i])
                {
                    r[i + 1] = m_edges[ edges[i] ].m_vertices[1];
                }
                else
                {
                    r[i + 1] = m_edges[ edges[i] ].m_vertices[0];
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
                    //element of the array are the same since the poly line is closed
                    auto vertices = get_ordered_vertices(i);

                    //push back the size of the elements
                    r.push_back(static_cast<int32_t>(vertices.size() - 1));

                    //the convention is that the vertices should be counterclockwise
                    //ordered when viewed from the negative side of the plane of the face.
                    //if you need the opposite convention, switch the inequality
                    //in the if else statement

                    if (dot(m_faces[i].m_plane.m_n, get_normal(vertices)) > 0.0f)
                    {
                        //counterclockwise
                        for (int32_t j = static_cast<int32_t>(vertices.size()) - 2; j >= 0; j--)
                        {
                            r.push_back(vertices[j]);
                        }
                    }
                    else
                    {
                        //clockwise
                        for (auto j = 0; j <= vertices.size() - 2; j++)
                        {
                            r.push_back(vertices[j]);
                        }
                    }
                }
            }

            return r;
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
            //sequence of sub arrays, each sub array having first element sorting
            //the number of vertices in the face, the remaining elements storing
            //the vertex indices for that face in the correct order. The indices
            //are relative to the m_vertices vector

            std::vector<int32_t> faces = get_ordered_faces();

            //map the vertex indices to those of the new table

            for (auto i = 0U; i < faces.size() ;)
            {
                int32_t index_count = faces[i];
                i = i + 1;

                for (auto j = 0; j < index_count; ++j)
                {
                    faces[i] = vmap[faces[i]];
                    i++;
                }
            }

            return make_tuple(std::move(point), std::move(faces));
        }
    };
   

    template <typename t>
    closed_convex_clipper make_clipper(const t& b)
    {
        closed_convex_clipper r;

        //todo: b is a frustum or aabb, so topology is known
        {
            const int32_t indices[6][4] =
            {
                {0,3,7,4},
                {1,5,6,2},
                {3,2,6,7},
                {4,5,1,0},
                {0,1,2,3},
                {5,4,7,6}
            };

            r.m_edges.resize(12);
            r.m_faces.resize(6);

            uint32_t edge = 0;

            for (auto i = 0; i < 6; ++i)
            {
                for (auto j = 0; j < 4; ++j)
                {
                    auto i1 = indices[i][(j + 1) % 4];
                    auto i0 = indices[i][j];
                    auto k = 0;

                    for (k = 0; k < 12; ++k)
                    {
                        auto v0 = r.m_edges[k].m_vertices[0];
                        auto v1 = r.m_edges[k].m_vertices[1];

                        if ((v0 == i1 && v1 == i0) || (v0 == i0 && v1 == i1))
                        {
                            break;
                        }
                    }

                    if (k == 12)
                    {
                        r.m_edges[edge].m_vertices[0] = i0;
                        r.m_edges[edge].m_vertices[1] = i1;
                        edge++;
                    }
                }
            }

            for (auto i = 0; i < 6; ++i)
            {
                for (auto j = 0; j < 4; ++j)
                {
                    auto i1 = indices[i][(j + 1) % 4];
                    auto i0 = indices[i][j];
                    auto k = 0;

                    for (k = 0; k < 12; ++k)
                    {
                        auto v0 = r.m_edges[k].m_vertices[0];
                        auto v1 = r.m_edges[k].m_vertices[1];

                        if ((v0 == i1 && v1 == i0) || (v0 == i0 && v1 == i1))
                        {
                            r.m_edges[k].m_faces.push_back(i);
                        }
                    }
                }
            }
        }

        for (auto i = 0; i < 12; ++i)
        {
            for (auto j = 0; j < r.m_edges[i].m_faces.size(); ++j)
            {
                r.m_faces[ r.m_edges[i].m_faces[j] ].m_edges.push_back(i);
            }
        }

       
        {
            auto planes = make_face_planes(b);

            r.m_faces[0].m_plane = planes[0];
            r.m_faces[1].m_plane = planes[1];
            r.m_faces[2].m_plane = planes[2];

            r.m_faces[3].m_plane = planes[3];
            r.m_faces[4].m_plane = planes[4];
            r.m_faces[5].m_plane = planes[5];
        }

        {
            auto points = make_points(b);

            for (auto i = 0U; i < points.size(); ++i)
            {
                closed_convex_clipper::vertex v;

                v.m_point = points[i];
                r.m_vertices.push_back(v);
            }
        }

        return r;
    }

    std::vector< float3 > clip(const frustum& f, const aabb& b)
    {
        auto clipper    = make_clipper(f);
        auto planes     = make_face_planes(b);

        for (auto i = 0; i < planes.size(); ++i)
        {
            if (clipper.clip(planes[i]) == -1)
            {
                return std::vector<float3>();
            }

        }

        auto r0 = clipper.convert();
        return std::vector<float3>();
    }
}
