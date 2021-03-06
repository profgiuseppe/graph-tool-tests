// graph-tool -- a general graph modification and manipulation thingy
//
// Copyright (C) 2006-2013 Tiago de Paula Peixoto <tiago@skewed.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef GRAPH_ADAPTOR_HH
#define GRAPH_ADAPTOR_HH

#include <list>

#include <boost/config.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/range/join.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>

#include "transform_iterator.hh"

namespace boost {

//==============================================================================
// UndirectedAdaptor
// This class encapsulates a directed graph with parallel edges and provides a
// view of the graph as undirected with parallel edges.
// Encapsulated graph can be: VertexListGraph, EdgeListGraph, IncidenceGraph,
//                            AdjacencyGraph, VertexMutableGraph,
//                            EdgeMutableGraph, VertexMutablePropertyGraph,
//                            EdgeMutablePropertyGraph, BidirectionalGraph
// The undirected graph obeys the same concepts.
//==============================================================================
template <class Graph> class UndirectedAdaptor
{
public:
    UndirectedAdaptor(const Graph& g):_g(const_cast<Graph&>(g)){}

    typedef typename vertex_property_type<Graph>::type vertex_property_type;
    typedef typename edge_property_type<Graph>::type edge_property_type;
    typedef typename Graph::graph_tag graph_tag;
    typedef Graph graph_type;

    class EdgeDescriptor;
    typedef Graph original_graph_t;

    typedef typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor
        vertex_descriptor;
    typedef typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor
        edge_descriptor;

    typedef undirected_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category;
    typedef typename graph_traits<Graph>::traversal_category traversal_category;

#ifndef BOOST_GRAPH_NO_BUNDLED_PROPERTIES
    // Bundled properties support
    template<typename Descriptor>
    typename graph::detail::bundled_result<Graph, Descriptor>::type&
        operator[](Descriptor x) { return this->m_g[x]; }

    template<typename Descriptor>
    typename graph::detail::bundled_result<Graph, Descriptor>::type const&
        operator[](Descriptor x) const { return this->m_g[x]; }
#endif // BOOST_GRAPH_NO_BUNDLED_PROPERTIES

    const Graph& OriginalGraph() const {return _g;}
    Graph& OriginalGraph() {return _g;}

    static vertex_descriptor null_vertex() {graph_traits<Graph>::null_vertex();}

private:
    Graph& _g;
};

#ifndef BOOST_GRAPH_NO_BUNDLED_PROPERTIES
template<typename Graph>
struct vertex_bundle_type<UndirectedAdaptor<Graph> >:
        vertex_bundle_type<Graph> { };

template<typename Graph>
struct edge_bundle_type<UndirectedAdaptor<Graph> >:
        edge_bundle_type<Graph> { };
#endif // BOOST_GRAPH_NO_BUNDLED_PROPERTIES


//==============================================================================
// UndirectedAdaptor::EdgeDescriptor
//==============================================================================
template <class Graph>
class UndirectedAdaptor<Graph>::EdgeDescriptor:
        public graph_traits<Graph>::edge_descriptor
{
public:
    typedef typename graph_traits<Graph>::edge_descriptor original_edge_t;
    EdgeDescriptor(){}
    EdgeDescriptor(const original_edge_t& e): original_edge_t(e), _inverted(false) {}
    EdgeDescriptor(const original_edge_t& e,  bool inverted):
        original_edge_t(e), _inverted(inverted) {}

    bool IsInverted() const {return _inverted;}

    bool operator==(const EdgeDescriptor& e) const
    {
        return original_edge_t(e) == original_edge_t(*this);
    }

private:
    bool _inverted;
};

//==============================================================================
// UndirectedAdaptorEdgeIterator
//==============================================================================

template <class Graph>
struct make_undirected_edge
{
    make_undirected_edge(bool inverted): _inverted(inverted) {}
    make_undirected_edge(): _inverted(false) {}
    bool _inverted;
    typedef typename graph_traits<Graph>::edge_descriptor original_edge_t;
    typedef typename UndirectedAdaptor<Graph>::EdgeDescriptor result_type;
    typename UndirectedAdaptor<Graph>::EdgeDescriptor operator()(const original_edge_t& e) const
    {
        return typename UndirectedAdaptor<Graph>::EdgeDescriptor(e, _inverted);
    }
};

template <class Graph, class Iter>
struct transformed_iterator
{
    typedef typename mpl::if_<std::tr1::is_convertible<typename std::iterator_traits<Iter>::iterator_category,
                                                       std::random_access_iterator_tag>,
                              transform_random_access_iterator<make_undirected_edge<Graph>, Iter>,
                              transform_iterator<make_undirected_edge<Graph>, Iter> >::type type;
    typedef is_convertible<typename std::iterator_traits<Iter>::iterator_category,
                           std::random_access_iterator_tag> is_orig_ra;
    typedef is_convertible<typename std::iterator_traits<type>::iterator_category,
                           std::random_access_iterator_tag> is_ra;
    BOOST_STATIC_ASSERT((!is_orig_ra::value || is_ra::value));
};

template <class Graph>
struct get_undirected_edge_iterator
{
    typedef typename graph_traits<Graph>::edge_iterator ei_t;
    typedef typename transformed_iterator<Graph, ei_t>::type type;
};

//==============================================================================
// UndirectedAdaptorOutEdgeIterator
// this will iterate through both in_edges and out_edges of the underlying graph
//==============================================================================

template <class Iter1, class Iter2>
class joined_iterator
    : public boost::iterator_facade<joined_iterator<Iter1, Iter2>,
                                    typename std::iterator_traits<Iter1>::value_type,
                                    typename std::iterator_traits<Iter1>::iterator_category,
                                    typename std::iterator_traits<Iter1>::reference>
{
 public:
    joined_iterator() {}
    explicit joined_iterator(const std::pair<Iter1, Iter1>& range1,
                             const std::pair<Iter2, Iter2>& range2,
                             const Iter1& pos1, const Iter2& pos2)
        : _range1(range1), _range2(range2), _pos1(pos1), _pos2(pos2)
    { _flip = (_pos1 == _range1.second); }

 private:
    friend class boost::iterator_core_access;
    void increment()
    {
        if (_flip)
        {
            ++_pos2;
        }
        else
        {
            ++_pos1;
            _flip = (_pos1 == _range1.second);
        }
    }

    void decrement()
    {
        if (_flip)
        {
            if (_pos2 == _range2.first)
            {
                _flip = false;
                --_pos1;
            }
            else
            {
                --_pos2;
            }
        }
        else
        {
            --_pos1;
        }
    }

    typedef typename std::iterator_traits<Iter1>::difference_type diff_t;
    void advance(diff_t n)
    {
        diff_t d1 = _range1.second - _pos1;
        if (n < d1)
        {
            _pos1 += n;
        }
        else
        {
            _pos1 = _range1.second;
            _pos2 += n - d1;
            _flip = true;
        }
    }

    diff_t distance_to(joined_iterator const& other)
    {
        return (other._pos1 - _pos1) + (other._pos2 - _pos2);
    }

    bool equal(joined_iterator const& other) const
    {
        return (_pos2 == other._pos2 && _pos1 == other._pos1);
    }

    typename std::iterator_traits<Iter1>::reference dereference() const
    {
        if (_flip)
            return *_pos2;
        return *_pos1;
    }

    std::pair<Iter1, Iter1> _range1;
    std::pair<Iter2, Iter2> _range2;
    Iter1 _pos1;
    Iter2 _pos2;
    bool _flip;
};

template <class Graph>
struct get_undirected_out_edge_iterator
{
    typedef typename graph_traits<Graph>::out_edge_iterator eo_t;
    typedef typename graph_traits<Graph>::in_edge_iterator ei_t;
    typedef typename transformed_iterator<Graph, eo_t>::type teo_t;
    typedef typename transformed_iterator<Graph, ei_t>::type tei_t;
    typedef joined_iterator<teo_t, tei_t> type;
};



//==============================================================================
// UndirectedAdaptorAdjacencyIterator
// just keeps an internal reference to out_edge_iterator and calls target() when
// referenced
//==============================================================================

template <class Graph>
struct get_undirected_adjacency_iterator
{
    typedef typename get_undirected_out_edge_iterator<Graph>::type out_edge_iter_t;
    typedef typename boost::adjacency_iterator_generator<UndirectedAdaptor<Graph>,
                                                         typename graph_traits<Graph>::vertex_descriptor,
                                                         out_edge_iter_t>::type type;
};


//==============================================================================
// graph_traits<UndirectedAdaptor>
// this defines all the necessary types associated with UndirectedAdaptor
//==============================================================================
template <class Graph>
struct graph_traits<UndirectedAdaptor<Graph> > {
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename UndirectedAdaptor<Graph>::EdgeDescriptor edge_descriptor;

