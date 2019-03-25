#include "pch.h"
#include "frustum_aabb_intersection.h"
#include <algorithm>
#include <array>
#include <vector>
#include <optional>
#include <unordered_map>

namespace std
{
    template <>
    struct hash< std::tuple<int32_t, int32_t> >
    {
        size_t operator()(const std::tuple<int32_t, int32_t> & l) const
        {
            // Compute individual hash values for first, second and third
            // http://stackoverflow.com/a/1646913/126995
            size_t res = 17;
            res = res * 31 + hash<int32_t>()(std::get<0>(l));
            res = res * 31 + hash<int32_t>()(std::get<1>(l));
            return res;
        }
    };
}

namespace computational_geometry
{
    //david eberly convex clipper implementation

    struct closed_convex_clipper
    {
        //todo: split these structures per usage
        struct vertex_attributes
        {
            float  m_distance = 0.0f;
            int    m_occurs = 0;
            bool   m_visible = true;
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

        std::vector<float3>                     m_vertices_points;
        std::vector<vertex_attributes>          m_vertices;
        std::vector<edge>                       m_edges;
        std::vector<face>                       m_faces;

        std::tuple<int32_t, int32_t> process_vertices(const plane& p)
        {
            int32_t positive = 0;
            int32_t negative = 0;

            auto vertices_to_process = m_vertices.size();

            for (auto i=0U; i < vertices_to_process;++i)
            {
                auto& v         = m_vertices[i];
                auto& v_point   = m_vertices_points[i];

                const float epsilon = 0.00001f;

                if (v.m_visible)
                {
                    v.m_distance = dot(p.m_n, v_point) + p.m_d;

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

                    const auto& v0_point = m_vertices_points[e.m_vertices[0]];
                    const auto& v1_point = m_vertices_points[e.m_vertices[1]];

                    float t = d0 / (d0 - d1);
                    float3 point = (1.0f - t) * v0_point + t * v1_point;

                    m_vertices_points.push_back(point);
                    m_vertices.push_back({});

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

        void process_faces(const plane& clip_plane)
        {
            face close_face;
            close_face.m_plane = clip_plane;
            m_faces.push_back(close_face);

            auto faces_to_process       = static_cast<uint32_t>(m_faces.size());
            uint32_t close_face_index   = faces_to_process - 1;

            //the mesh straddles the plane. a new convex face will be generated
            //add it now and insert edges, when they are needed

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

                        m_edges[index].m_faces.push_back(close_face_index);
                        m_faces[close_face_index].m_edges.push_back(index);
                    }
                }
            }
        }

