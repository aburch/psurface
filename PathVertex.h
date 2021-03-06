#ifndef PATH_VERTEX_H
#define PATH_VERTEX_H

#include <vector>
#include "NodeBundle.h"

namespace psurface {

/**  \brief This class represents a node that is to be created when an edge of the target surface can be inserted.*/
template <class ctype>
class PathVertex
{
public:
    /** \brief Default Constructor. */
    PathVertex() 
    {}

    /** \brief Construct Vertex from node bundle. */
    PathVertex(const NodeBundle& bundle) :
        bundle_(bundle), enteringEdge_(-1)
    {} 

    /** \brief Full constructor. */
    PathVertex(int tri, int edge, ctype locEdge, 
               typename Node<ctype>::NodeType type, 
               const NodeBundle& bundle, 
               ctype lambda, int enteringEdge, int corner=-1) :
        tri_(tri), edge_(edge), locEdge_(locEdge),  
        corner_(corner), type_(type), bundle_(bundle),
        lambda_(lambda), enteringEdge_(enteringEdge)
    {} 

    /** \brief Assignment operator */
    PathVertex & operator=(const PathVertex& other) {
        (*this).tri_ = other.tri_; 
        (*this).edge_ = other.edge_; 
        (*this).locEdge_ = other.locEdge_; 
        (*this).corner_ = other.corner_; 
        (*this).type_ = other.type_; 
        (*this).bundle_ = other.bundle_; 
        (*this).lambda_ = other.lambda_; 
        (*this).enteringEdge_ = other.enteringEdge_; 
        return *this;
    }

    /** \brief Equality-operator. */
    bool operator==(const PathVertex& other) const {
        return (tri_==other.tri_) && (edge_ == other.edge_) && (std::fabs(locEdge_-other.locEdge_)<1e-8)
                && (corner_ == other.corner_) && (type_ == other.type_) && (bundle_==other.bundle_)
                    && (std::fabs(lambda_-other.lambda_)<1e-8) && (enteringEdge_ == other.enteringEdge_);
    }

    /** \brief Print the content for debugging */
    void print() const {
        std::cout<<"Triangle: "<<tri_<<", Edge: "<<edge_<<std::endl;
        std::cout<<" Edge coordinate "<<locEdge_<<",  Corner "<<corner_<<std::endl;
        std::cout<<"Type: "<<type_<<", lambda on ray: "<<lambda_<<std::endl;
        std::cout<<" enteringEdge "<<enteringEdge_<<std::endl;
    }

    //! The triangle the vertex lives on, for INTERSECTION nodes the first triangle is stored
    int tri_;
    //! The edge the vertex lives on or -1
    int edge_;
    //! Barycentric coordinate on the edge
    ctype locEdge_;
    //! If the vertex is a corner, store the corner idx
    int corner_; 
    //! The vertex type
    typename Node<ctype>::NodeType type_;
    //! The node bundle of the vertex
    NodeBundle bundle_;
    //! Position on the path of the target surface edge 0<=lambda<=1
    ctype lambda_;
    //! The edge from which the target edge entered the domain triangle tri_
    int enteringEdge_;
};

} // namespace psurface

#endif
