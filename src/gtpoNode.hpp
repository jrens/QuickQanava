/*
    The MIT License (MIT)

    Copyright (c) 2016 Benoit AUTHEMAN

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the GTpo software.
//
// \file	gtpoNode.hpp
// \author	benoit@qanava.org
// \date	2016 01 22
//-----------------------------------------------------------------------------

namespace gtpo { // ::gtpo

/* GenNode Edges Management *///-----------------------------------------------
template < class Config >
auto GenNode< Config >::addOutEdge( WeakEdge sharedOutEdge ) -> void
{
    assert( !sharedOutEdge.expired() );
    SharedNode node = this->shared_from_this(); // FIXME g++: this-> necessary
    SharedEdge outEdge = sharedOutEdge.lock( );
    SharedNode outEdgeSrc = outEdge->getSrc().lock();
    if ( outEdge ) {
        if ( !outEdgeSrc || outEdgeSrc != node )  // Out edge source should point to target node
            outEdge->setSrc( node );
        Config::template insert< WeakEdges >::into( _outEdges, sharedOutEdge );
        if ( !outEdge->getDst().expired() ) {
            Config::template insert< WeakNodes >::into( _outNodes, outEdge->getDst() );
            notifyOutNodeInserted( outEdge->getDst() );
        }
    }
}

template < class Config >
auto GenNode< Config >::addInEdge( WeakEdge sharedInEdge ) -> void
{
    assert( !sharedInEdge.expired() );
    SharedNode node = this->shared_from_this();
    SharedEdge inEdge = sharedInEdge.lock( );
    SharedNode inEdgeDst = inEdge->getDst().lock();
    if ( inEdge ) {
        if ( !inEdgeDst || inEdgeDst != node ) // In edge destination should point to target node
            inEdge->setDst( node );
        Config::template insert< WeakEdges >::into( _inEdges, sharedInEdge );
        if ( !inEdge->getSrc().expired() ) {
            Config::template insert< WeakNodes >::into( _inNodes, inEdge->getSrc() );
            notifyInNodeInserted( inEdge->getSrc() );
        }
    }
}

template < class Config >
auto GenNode< Config >::removeOutEdge( const WeakEdge outEdge ) -> void
{
    gtpo::assert_throw( !outEdge.expired(), "gtpo::GenNode<>::removeOutEdge(): Error: Out edge has expired" );
    SharedNode sharedNode = this->shared_from_this();
    SharedEdge ownedOutEdge = outEdge.lock( );
    SharedNode ownedOutEdgeSrc = ownedOutEdge->getSrc().lock();
    gtpo::assert_throw( ownedOutEdgeSrc != nullptr &&    // Out edge src must be this node
                        ownedOutEdgeSrc == sharedNode, "gtpo::GenNode<>::removeOutEdge(): Error: Out edge source is expired or different from this node.");
    auto ownedOutEdgeDst = ownedOutEdge->getDst().lock();
    gtpo::assert_throw( ownedOutEdgeDst != nullptr, "gtpo::GenNode<>::removeOutEdge(): Error: Out edge destination is expired." );
    notifyOutNodeRemoved( ownedOutEdge->getDst() );
    Config::template remove< WeakEdges >::from( _outEdges, outEdge );
    Config::template remove< WeakNodes >::from( _outNodes, ownedOutEdge->getDst() );
    if ( getInDegree() == 0 ) {
        Graph* graph{ getGraph() };
        if ( graph != nullptr )
            graph->installRootNode( WeakNode( sharedNode ) );
    }
    notifyOutNodeRemoved();
}

template < class Config >
auto GenNode< Config >::removeInEdge( const WeakEdge inEdge ) -> void
{
    gtpo::assert_throw( !inEdge.expired(), "gtpo::GenNode<>::removeInEdge(): Error: In edge has expired" );
    SharedNode sharedNode = this->shared_from_this();
    SharedEdge ownedInEdge = inEdge.lock( );
    SharedNode ownedInEdgeDst = ownedInEdge->getDst().lock();
    gtpo::assert_throw( ownedInEdgeDst != nullptr &&    // in edge dst must be this node
                        ownedInEdgeDst == sharedNode, "gtpo::GenNode<>::removeInEdge(): Error: In edge destination is expired or different from this node.");

    auto ownedInEdgeSrc = ownedInEdge->getSrc().lock();
    gtpo::assert_throw( ownedInEdgeSrc != nullptr, "gtpo::GenNode<>::removeInEdge(): Error: In edge source is expired." );
    notifyInNodeRemoved( ownedInEdge->getSrc() );
    Config::template remove< WeakEdges >::from( _inEdges, inEdge );
    Config::template remove< WeakNodes >::from( _inNodes, ownedInEdge->getSrc() );
    if ( getInDegree() == 0 ) {
        Graph* graph{ getGraph() };
        if ( graph != nullptr )
            graph->installRootNode( WeakNode( sharedNode ) );
    }
    notifyInNodeRemoved();
}
//-----------------------------------------------------------------------------

/* GenNode Behaviour Notifications *///----------------------------------------
template < class Config >
auto    GenNode< Config >::notifyInNodeInserted( WeakNode& inNode ) -> void
{
    notifyBehaviours< WeakNode >( &gtpo::NodeBehaviour<Config>::inNodeInserted, inNode );
    sNotifyBehaviours( [&](auto& behaviour) { behaviour.inNodeInserted( inNode ); } );
}

template < class Config >
auto    GenNode< Config >::notifyInNodeRemoved( WeakNode& inNode ) -> void
{
    notifyBehaviours< WeakNode >( &gtpo::NodeBehaviour<Config>::inNodeRemoved, inNode );
    sNotifyBehaviours( [&](auto& behaviour) { behaviour.inNodeRemoved( inNode ); } );
}

template < class Config >
auto    GenNode< Config >::notifyInNodeRemoved() -> void
{
    notifyBehaviours< WeakNode >( &gtpo::NodeBehaviour<Config>::inNodeRemoved );
    sNotifyBehaviours( [&](auto& behaviour) { behaviour.inNodeRemoved(); } );
}

template < class Config >
auto    GenNode< Config >::notifyOutNodeInserted( WeakNode& outNode ) -> void
{
    notifyBehaviours< WeakNode >( &gtpo::NodeBehaviour<Config>::outNodeInserted, outNode );
    sNotifyBehaviours( [&](auto& behaviour) { behaviour.outNodeInserted( outNode ); } );
}

template < class Config >
auto    GenNode< Config >::notifyOutNodeRemoved( WeakNode& outNode ) -> void
{
    notifyBehaviours< WeakNode >( &gtpo::NodeBehaviour<Config>::outNodeRemoved, outNode );
    sNotifyBehaviours( [&](auto& behaviour) { behaviour.outNodeRemoved( outNode ); } );
}

template < class Config >
auto    GenNode< Config >::notifyOutNodeRemoved() -> void
{
    notifyBehaviours< WeakNode >( &gtpo::NodeBehaviour<Config>::outNodeRemoved );
    sNotifyBehaviours( [&](auto& behaviour) { behaviour.outNodeRemoved( ); } );
}
//-----------------------------------------------------------------------------

} // ::gtpo