        float3 get_normal(const std::vector<int32_t>& vi)
        {
            float3 normal;
            auto   vi_to_process = vi.size();

            for (auto i = 0U; i < vi_to_process-1; ++i)
            {
                normal = normal + cross(m_vertices_points[i], m_vertices_points[i + 1]);
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

        int32_t     clip(const plane& clipPlane)
        {
            //vertex processing
            {
                auto v        = process_vertices(clipPlane);
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
            process_faces(clipPlane);

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
                    point.push_back(m_vertices_points[i]);
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
    closed_convex_clipper make_clipper(const t& b);

    template <>
    closed_convex_clipper make_clipper<frustum>(const frustum& b)
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
                r.m_vertices_points.push_back(points[i]);
                r.m_vertices.push_back({});
            }
        }

        return r;
    }

    template <>
    closed_convex_clipper make_clipper(const convex_polyhedron& b)
    {
        closed_convex_clipper r;

        //build edges
        {
            const auto edge_count = b.m_points.size() + b.m_faces.size() - 2;

            std::unordered_map< std::tuple<int32_t, int32_t>, int32_t > edges;

            for (auto i = 0; i < b.m_faces.size(); ++i)
            {
                const auto& f = b.m_faces[i];
                const auto indices_size = f.m_indices.size();

                for (auto p = 0U; p < indices_size; ++p)
                {
                    const   int32_t computed_index_0 = f.m_indices[p];
                    const   int32_t computed_index_1 = f.m_indices[(p + 1) % indices_size];
                    const   int32_t index_0 = std::min(computed_index_0, computed_index_1);
                    const   int32_t index_1 = std::max(computed_index_0, computed_index_1);

                    auto&&  it = edges.find(std::make_tuple(index_0, index_1));
                    if (it == edges.end())
                    {
                        closed_convex_clipper::edge e;

                        e.m_vertices[0] = index_0;
                        e.m_vertices[1] = index_1;
                        e.m_faces.push_back(i);

                        //new edge
                        r.m_edges.push_back(std::move(e));

                        edges.insert(std::make_pair(std::make_tuple(index_0, index_1), static_cast<int32_t>(r.m_edges.size()) - 1));
                    }
                    else
                    {
                        r.m_edges[it->second].m_faces.push_back(i);
                    }
                }
            }
        }

        //build faces
        {
            r.m_faces.resize(b.m_faces.size());
            for (auto i = 0; i < r.m_edges.size(); ++i)
            {
                for (auto j = 0; j < r.m_edges[i].m_faces.size(); ++j)
                {
                    r.m_faces[r.m_edges[i].m_faces[j]].m_edges.push_back(i);
                }
            }
        }

        //build points
        {
            r.m_vertices_points.resize(b.m_points.size());
            r.m_vertices.resize(b.m_points.size());

            for (auto i = 0; i < b.m_points.size();++i)
            {
                r.m_vertices_points[i] = b.m_points[i];
            }
        }

        //build planes
        {
            //compute planes
            for (auto i = 0U; i < r.m_faces.size(); ++i)
            {
                const auto&  e0         = r.m_edges[r.m_faces[i].m_edges[0]];
                const auto&  e1         = r.m_edges[r.m_faces[i].m_edges[1]];

                const float3 a          = r.m_vertices_points[ e0.m_vertices[0] ];
                const float3 b          = r.m_vertices_points[ e0.m_vertices[1] ];
                const auto   index      = e0.m_vertices[0] == e1.m_vertices[0] ? 1 : 0;
                const float3 c          = r.m_vertices_points[e1.m_vertices[index]];
                
                r.m_faces[i].m_plane    = make_plane(a, b, c);
            }

            //orient normals to point inside
            for (auto i = 0U; i < r.m_faces.size(); ++i)
            {
                const auto& plane = r.m_faces[i].m_plane;

                for (auto&& v : r.m_vertices_points)
                {
                    if (dot(plane.m_n, v) + plane.m_d > 0.00001f)
                    {
                        r.m_faces[i].m_plane.m_n = -1.0f * plane.m_n;
                        break;
                    }
                }
            }
        }

        return r;
    }

    std::optional<convex_polyhedron> clip(const frustum& f, const aabb& b)
    {
        auto clipper    = make_clipper(f);
        auto planes     = make_face_planes(b);

        for (auto i = 0; i < planes.size(); ++i)
        {
            if (clipper.clip(planes[i]) == -1)
            {
                return {};
            }
        }

        convex_polyhedron r;

        {
            auto r0 = clipper.convert();
            r.m_points = std::move(std::get<0>(r0));
            const auto& faces = std::get<1>(r0);

            for (auto i = 0; i < faces.size(); )
            {
                auto face_count = faces[i]; 
                i++;

                convex_polyhedron::polygon polygon;
                for (auto j = 0; j < face_count; ++j)
                {
                    polygon.m_indices.push_back( faces[i] );
                    i++;
                }

                r.m_faces.push_back(std::move(polygon));
            }
        }

        return r;
    }

    std::optional< convex_polyhedron > clip(const convex_polyhedron& f, const aabb& b)
    {
        auto clipper = make_clipper(f);
        auto planes = make_face_planes(b);

        for (auto i = 0; i < planes.size(); ++i)
        {
            if (clipper.clip(planes[i]) == -1)
            {
                return {};
            }
        }

        convex_polyhedron r;

        {
            auto r0 = clipper.convert();
            r.m_points = std::move(std::get<0>(r0));
            const auto& faces = std::get<1>(r0);

            for (auto i = 0; i < faces.size(); )
            {
                auto face_count = faces[i];
                i++;

                convex_polyhedron::polygon polygon;
                for (auto j = 0; j < face_count; ++j)
                {
                    polygon.m_indices.push_back(faces[i]);
                    i++;
                }

                r.m_faces.push_back(std::move(polygon));
            }
        }

        return r;
    }

    convex_polyhedron convex_hull_with_direction(const convex_polyhedron& body, const float3& vector)
    {
        convex_polyhedron r = body;

        std::vector<plane> planes;
        planes.resize(body.m_faces.size());

        //compute planes
        for (auto i = 0U; i < planes.size(); ++i)
        {
            const float3 a  = body.m_points[body.m_faces[i].m_indices[0]];
            const float3 b  = body.m_points[body.m_faces[i].m_indices[1]];
            const float3 c  = body.m_points[body.m_faces[i].m_indices[2]];
            planes[i]       = make_plane(a, b, c);
        }

        //orient normals to point inside
        for (auto i = 0U; i < planes.size(); ++i)
        {
            auto plane = planes[i];

            for (auto&& v : body.m_points)
            {
                if ( dot(plane.m_n, v) + plane.m_d > 0.00001f)
                {
                    planes[i].m_n = -1.0f * plane.m_n;
                    break;
                }
            }
        }

        std::vector<bool> face_vector;
        face_vector.resize(planes.size());

        for ( auto i = 0U; i < face_vector.size(); ++i)
        {
            face_vector[i] = dot( planes[i].m_n, vector ) > 0.0f;
        }

        //1. duplicate points and indices
        std::vector<int32_t> points_indices(body.m_points.size(), -1);

        for (auto i = 0U; i < face_vector.size(); ++i)
        {
            if (face_vector[i])
            {
                const auto& face = body.m_faces[i];
                for (auto p = 0; p < face.m_indices.size(); ++p)
                {
                    //split vertex
                    if ( points_indices[face.m_indices[p]] == -1 )
                    {
                        float3 v = r.m_points[ face.m_indices[p] ];
                        r.m_points.push_back(v);
                        int32_t index = static_cast<int32_t>( r.m_points.size() ) - 1;
                        points_indices[face.m_indices[p]] = index;
                    }
                }
            }
        }

        //now patch with quads
        for (auto i = 0U; i < face_vector.size(); ++i)
        {
            if (face_vector[i])
            {
                const auto&     face            = body.m_faces[i];
                const int32_t   indices_size    = static_cast<int32_t>(face.m_indices.size());

                for (auto p = 0; p < indices_size; ++p)
                {
                    const   int32_t computed_index_0 = p;
                    const   int32_t computed_index_1 = (p + 1) % indices_size;

                    int32_t index_0  = face.m_indices[computed_index_0];
                    int32_t index_1  = face.m_indices[computed_index_1];

                    int32_t index_0_ = points_indices[index_0];
                    int32_t index_1_ = points_indices[index_1];

                    convex_polyhedron::polygon polygon;

                    polygon.m_indices.push_back(index_0);
                    polygon.m_indices.push_back(index_1);
                    polygon.m_indices.push_back(index_1_);
                    polygon.m_indices.push_back(index_0_);

                    r.m_faces.push_back(std::move(polygon));
                }

                //replace the old face indices
                for (auto p = 0; p < indices_size; ++p)
                {
                    int32_t index_0             = face.m_indices[p];
                    r.m_faces[i].m_indices[p]   = points_indices[ index_0 ];
                }
            }
        }

        //now move the points to the light
        for (auto i = 0U; i < points_indices.size(); ++i)
        {
            if (points_indices[i] != -1)
            {
                r.m_points[points_indices[i]] = r.m_points[points_indices[i]] + vector;
            }
        }

        return r;
    }

    convex_polyhedron convex_hull_with_direction(const convex_polyhedron& body, const float3& vector, const aabb& clip_body)
    {
        float d = distance(clip_body.m_max, clip_body.m_min);
        return convex_hull_with_direction(body, d * vector);
    }

    convex_polyhedron convex_hull_with_point(const convex_polyhedron& body, const float3& point)
    {
        convex_polyhedron r = body;

        std::vector<plane> planes;
        planes.resize(body.m_faces.size());

        //compute planes
        for (auto i = 0U; i < planes.size(); ++i)
        {
            const float3 a = body.m_points[body.m_faces[i].m_indices[0]];
            const float3 b = body.m_points[body.m_faces[i].m_indices[1]];
            const float3 c = body.m_points[body.m_faces[i].m_indices[2]];
            planes[i] = make_plane(a, b, c);
        }

        //orient normals to point inside
        for (auto i = 0U; i < planes.size(); ++i)
        {
            auto plane = planes[i];

            for (auto&& v : body.m_points)
            {
                if (dot(plane.m_n, v) + plane.m_d > 0.00001f)
                {
                    planes[i].m_n = -1.0f * plane.m_n;
                    break;
                }
            }
        }

        {
            uint32_t inside_all = 0;

            //orient normals to point inside
            for (auto i = 0U; i < planes.size(); ++i)
            {
                auto plane = planes[i];

                for (auto&& v : body.m_points)
                {
                    if (dot(plane.m_n, v) + plane.m_d < -0.00001f)
                    {
                        inside_all = inside_all + 1;

                    }
                }
            }

            //the point is inside
            if (inside_all == planes.size())
            {
                return r;
            }
        }

        std::vector<bool> face_vector;
        face_vector.resize(planes.size());

        for (auto i = 0U; i < face_vector.size(); ++i)
        {
            face_vector[i] = dot(planes[i].m_n, point) > 0.00001f;
        }

        return r;
    }

    convex_triangulated_polyhedron triangulate( const convex_polyhedron& p )
    {
        //assert(false); //not implemented
        convex_triangulated_polyhedron r;

        r.m_points = p.m_points;
        r.m_faces.reserve(p.m_faces.size() * 2);

        for ( auto i = 0U; i < p.m_faces.size(); ++i)
        {
            const auto& f = p.m_faces[i];
            if (f.m_indices.size() == 3)
            {

            }
        }

        return r;
    }
}
