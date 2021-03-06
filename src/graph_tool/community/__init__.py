#! /usr/bin/env python
# -*- coding: utf-8 -*-
#
# graph_tool -- a general graph manipulation python module
#
# Copyright (C) 2006-2013 Tiago de Paula Peixoto <tiago@skewed.de>
# Copyright (C) 2013 Giuseppe Profiti <profgiuseppe@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
``graph_tool.community`` - Community structure
----------------------------------------------

This module contains algorithms for the computation of community structure on
graphs.


Stochastic blockmodel inference
+++++++++++++++++++++++++++++++

Summary
=======

.. autosummary::
   :nosignatures:

   minimize_blockmodel_dl
   BlockState
   mcmc_sweep
   collect_vertex_marginals
   collect_edge_marginals
   mf_entropy
   bethe_entropy
   model_entropy
   get_max_B
   get_akc
   min_dist
   condensation_graph


Modularity-based community detection
++++++++++++++++++++++++++++++++++++

Summary
=======

.. autosummary::
   :nosignatures:

   community_structure
   modularity
   louvain


Contents
++++++++
"""

from __future__ import division, absolute_import, print_function
import sys
if sys.version_info < (3,):
    range = xrange

from .. dl_import import dl_import
dl_import("from . import libgraph_tool_community")

from .. import _degree, _prop, Graph, GraphView, libcore, _get_rng
import random
import sys

__all__ = ["minimize_blockmodel_dl", "BlockState", "mcmc_sweep",
           "collect_edge_marginals", "collect_vertex_marginals",
           "bethe_entropy", "mf_entropy", "model_entropy", "get_max_B",
           "get_akc", "min_dist", "condensation_graph",  "community_structure",
           "modularity","louvain"]

from . blockmodel import minimize_blockmodel_dl, BlockState, mcmc_sweep, \
    model_entropy, get_max_B, get_akc, min_dist, condensation_graph, \
    collect_edge_marginals, collect_vertex_marginals, bethe_entropy, mf_entropy

def community_structure(g, n_iter, n_spins, gamma=1.0, corr="erdos",
                        spins=None, weight=None, t_range=(100.0, 0.01),
                        verbose=False, history_file=None):
    r"""
    Obtain the community structure for the given graph, using a Potts model approach.

    Parameters
    ----------
    g :  :class:`~graph_tool.Graph`
        Graph to be used.
    n_iter : int
        Number of iterations.
    n_spins : int
        Number of maximum spins to be used.
    gamma : float (optional, default: 1.0)
        The :math:`\gamma` parameter of the hamiltonian.
    corr : string (optional, default: "erdos")
        Type of correlation to be assumed: Either "erdos", "uncorrelated" and
        "correlated".
    spins : :class:`~graph_tool.PropertyMap`
        Vertex property maps to store the spin variables. If this is specified,
        the values will not be initialized to a random value.
    weight : :class:`~graph_tool.PropertyMap` (optional, default: None)
        Edge property map with the optional edge weights.
    t_range : tuple of floats (optional, default: (100.0, 0.01))
        Temperature range.
    verbose : bool (optional, default: False)
        Display verbose information.
    history_file : string (optional, default: None)
        History file to keep information about the simulated annealing.

    Returns
    -------
    spins : :class:`~graph_tool.PropertyMap`
        Vertex property map with the spin values.

    See Also
    --------
    community_structure: Obtain the community structure
    modularity: Calculate the network modularity
    condensation_graph: Network of communities, or blocks

    Notes
    -----
    The method of community detection covered here is an implementation of what
    was proposed in [reichard-statistical-2006]_. It
    consists of a `simulated annealing`_ algorithm which tries to minimize the
    following hamiltonian:

    .. math::

        \mathcal{H}(\{\sigma\}) = - \sum_{i \neq j} \left(A_{ij} -
        \gamma p_{ij}\right) \delta(\sigma_i,\sigma_j)

    where :math:`p_{ij}` is the probability of vertices i and j being connected,
    which reduces the problem of community detection to finding the ground
    states of a Potts spin-glass model. It can be shown that minimizing this
    hamiltonan, with :math:`\gamma=1`, is equivalent to maximizing
    Newman's modularity ([newman-modularity-2006]_). By increasing the parameter
    :math:`\gamma`, it's possible also to find sub-communities.

    It is possible to select three policies for choosing :math:`p_{ij}` and thus
    choosing the null model: "erdos" selects a Erdos-Reyni random graph,
    "uncorrelated" selects an arbitrary random graph with no vertex-vertex
    correlations, and "correlated" selects a random graph with average
    correlation taken from the graph itself. Optionally a weight property
    can be given by the `weight` option.


    The most important parameters for the algorithm are the initial and final
    temperatures (`t_range`), and total number of iterations (`max_iter`). It
    normally takes some trial and error to determine the best values for a
    specific graph. To help with this, the `history` option can be used, which
    saves to a chosen file the temperature and number of spins per iteration,
    which can be used to determined whether or not the algorithm converged to
    the optimal solution. Also, the `verbose` option prints the computation
    status on the terminal.

    .. note::

        If the spin property already exists before the computation starts, it's
        not re-sampled at the beginning. This means that it's possible to
        continue a previous run, if you saved the graph, by properly setting
        `t_range` value, and using the same `spin` property.

    If enabled during compilation, this algorithm runs in parallel.

    Examples
    --------

    This example uses the network :download:`community.xml <community.xml>`.

    >>> from pylab import *
    >>> from numpy.random import seed
    >>> seed(42)
    >>> g = gt.load_graph("community.xml")
    >>> pos = g.vertex_properties["pos"]
    >>> spins = gt.community_structure(g, 10000, 20, t_range=(5, 0.1),
    ...                                history_file="community-history1")
    >>> gt.graph_draw(g, pos=pos, vertex_fill_color=spins, output_size=(420, 420), output="comm1.pdf")
    <...>

    .. testcode::
       :hide:

       gt.graph_draw(g, pos=pos, vertex_fill_color=spins, output_size=(420, 420), output="comm1.png")

    >>> spins = gt.community_structure(g, 10000, 40, t_range=(5, 0.1),
    ...                                gamma=2.5, history_file="community-history2")
    >>> gt.graph_draw(g, pos=pos, vertex_fill_color=spins, output_size=(420, 420), output="comm2.pdf")
    <...>

    .. testcode::
       :hide:

       gt.graph_draw(g, pos=pos, vertex_fill_color=spins, output_size=(420, 420), output="comm2.png")

    >>> figure(figsize=(6, 4))
    <...>
    >>> xlabel("iterations")
    <...>
    >>> ylabel("number of communities")
    <...>
    >>> a = loadtxt("community-history1").transpose()
    >>> plot(a[0], a[2])
    [...]
    >>> savefig("comm1-hist.pdf")

    .. testcode::
       :hide:

       savefig("comm1-hist.png")

    >>> clf()
    >>> xlabel("iterations")
    <...>
    >>> ylabel("number of communities")
    <...>
    >>> a = loadtxt("community-history2").transpose()
    >>> plot(a[0], a[2])
    [...]
    >>> savefig("comm2-hist.pdf")

    .. testcode::
       :hide:

       savefig("comm2-hist.png")


    The community structure with :math:`\gamma=1`:

    .. image:: comm1.*
    .. image:: comm1-hist.*

    The community structure with :math:`\gamma=2.5`:

    .. image:: comm2.*
    .. image:: comm2-hist.*


    References
    ----------
    .. [reichard-statistical-2006] Joerg Reichardt and Stefan Bornholdt,
       "Statistical Mechanics of Community Detection", Phys. Rev. E 74
       016110 (2006), :doi:`10.1103/PhysRevE.74.016110`, :arxiv:`cond-mat/0603718`
    .. [newman-modularity-2006] M. E. J. Newman, "Modularity and community
       structure in networks", Proc. Natl. Acad. Sci. USA 103, 8577-8582 (2006),
       :doi:`10.1073/pnas.0601602103`, :arxiv:`physics/0602124`
    .. _simulated annealing: http://en.wikipedia.org/wiki/Simulated_annealing
    """

    if spins is None:
        spins = g.new_vertex_property("int32_t")
    if history_file is None:
        history_file = ""
    ug = GraphView(g, directed=False)
    libgraph_tool_community.community_structure(ug._Graph__graph, gamma, corr,
                                                n_iter, t_range[1], t_range[0],
                                                n_spins, _get_rng(),
                                                verbose, history_file,
                                                _prop("e", ug, weight),
                                                _prop("v", ug, spins))
    return spins


def modularity(g, prop, weight=None):
    r"""
    Calculate Newman's modularity.

    Parameters
    ----------
    g : :class:`~graph_tool.Graph`
        Graph to be used.
    prop : :class:`~graph_tool.PropertyMap`
        Vertex property map with the community partition.
    weight : :class:`~graph_tool.PropertyMap` (optional, default: None)
        Edge property map with the optional edge weights.

    Returns
    -------
    modularity : float
        Newman's modularity.

    See Also
    --------
    community_structure: obtain the community structure
    modularity: calculate the network modularity
    condensation_graph: Network of communities, or blocks

    Notes
    -----

    Given a specific graph partition specified by `prop`, Newman's modularity
    [newman-modularity-2006]_ is defined by:

    .. math::

          Q = \sum_s e_{ss}-\left(\sum_r e_{rs}\right)^2

    where :math:`e_{rs}` is the fraction of edges which fall between
    vertices with spin s and r.

    If enabled during compilation, this algorithm runs in parallel.

    Examples
    --------
    >>> from pylab import *
    >>> from numpy.random import seed
    >>> seed(42)
    >>> g = gt.load_graph("community.xml")
    >>> spins = gt.community_structure(g, 10000, 10)
    >>> gt.modularity(g, spins)
    0.535314188562404

    References
    ----------
    .. [newman-modularity-2006] M. E. J. Newman, "Modularity and community
       structure in networks", Proc. Natl. Acad. Sci. USA 103, 8577-8582 (2006),
       :doi:`10.1073/pnas.0601602103`, :arxiv:`physics/0602124`
    """

    ug = GraphView(g, directed=False)
    m = libgraph_tool_community.modularity(ug._Graph__graph,
                                           _prop("e", ug, weight),
                                           _prop("v", ug, prop))
    return m

####EXPERIMENTAL
def louvain(g, weight=None, partition=None, threshold=0.00001, verbose=False):
    r"""
    Calculate a community partition using the Louvain method based
    on modularity maximization.

    Parameters
    ----------
    g : :class:`~graph_tool.Graph`
        Graph to be used. Only undirected graphs are supported.
    weight : :class:`~graph_tool.PropertyMap` (optional, default: None)
        Edge property map with the optional edge weights.
    partition : :class:`~graph_tool.PropertyMap` (optional, default: None)
        Initial partitioning to start with.
    threshold : float (optional, default: 0.00001)
        The threshold used to stop the algorithm.
    verbose : boolean (optional, default: False)
        Enables the verbose evaluation of the algorithm.

    Returns
    -------
    prop : :class:`~graph_tool.PropertyMap`
        Vertex property map with the community partition.

    See Also
    --------
    community_structure: obtain the community structure
    modularity: calculate the network modularity
    condensation_graph: Network of communities, or blocks

    Notes
    -----

    Given a graph and optional edge weights specified by `weight`, the Louvain
    method of modularity maximization [blondel-fast-2008]_ proceeds as
    follow:

        * each node is assigned to a different community
        * it is evaluated the gain in modularity by moving each node in its
        neighbouring communities
        * the assignment that leads to the best increase is performed
        * repeat from step 2 on the condensation graph

    The algorithm ends when the increase in modularity is below a specified threshold.
    
    .. note ::
        The quality of the partition returned by the algorithm depends on the
        order in which the nodes are considered.
        
    References
    ----------
    .. [blondel-fast-2008] V. D. Blondel, et al. "Fast unfolding of communities
       in large networks", Journal of Statistical Mechanics: Theory and
       Experiment 2008.10 (2008): P10008,
       :doi:`10.1088/1742-5468/2008/10/P10008` :arxiv:`physics/0803.0476`
    """
    if type(g) is not Graph:
        raise TypeError('Method must be used on graph_tool undirected graphs')
    
    if g.is_directed():
        raise TypeError('Method must be used on graph_tool undirected graphs')

    #if the graph has no edges...
    if g.num_edges()==0:
        if verbose:
            print("The graph has no edges...")
        #... if there is already a partition, return the same
        if partition:
            if verbose:
                print("Returning input partition")
            return partition
        else:
            #... otherwise, each vertex goes into its own community
            comm = g.vertex_index.copy('int')
            if verbose:
                print("Returning single nodes partition")
            return comm

    if partition:
        #in case a partition was provided, use the condensation graph
        if verbose:
            print("Evaluating condensation graph")
        work_graph, old_partition, part_sizes, weight, va, ea = condensation_graph(g, partition, eweight=weight)
    else:
        #otherwise, each vertex goes into its own community
        if verbose:
            print("Copying graph")
        work_graph = g.copy()
        old_partition = g.vertex_index.copy('int')

    dendo = [old_partition]
    #perform the evaluation
    if verbose:
        print("Evaluating 1 step")

    kw = work_graph.degree_property_map('total',weight)
    #this is the same no matter the level in the dendogram
    m = sum(kw.a)
    
    new_partition = louvain_step(work_graph,old_partition,threshold,weight,m,kw,verbose)
    thepart = g.copy_property(new_partition)
    dendo.append(thepart)

    return dendo

def louvain_step(g,partition,threshold,weight=None,m=None,kw=None,verbose=False):
    #(weighted) degree of the nodes
    if verbose:
        print("Evaluating degrees")
    if not kw:
        kw = g.degree_property_map('total',weight)
    #result
    new_partition = g.copy_property(partition)
    #trading memory for speed
    c2v = reverse_dict(new_partition,g.vertices())
    #temporary, it leads to numerical errors **WARNING**
    if not m:
        m = sum(kw.a)
    if verbose:
        print("2m=",m)        

    neigh = dict((v,set(v.all_neighbours())) for v in g.vertices())
    for v in g.vertices():
        #for all the neighbours
        if v in neigh[v]:
            neigh[v].remove(v)

    #a fictional change just to enter the loop
    changed = True
    #until the increase is below the threshold
    while changed:
        if verbose:
            print("The partition has changed...")
        changed = False
        #for each node
        for v in g.vertices():
            if verbose:
                print("Evaluating node",v)
            increase = (0.0,)
            #the current community
            current_comm = new_partition[v]
            if verbose:
                print("Current community is ",current_comm)
            s2 = sum([kw[vv] for vv in c2v[current_comm]])-kw[v]
                     
            d = sum([weight[e] for e in v.all_edges() if e.target() in c2v[current_comm]])
            ee = g.edge(v,v)
            if ee:
                d -= weight[ee]
#            if verbose:
#                print("s2=",s2,"\nd=",d)
            #the new community, the same for the moment
            newcom = new_partition[v]
            
            #set of neighbouring communities
            neighcomms = set([new_partition[vv] for vv in neigh[v]])
            if current_comm in neighcomms:
                neighcomms.remove(current_comm)
            if verbose:
                print("Neighbouring communities:",neighcomms)
            for n in neighcomms:
                #evaluate the increase in modularity
                deltaq = deltas(g,v,new_partition,c2v,
                                n,s2,d,
                                weight,kw,m)
                if verbose:
                    print("Moving to community ",n," gives ",deltaq[0])
                #if the increase is better than the previous one
                if deltaq[0] > increase[0]:
                    if verbose:
                        print("Which is higer than ",increase[0])
                    increase = deltaq
                    newcom = n
            #if the increase is above the threshold
            if (increase[0]>threshold):
                if verbose:
                    print(increase[0]," > ",threshold,": changing community to ",newcom)
                c2v[current_comm].remove(v)
                new_partition[v]=newcom
                c2v[newcom].add(v)
                changed = True

    return new_partition

def deltas(g,x,partition,c2v,new_community,s2,d,w=None,kw=None,m=None):
    r"""
    Calculate the increase in modularity if a node is moved
    from its current community to a new one.

    Parameters
    ----------
    g : :class:`~g_tool.Graph`
        Graph to be used. Only undirected graphs are supported.
    x : :class:`~g_tool.Vertex`
        The vertex that would be moved.
    partition : :class:`~g_tool.PropertyMap` (optional, default: None)
        The current partitioning of the graph.
    new_community: int
        The community where the node should be moved to.
    w : :class:`~g_tool.PropertyMap` (optional, default: None)
        Edge property map with the optional edge weights.
    kw : :class:`~g_tool.PropertyMap` (optional, default: None)
        Vertex property map with the (weighted) degree of each node.
    m : float (optional, default: False)
        Double of the total weight of the graph, i.e.:

    .. math::

          m = \sum_{ij} A_{ij}

    where i and j are vertex of the graph and A is the adjacency matrix
    of the graph.

    Returns
    -------
    deltaq : float
        The increase in modularity.
    kw : :class:`~g_tool.PropertyMap`
        Vertex property map with the (weighted) degree of each node.
    m : float
        Double of the total weight of the graph.

    Notes
    -------
    """
    
    #evaluates the weighted degree, if needed
    if not kw:
        kw = g.degree_property_map('total',w)
        
    #the increase in modularity depends on the weight of the edges
    #from vertex x to the current and the new communities.
    #we don't want to consider self loops
    if w:
        delta = sum([w[e] for e in x.all_edges() if e.target() in c2v[new_community][0]])
    else:
        delta = sum([1 for e in x.all_edges() if e.target() in c2v[new_community]])
    delta = delta-d
    #print("Delta=",delta)
    s1 = sum([kw[v] for v in c2v[new_community]])
    s = s1-s2+kw[x]
    #print("s=",s)
    if not m:    
        m = sum([kw[v] for v in g.vertices()])
        
    deltaq = 2*(delta - (kw[x]/m)*s)/m
    return deltaq,kw,m

def reverse_dict(d,keys=None):
    if not keys:
        keys = d.keys()

    c2v = dict()
    
    for v in keys:
        if d[v] in c2v:
            c2v[d[v]].add(v)
        else:
            c2v[d[v]]=set([v])

    return c2v