    typedef typename get_undirected_adjacency_iterator<Graph>::type adjacency_iterator;
    typedef typename get_undirected_out_edge_iterator<Graph>::type out_edge_iterator;
    typedef typename graph_traits<Graph>::in_edge_iterator in_edge_iterator;
    typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
    typedef typename get_undirected_edge_iterator<Graph>::type edge_iterator;


    typedef undirected_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category;
    typedef typename graph_traits<Graph>::traversal_category traversal_category;
    typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;
    typedef typename graph_traits<Graph>::edges_size_type edges_size_type;
    typedef typename graph_traits<Graph>::degree_size_type degree_size_type;

    static vertex_descriptor null_vertex()
    {
        return graph_traits<Graph>::null_vertex();
    }

private:
    typedef is_convertible<typename std::iterator_traits<typename graph_traits<Graph>::out_edge_iterator>::iterator_category,
                           std::random_access_iterator_tag> is_orig_ra;
    typedef is_convertible<typename std::iterator_traits<out_edge_iterator>::iterator_category,
                           std::random_access_iterator_tag> is_ra;
    BOOST_STATIC_ASSERT((!is_orig_ra::value || is_ra::value));
    // typedef is_convertible<typename std::iterator_traits<adjacency_iterator>::iterator_category,
    //                        std::random_access_iterator_tag> is_ara;
    // BOOST_STATIC_ASSERT((!is_orig_ra::value || is_ara::value));
};

template <class Graph>
struct graph_traits< const UndirectedAdaptor<Graph> >:
    public graph_traits<UndirectedAdaptor<Graph> > {};

//==============================================================================
// Nonmember functions
// these provide manipulation of the graph
//==============================================================================

//==============================================================================
// source(e,g)
//==============================================================================
template <class Graph>
inline typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor
source(typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor e,
       const UndirectedAdaptor<Graph>& g)
{
    typedef typename graph_traits<Graph>::edge_descriptor original_edge_t;
    if (e.IsInverted())
        return target(original_edge_t(e), g.OriginalGraph());
    else
        return source(original_edge_t(e), g.OriginalGraph());
}

//==============================================================================
// target(e,g)
//==============================================================================
template <class Graph>
inline typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor
target(typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor e,
       const UndirectedAdaptor<Graph>& g)
{
    typedef typename graph_traits<Graph>::edge_descriptor original_edge_t;
    if (e.IsInverted())
        return source(original_edge_t(e), g.OriginalGraph());
    else
        return target(original_edge_t(e), g.OriginalGraph());
}

//==============================================================================
// vertex(n,g)
//==============================================================================
template <class Graph>
inline typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor
vertex(typename graph_traits<UndirectedAdaptor<Graph> >::vertices_size_type n,
       const UndirectedAdaptor<Graph>& g)
{
    return vertex(n, g.OriginalGraph());
}

//==============================================================================
// vertices(g)
//==============================================================================
template <class Graph>
inline
std::pair<typename graph_traits<UndirectedAdaptor<Graph> >::vertex_iterator,
          typename graph_traits<UndirectedAdaptor<Graph> >::vertex_iterator >
vertices(const UndirectedAdaptor<Graph>& g)
{
    return vertices(g.OriginalGraph());
}

//==============================================================================
// edges(g)
//==============================================================================
template <class Graph>
inline
std::pair<typename graph_traits<UndirectedAdaptor<Graph> >::edge_iterator,
          typename graph_traits<UndirectedAdaptor<Graph> >::edge_iterator >
edges(const UndirectedAdaptor<Graph>& g)
{
    std::pair<typename graph_traits<Graph>::edge_iterator,
              typename graph_traits<Graph>::edge_iterator> range;
    range = edges(g.OriginalGraph());
    typedef typename graph_traits<UndirectedAdaptor<Graph> >::edge_iterator e_iter_t;
    return std::make_pair(e_iter_t(range.first, make_undirected_edge<Graph>()),
                          e_iter_t(range.second, make_undirected_edge<Graph>()));
}

//==============================================================================
// edge(u, v, g)
//==============================================================================
template <class Graph>
inline
std::pair<typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor,
          bool>
edge(typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
     typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor v,
     const UndirectedAdaptor<Graph>& g)
{
    bool reversed = false;
    std::pair<typename graph_traits<Graph>::edge_descriptor, bool> res
        = edge(u, v, g.OriginalGraph());

    if (!res.second)
    {
        res = edge(v, u, g.OriginalGraph());
        reversed = true;
    }
    return std::make_pair(typename UndirectedAdaptor<Graph>::EdgeDescriptor(res.first, reversed),
                          res.second);
}

//==============================================================================
// out_edges(u,g)
//==============================================================================
template <class Graph>
inline
std::pair<typename graph_traits<UndirectedAdaptor<Graph> >::out_edge_iterator,
          typename graph_traits<UndirectedAdaptor<Graph> >::out_edge_iterator >
out_edges(typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
          const UndirectedAdaptor<Graph>& g)
{
    typedef typename graph_traits<Graph>::out_edge_iterator eo_t;
    typedef typename graph_traits<Graph>::in_edge_iterator ei_t;

    std::pair<eo_t, eo_t> range1 = out_edges(u, g.OriginalGraph());
    std::pair<ei_t, ei_t> range2 = in_edges(u, g.OriginalGraph());

    typedef typename transformed_iterator<Graph, eo_t>::type teo_t;
    typedef typename transformed_iterator<Graph, ei_t>::type tei_t;

    std::pair<teo_t, teo_t> trange1(teo_t(range1.first,
                                          make_undirected_edge<Graph>(false)),
                                    teo_t(range1.second,
                                          make_undirected_edge<Graph>(false)));
    std::pair<tei_t, tei_t> trange2(tei_t(range2.first,
                                          make_undirected_edge<Graph>(true)),
                                    tei_t(range2.second,
                                          make_undirected_edge<Graph>(true)));

    joined_iterator<teo_t, tei_t> begin(trange1, trange2, trange1.first, trange2.first);
    joined_iterator<teo_t, tei_t> end(trange1, trange2, trange1.second, trange2.second);
    return std::make_pair(begin, end);
}

//==============================================================================
// adjacent_vertices(u,g)
//==============================================================================
template <class Graph>
inline
std::pair<typename graph_traits<UndirectedAdaptor<Graph> >::adjacency_iterator,
          typename graph_traits<UndirectedAdaptor<Graph> >::adjacency_iterator>
adjacent_vertices
    (typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
     const UndirectedAdaptor<Graph>& g)
{
    std::pair<typename graph_traits<UndirectedAdaptor<Graph> >::out_edge_iterator,
              typename graph_traits<UndirectedAdaptor<Graph> >::out_edge_iterator>
        e_range = out_edges(u, g);

    typedef typename graph_traits<UndirectedAdaptor<Graph> >::adjacency_iterator adjacency_iterator;
    return std::make_pair(adjacency_iterator(e_range.first, &g),
                          adjacency_iterator(e_range.second, &g));
}

//==============================================================================
// num_vertices(g)
//==============================================================================
template <class Graph>
inline typename graph_traits<UndirectedAdaptor<Graph> >::vertices_size_type
num_vertices(const UndirectedAdaptor<Graph>& g)
{
    return num_vertices(g.OriginalGraph());
}

//==============================================================================
// num_edges(g)
//==============================================================================
template <class Graph>
inline typename graph_traits<UndirectedAdaptor<Graph> >::edges_size_type
num_edges(const UndirectedAdaptor<Graph>& g)
{
    return num_edges(g.OriginalGraph());
}

//==============================================================================
// out_degree(u,g)
//==============================================================================
template <class Graph>
inline typename graph_traits<UndirectedAdaptor<Graph> >::degree_size_type
out_degree
    (typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
     const UndirectedAdaptor<Graph>& g)
{
    return (out_degree(u, g.OriginalGraph())+in_degree(u,g.OriginalGraph()));
}

//==============================================================================
// degree(u,g)
//==============================================================================
template <class Graph>
inline typename graph_traits<UndirectedAdaptor<Graph> >::degree_size_type
degree(typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
       const UndirectedAdaptor<Graph>& g)
{
    return out_degree(u, g);
}


//==============================================================================
// add_vertex(g)
//==============================================================================
template <class Graph>
inline typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor
add_vertex(UndirectedAdaptor<Graph>& g)
{
    return add_vertex(g.OriginalGraph());
}

//==============================================================================
// add_vertex(vp,g)
//==============================================================================
template <class Graph, class VertexProperties>
inline typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor
add_vertex(const VertexProperties& p, UndirectedAdaptor<Graph>& g)
{
    return add_vertex(p, g.OriginalGraph());
}

//==============================================================================
// clear_vertex(u,g)
//==============================================================================
template <class Graph>
inline void clear_vertex
    (typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
     UndirectedAdaptor<Graph>& g)
{
    clear_vertex(u, g.OriginalGraph());
}

//==============================================================================
// remove_vertex(u,g)
//==============================================================================
template <class Graph>
inline void remove_vertex
    (typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
     UndirectedAdaptor<Graph>& g)
{
    remove_vertex(u, g.OriginalGraph());
}

//==============================================================================
// add_edge(u,v,g)
//==============================================================================
template <class Graph>
inline
std::pair<typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor,
          bool>
add_edge(typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
         typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor v,
         UndirectedAdaptor<Graph>& g)
{
    std::pair<typename graph_traits<Graph>::edge_descriptor, bool> retval =
        add_edge(u,v,g.OriginalGraph());
    return std::make_pair
        (typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor
         (retval.first,false),
         retval.second);
}

//==============================================================================
// add_edge(u,v,ep,g)
//==============================================================================
template <class Graph, class EdgeProperties>
inline
std::pair<typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor,
          bool>
add_edge(typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
         typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor v,
         const EdgeProperties& ep, UndirectedAdaptor<Graph>& g)
{
    std::pair<typename graph_traits<Graph>::edge_descriptor, bool> retval =
        add_edge(u,v,ep,g.OriginalGraph());
    return std::make_pair
        (typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor
         (retval.first,false),
         retval.second);
}

//==============================================================================
// remove_edge(u,v,g)
//==============================================================================
template <class Graph>
inline void remove_edge
    (typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor u,
     typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor v,
     UndirectedAdaptor<Graph>& g)
{
    remove_edge(u,v,g.OriginalGraph());
    remove_edge(v,u,g.OriginalGraph());
}

//==============================================================================
// remove_edge(e,g)
//==============================================================================
template <class Graph>
inline void remove_edge
    (typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor e,
     UndirectedAdaptor<Graph>& g)
{
    remove_edge(typename graph_traits<Graph>::edge_descriptor(e),
                g.OriginalGraph());
}

//==============================================================================
// remove_edge(e_iter,g)
//==============================================================================
template <class Graph>
inline void remove_edge
    (typename graph_traits<UndirectedAdaptor<Graph> >::out_edge_iterator iter,
     UndirectedAdaptor<Graph>& g)
{
    remove_edge(*iter, g);
}

//==============================================================================
// remove_out_edge_if(v,predicate,g)
//==============================================================================
template <class Graph, class Predicate>
inline void remove_out_edge_if
    (typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor v,
     Predicate predicate, UndirectedAdaptor<Graph>& g)
{
    std::list<typename UndirectedAdaptor<Graph>::EdgeDescriptor> removed_edges;
    typedef typename graph_traits<UndirectedAdaptor<Graph> >::out_edge_iterator
        iter_t;
    std::pair<iter_t, iter_t> edge_range;
    edge_range = out_edges(v,g);
    for(iter_t iter = edge_range.first; iter != edge_range.second; ++iter)
        if (predicate(*iter))
            removed_edges.push_front(*iter);

    for(typeof(removed_edges.begin()) iter = removed_edges.begin();
        iter != removed_edges.end(); ++iter)
        remove_edge(*iter,g);
}


//==============================================================================
// Property maps
//==============================================================================

//==============================================================================
// vertex_property<UndirectedAdaptor>
//==============================================================================
template <class Graph>
class vertex_property<UndirectedAdaptor<Graph> >
{
public:
    typedef typename vertex_property<Graph>::type type;
};

//==============================================================================
// vertex_property_type<UndirectedAdaptor>
//==============================================================================
template <class Graph>
class vertex_property_type<UndirectedAdaptor<Graph> >
{
public:
    typedef typename vertex_property_type<Graph>::type type;
};

//==============================================================================
// edge_property<UndirectedAdaptor>
//==============================================================================
template <class Graph>
class edge_property<UndirectedAdaptor<Graph> >
{
public:
    typedef typename edge_property<Graph>::type type;
};

//==============================================================================
// edge_property_type<UndirectedAdaptor>
//==============================================================================
template <class Graph>
class edge_property_type<UndirectedAdaptor<Graph> >
{
public:
    typedef typename edge_property_type<Graph>::type type;
};

//==============================================================================
// property_map<UndirecterdAdaptor, PropertyTag>
//==============================================================================
template <class Graph, class PropertyTag>
class property_map<UndirectedAdaptor<Graph>, PropertyTag>
{
public:
    typedef typename property_map<Graph, PropertyTag>::type type;
    typedef typename property_map<Graph, PropertyTag>::const_type const_type;
};

//==============================================================================
// property_map<UndirectedAdaptor, T Bundle::*>
//==============================================================================
template <typename Graph, typename T, typename Bundle>
class property_map<UndirectedAdaptor<Graph>, T Bundle::*>
{
public:
    typedef typename property_map<Graph, T Bundle::*>::type type;
    typedef typename property_map<Graph, T Bundle::*>::const_type const_type;
};


//==============================================================================
// get(tag,g)
//==============================================================================
template <class PropertyTag, class Graph>
inline typename property_map<UndirectedAdaptor<Graph>, PropertyTag>::type
get(PropertyTag tag, UndirectedAdaptor<Graph>& g)
{
    return get(tag, g.OriginalGraph());
}

//==============================================================================
// const get(tag,g)
//==============================================================================
template <class PropertyTag, class Graph>
inline typename property_map<UndirectedAdaptor<Graph>, PropertyTag>::const_type
get(PropertyTag tag, const UndirectedAdaptor<Graph>& g)
{
    return get(tag, g.OriginalGraph());
}

//==============================================================================
// get(tag,g,v)
//==============================================================================
template <class PropertyTag, class Graph>
inline
typename property_traits
    <typename property_map<UndirectedAdaptor<Graph>,
                           PropertyTag>::const_type >::value_type
get(PropertyTag tag, const UndirectedAdaptor<Graph>& g,
    typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor v)
{
    return get(tag, g.OriginalGraph(), v);
}

//==============================================================================
// get(tag,g,e)
//==============================================================================
template <class PropertyTag, class Graph>
inline
typename property_traits
    <typename property_map<UndirectedAdaptor<Graph>,
                           PropertyTag>::const_type >::value_type
get(PropertyTag tag, const UndirectedAdaptor<Graph>& g,
    typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor e)
{
    return get(tag, g.OriginalGraph(), e.OriginalEdge());
}

//==============================================================================
// put(tag, g, v, value)
//==============================================================================
template <class Graph, class PropertyTag, class Value>
inline
void put(PropertyTag tag, UndirectedAdaptor<Graph>& g,
         typename graph_traits<UndirectedAdaptor<Graph> >::vertex_descriptor v,
         const Value& value)
{
    put(tag, g.OriginalGraph(), v, value);
}

//==============================================================================
// put(tag, g, e, value)
//==============================================================================
template <class Graph, class PropertyTag, class X, class Value>
inline
void put(PropertyTag tag, const UndirectedAdaptor<Graph>& g,
         typename graph_traits<UndirectedAdaptor<Graph> >::edge_descriptor e,
         const Value &value)
{
    put(tag, g.OriginalGraph(), e.OriginalEdge(), value);
}

//==============================================================================
// get_property(g,tag)
//==============================================================================
template <class Graph, class GraphProperties, class GraphPropertyTag>
inline
typename property_value<GraphProperties, GraphPropertyTag>::type&
get_property(UndirectedAdaptor<Graph>& g, GraphPropertyTag tag)
{
    get_property(g.OriginalGraph(), tag);
}

//==============================================================================
// const get_property(g,tag)
//==============================================================================
template <class Graph, class GraphProperties, class GraphPropertyTag>
inline
const typename property_value<GraphProperties, GraphPropertyTag>::type&
get_property(const UndirectedAdaptor<Graph>& g, GraphPropertyTag tag)
{
    get_property(g.OriginalGraph(), tag);
}

} // namespace boost


#endif // GRAPH_ADAPTOR_HH
