#ifndef MC__SURFACE_BASE
#define MC__SURFACE_BASE

#include <vector>
#include <tr1/array>
#include <algorithm>
#include <limits>

#include "McPointerSurfaceParts.h"


/** A simple pointer-based surface.  The @c VertexType, @c EdgeType, and @c TriangleType
    classes have to be derived from McVertex, McEdge, McTriangle,
    respectively.
    
    For an example of the use of this class look at the module HxParametrization,
    which is derived from McPointerSurface, and its three constituent classes
    DomainVertex, DomainEdge, and DomainTriangle.
    @see McVertex, McEdge, McTriangle
*/
template <class VertexType, class EdgeType, class TriangleType>
class MCLIB_API McSurfaceBase {

public:

    ///
    McSurfaceBase(){}

    ///
    ~McSurfaceBase(){}

    ///
    void clear() {
        triangleArray.resize(0);
        freeTriangleStack.resize(0);
        edgeArray.resize(0);
        freeEdgeStack.resize(0);
        vertexArray.resize(0);
        freeVertexStack.resize(0);
    }

    /**@name Procedural Access to the Elements */
    //@{
    ///
    TriangleType& triangles(int i) {return triangleArray[i];}

    ///
    const TriangleType& triangles(int i) const {return triangleArray[i];}

    ///
    EdgeType& edges(int i) {return edgeArray[i];}

    ///
    const EdgeType& edges(int i) const {return edgeArray[i];}

    ///
    VertexType& vertices(int i) {return vertexArray[i];}

    ///
    const VertexType& vertices(int i) const {return vertexArray[i];}

    ///
    size_t getNumTriangles() const {
        return triangleArray.size();
    }

    ///
    size_t getNumEdges() const {
        return edgeArray.size();
    }

    ///
    size_t getNumVertices() const {
        return vertexArray.size();
    }

    //@}

    /**@name Insertion and Removal of Elements */
    //@{
    /// removes a triangle and maintains a consistent data structure
    void removeTriangle(int tri){
        int i;
        
        // update all three neighboring edges
        for (i=0; i<3; i++){
            
            int thisEdge = triangles(tri).edges[i];
            if (thisEdge==-1)
                continue;
            
            if (edges(thisEdge).triangles.size() == 1){
                // remove this edge
                removeEdge(thisEdge);
            }else {
                edges(thisEdge).removeReferenceTo(tri);           
            }
            
            triangles(tri).edges[i] = -1;
        }

        // remove triangle
        //triangles(tri).invalidate();
        freeTriangleStack.push_back(tri);
    }

    /// removes an edge
    void removeEdge(int edge){
        vertices(edges(edge).from).removeReferenceTo(edge);
        vertices(edges(edge).to).removeReferenceTo(edge);

        freeEdgeStack.push_back(edge);
    };

    void removeVertex(int vertex) {
        assert(!vertices(vertex).degree());
        freeVertexStack.push_back(vertex);
    }

    void addVertex(VertexType* vertex) {
        vertices.append(vertex);
    }


    int newVertex(const McVec3f& p) {

        if (freeVertexStack.size()){
            int newVertexIdx = freeVertexStack.back();
            freeVertexStack.pop_back();
            vertices(newVertexIdx) = p;
            return newVertexIdx;
        } 

        vertexArray.push_back(p);
        return vertexArray.size()-1;
    }

    int newEdge(int a, int b) {

        int newEdgeIdx;
        if (freeEdgeStack.size()) {
            newEdgeIdx = freeEdgeStack.back();
            freeEdgeStack.pop_back();
        } else {
            edgeArray.push_back(EdgeType());
            newEdgeIdx = edgeArray.size()-1;
        }

        EdgeType& newEdge = edges(newEdgeIdx);

        newEdge.from = a;
        newEdge.to   = b;

        newEdge.triangles.resize(0);
        
        return newEdgeIdx;
    }

    int createSpaceForTriangle(int a, int b, int c) {
        int newTri;
        if (freeTriangleStack.size()) {
            newTri = freeTriangleStack.back();
            freeTriangleStack.pop_back();
        } else {
            triangleArray.push_back(TriangleType(a,b,c));
            newTri = triangleArray.size()-1;
        }

        return newTri;
    }

    void integrateTriangle(int triIdx) {

        TriangleType& tri = triangles(triIdx);
        
        // look for and possible create the new edges
        for (int i=0; i<3; i++){
            
            int pointA = tri.vertices[i];
            int pointB = tri.vertices[(i+1)%3];
            
            int thisEdge = findEdge(pointA, pointB);
            
            if (thisEdge == -1){
                
                // this edge doesn't exist yet
                int newEdgeIdx = newEdge(pointA, pointB);
                
                vertices(pointA).edges.push_back(newEdgeIdx);
                vertices(pointB).edges.push_back(newEdgeIdx);
                
                edges(newEdgeIdx).triangles.append(triIdx);
                tri.edges[i] = newEdgeIdx;
            }
            else{
                if (!edges(thisEdge).isConnectedToTriangle(triIdx))
                    edges(thisEdge).triangles.append(triIdx);

                tri.edges[i] = thisEdge;
            }
        }
    }
    //@}

    /**@name Topological Queries */
    //@{

    /** Given two vertices, this routine returns the edge that connects them,
        or -1, if no such edge exists. */
    int findEdge(int a, int b) const {
        assert(a>=0 && a<getNumVertices() && b>=0 && b<getNumVertices());
        for (int i=0; i<vertices(a).degree(); i++)
            if (edges(vertices(a).edges[i]).from == b ||
                edges(vertices(a).edges[i]).to   == b)
                return vertices(a).edges[i];
        
        return -1;
    }

    /** Given three vertices, this routine returns the triangle that connects them,
        or -1, if no such triangle exists. */
    int findTriangle(int a, int b, int c) const {
        assert(a>=0 && a<getNumVertices());
        assert(b>=0 && b<getNumVertices());
        assert(c>=0 && c<getNumVertices());

        int oneEdge = findEdge(a, b);
        
        if (oneEdge==-1)
            return -1;
        for (int i=0; i<edges(oneEdge).triangles.size(); i++)
            if (triangles(edges(oneEdge).triangles[i]).isConnectedTo(c)){
                assert(edges(oneEdge).triangles[i]<getNumTriangles());
                return edges(oneEdge).triangles[i];
            }
        return -1;
    }

    /// Tests whether the two edges are connected by a common triangle
    int findCommonTriangle(int a, int b) const {
        assert(a!=b);
        assert(a>=0 && a<getNumVertices());
        assert(b>=0 && b<getNumVertices());

        for (int i=0; i<edges(a).triangles.size(); i++)
            for (int j=0; j<edges(b).triangles.size(); j++)
                if (edges(a).triangles[i] == edges(b).triangles[j])
                    return edges(a).triangles[i];
        
        return -1;
    }

    /// 
    McSmallArray<int, 12> getTrianglesPerVertex(int v) const {

        const VertexType& cV = vertices(v);

        McSmallArray<int, 12> result;
        
        for (int i=0; i<cV.edges.size(); i++) {

            const EdgeType& cE = edges(cV.edges[i]);

            for (int j=0; j<cE.triangles.size(); j++) {

                if (result.findSorted(cE.triangles[j], &mcStandardCompare)==-1)
                    result.insertSorted(cE.triangles[j], &mcStandardCompare);
                
            }

        }

        return result;
    }

    ///
    McSmallArray<int, 12> getNeighbors(int v) const {

        const VertexType& cV = vertices(v);
        McSmallArray<int, 12> result;
        
        for (int i=0; i<cV.edges.size(); i++) {
            
            const EdgeType& cE = edges(cV.edges[i]);
            result.append(cE.theOtherVertex(v));

        }

        return result;
    }

    int getNeighboringTriangle(int tri, int side) const {
        assert(side>=0 && side<3);
        int neighboringTri = -1;
        int cE = triangles(tri).edges[side];
                
        if (edges(cE).triangles.size()==2) {
            neighboringTri = (edges(cE).triangles[0]==tri) 
                ? edges(cE).triangles[1]
                : edges(cE).triangles[0];
        }
        return neighboringTri;
    }
    
    //@}

    /**@name Geometrical Queries */
    //@{

    /// gives the smallest interior angle of a triangle
    float minInteriorAngle(int n) const {
        float minAngle = 2*M_PI;
        const McSArray<int, 3>& p = triangles(n).vertices;

        for (int i=0; i<3; i++){
            McVec3f a = vertices(p[(i+1)%3]) - vertices(p[i]);
            McVec3f b = vertices(p[(i+2)%3]) - vertices(p[i]);

            float angle = acosf(a.dot(b) / (a.length() * b.length()));
            if (angle<minAngle)
                minAngle = angle;
        }

        return minAngle;
    }

    /// returns the aspect ratio
    float aspectRatio(int n) const {

        const McSArray<int, 3>& p = triangles(n).vertices;

        const float a = (vertices(p[1]) - vertices(p[0])).length();
        const float b = (vertices(p[2]) - vertices(p[1])).length();
        const float c = (vertices(p[0]) - vertices(p[2])).length();

        const float aR = 2*a*b*c/((-a+b+c)*(a-b+c)*(a+b-c));

        // might be negative due to inexact arithmetic
        return fabs(aR);
    }

        /// returns the normal vector
    McVec3f normal(int tri) const {
        const McVec3f a = vertices(triangles(tri).vertices[1]) - vertices(triangles(tri).vertices[0]);
        const McVec3f b = vertices(triangles(tri).vertices[2]) - vertices(triangles(tri).vertices[0]);
        McVec3f n = a.cross(b);
        n.normalize();
        return n;
    }

    ///
    float smallestDihedralAngle(int edge) const {
        float minAngle = std::numeric_limits<float>::max();
        for (int i=0; i<edges(edge).triangles.size(); i++)
            for (int j=i+1; j<edges(edge).triangles.size(); j++)
                minAngle = std::min(minAngle,dihedralAngle(edges(edge).triangles[i], edges(edge).triangles[j]));

        return minAngle;
    }

    /// gives the surface area
    float area(int tri) const { 
        McVec3f a = vertices(triangles(tri).vertices[1]) - vertices(triangles(tri).vertices[0]);
        McVec3f b = vertices(triangles(tri).vertices[2]) - vertices(triangles(tri).vertices[0]);

        return fabs(0.5 * (a.cross(b)).length());
    }

    /// gives the dihedral angle with a neighboring triangle
    float dihedralAngle(int first, int second) const {
        McVec3f n1 = normal(first);
        McVec3f n2 = normal(second);

        float scalProd = n1.dot(n2);
        if (scalProd < -1) scalProd = -1;
        if (scalProd >  1) scalProd =  1;

        return (triangles(first).isCorrectlyOriented(triangles(second))) ? acos(-scalProd) : acos(scalProd);
    }

    float length(int e) const {
        return (vertices(edges(e).from) - vertices(edges(e).to)).length();
    }

    //@}

     /**@name Intersection Tests */
    //@{

    /** Tests whether this triangle intersects the given edge.  This routine is not
        faster than the one that returns the intersection point. */
    bool intersectionTriangleEdge(int tri, 
                                  const McEdge<VertexType> *edge,
                                  float eps=0) const {
        bool parallel;
        McVec3f where;
        return intersectionTriangleEdge(tri, edge, where, parallel, eps);
    }

    /** Tests whether this triangle intersects the given edge, and returns the intersection
        point if there is one. If not, the variable @c where is untouched. */
    bool intersectionTriangleEdge(int tri, 
                                  const McEdge<VertexType> *edge, 
                                  McVec3f& where, 
                                  bool& parallel, float eps=0) const{

        const TriangleType& cT = triangles(tri);

        const McVec3f &p = vertices(edge->from);
        const McVec3f &q = vertices(edge->to);
        const McVec3f &a = vertices(cT.vertices[0]);
        const McVec3f &b = vertices(cT.vertices[1]);
        const McVec3f &c = vertices(cT.vertices[2]);

        // Cramer's rule
        float det = McMat3f(b-a, c-a, p-q).det();
        if (det<-eps || det>eps){
            
            // triangle and edge are not parallel
            parallel = false;

            float nu = McMat3f(b-a, c-a, p-a).det() / det;
            if (nu<-eps || nu>1+eps) return false;

            float lambda = McMat3f(p-a, c-a, p-q).det() / det;
            if (lambda<-eps) return false;

            float mu = McMat3f(b-a, p-a, p-q).det() / det;
            if (mu<-eps) return false;

            if (lambda+mu > 1+eps) 
                return false;
            else {
                where = p + nu*(q-p);
                return true;
            }

        } else {

            // triangle and edge are parallel
            parallel = true;
            
            float alpha = McMat3f(b-a, c-a, p-a).det();
            if (alpha<-eps || alpha>eps)
                return false;
            else {

                int i;

                // 2D intersection test

                // project onto the coordinate plane that is 'most parallel' to the triangle
                McVec3f normal = (b-a).cross(c-a);

                McVec2f a2D, b2D, c2D, p2D, q2D;

                for (i=0; i<3; i++) 
                    if (normal[i]<0)
                        normal[i] = -normal[i];

                for (i=0; i<3; i++)
                    if (normal[i]>=normal[(i+1)%3] && normal[i]>=normal[(i+2)%3]) {

                        a2D = McVec2f(a[(i+1)%3], a[(i+2)%3]);
                        b2D = McVec2f(b[(i+1)%3], b[(i+2)%3]);
                        c2D = McVec2f(c[(i+1)%3], c[(i+2)%3]);
                        p2D = McVec2f(p[(i+1)%3], p[(i+2)%3]);
                        q2D = McVec2f(q[(i+1)%3], q[(i+2)%3]);

                    }

                if (p2D.isInTriangle(a2D, b2D, c2D, eps) ||
                    q2D.isInTriangle(a2D, b2D, c2D, eps))
                    return true;
                else
                    return (lineIntersection2D(p2D, q2D, a2D, b2D, eps) ||
                            lineIntersection2D(p2D, q2D, b2D, c2D, eps) ||
                            lineIntersection2D(p2D, q2D, a2D, c2D, eps));

            }
                
        }

    }
        
protected:
    static bool lineIntersection2D(const McVec2f &p1, const McVec2f &p2, const McVec2f &p3, const McVec2f &p4, float eps=0) {
        const McVec2f A = p2 - p1;
        const McVec2f B = p3 - p4;
        const McVec2f C = p1 - p3;
        
        float det = A.y*B.x - A.x*B.y;

        // 1D intersection
        if (det>=-eps && det<=eps)
            return (  ((p3-p1).length() + (p3-p2).length()) / (p1-p2).length() < 1+eps ||
                      ((p4-p1).length() + (p4-p2).length()) / (p1-p2).length() < 1+eps ||
                      ((p2-p3).length() + (p2-p4).length()) / (p3-p4).length() < 1+eps ||
                      ((p1-p3).length() + (p1-p4).length()) / (p3-p4).length() < 1+eps   );
        
        float mu     = (A.x*C.y - A.y*C.x) / det;
        float lambda = (B.y*C.x - B.x*C.y) / det;

        return (mu>-eps && mu<1+eps && lambda>-eps && lambda<1+eps);
    }

public:
    /// 
    bool intersectionTriangleBox(int n, const Box<std::tr1::array<float,3>, 3>& box) {
        return box.contains(vertices(triangles(n).vertices[0])) ||
            box.contains(vertices(triangles(n).vertices[1])) ||
            box.contains(vertices(triangles(n).vertices[2]));
    }

    //@}

public:
    /// \todo The copying could be sped up considerably...
    void garbageCollection()
    {
        int i, j;
#ifndef NDEBUG
        printf("This is the SurfaceBase garbage collection...\n");
        printf("%ld vertices, %ld edges, %ld triangles removed\n",
               freeVertexStack.size(), freeEdgeStack.size(), freeTriangleStack.size());
#endif

        std::vector<bool> isInvalid;

        // clean up vertices
        if (freeVertexStack.size()) {
            
            int offset = 0;

            std::vector<int> vertexOffsets(vertexArray.size());
            isInvalid.resize(vertexArray.size());

            for (i=0; i<isInvalid.size(); i++)
                isInvalid[i] = false;

            for (i=0; i<freeVertexStack.size(); i++)
                isInvalid[freeVertexStack[i]] = true;

            for (i=0; i<vertexArray.size(); i++){
                vertexOffsets[i] = offset;
                
                if (isInvalid[i]) 
                    offset++;
            }

            ////////////////////
            for (i=0; i<vertexOffsets.size(); i++)
                vertexArray[i-vertexOffsets[i]] = vertexArray[i];
            
            vertexArray.resize(vertexArray.size()-offset);
            
            
            // Adjust edges
            for (i=0; i<edgeArray.size(); i++) {
                edgeArray[i].from -= vertexOffsets[edgeArray[i].from];
                edgeArray[i].to   -= vertexOffsets[edgeArray[i].to];
            }                

            // Adjust triangles
            for (i=0; i<triangleArray.size(); i++) 
                for (j=0; j<3; j++) 
                    triangleArray[i].vertices[j] -= vertexOffsets[triangleArray[i].vertices[j]];

            freeVertexStack.resize(0);                
        }

        // clean up edges
        if (freeEdgeStack.size()) {
            
            int offset = 0;

            std::vector<int> edgeOffsets(edgeArray.size());
            isInvalid.resize(edgeArray.size());

            for (i=0; i<isInvalid.size(); i++)
                isInvalid[i] = false;
            
            for (i=0; i<freeEdgeStack.size(); i++)
                isInvalid[freeEdgeStack[i]] = true;
            
            for (i=0; i<edgeArray.size(); i++){
                edgeOffsets[i] = offset;
                
                if (isInvalid[i]) 
                    offset++;
            }
            //printf("1.3\n");
            ////////////////////
            for (i=0; i<edgeOffsets.size(); i++)
                edgeArray[i-edgeOffsets[i]] = edgeArray[i];
            
            edgeArray.resize(edgeArray.size()-offset);
            
            // Adjust vertices
            for (i=0; i<vertexArray.size(); i++) 
                for (j=0; j<vertexArray[i].degree(); j++)
                    vertexArray[i].edges[j] -= edgeOffsets[vertexArray[i].edges[j]];
            
            // Adjust triangles
            for (i=0; i<triangleArray.size(); i++) 
                for (j=0; j<3; j++) 
                    if (triangleArray[i].edges[j]>=0)
                        triangleArray[i].edges[j] -= edgeOffsets[triangleArray[i].edges[j]];

            freeEdgeStack.resize(0);
        }

        // clean up triangles
        if (freeTriangleStack.size()) {
            
            int offset = 0;

            std::vector<int> triangleOffsets(triangleArray.size());
            isInvalid.resize(triangleArray.size());

            for (i=0; i<isInvalid.size(); i++)
                isInvalid[i] = false;

            for (i=0; i<freeTriangleStack.size(); i++)
                isInvalid[freeTriangleStack[i]] = true;

            for (i=0; i<triangleArray.size(); i++){
                triangleOffsets[i] = offset;
                
                if (isInvalid[i]) 
                    offset++;
            }

            ////////////////////
            for (i=0; i<triangleOffsets.size(); i++)
                triangleArray[i-triangleOffsets[i]] = triangleArray[i];
            
            triangleArray.resize(triangleArray.size()-offset);
            
            
            // Adjust edges
            for (i=0; i<edgeArray.size(); i++) 
                for (j=0; j<edgeArray[i].triangles.size(); j++) 
                    edgeArray[i].triangles[j] -= triangleOffsets[edgeArray[i].triangles[j]];
                
            freeTriangleStack.resize(0);
        }


#ifndef NDEBUG
        printf("   ...Garbage collection finished!\n");
#endif
    }
    
protected:
    ///
    std::vector<TriangleType> triangleArray;

    ///
    std::vector<VertexType> vertexArray;

    ///
    std::vector<EdgeType> edgeArray;

public:
    ///
    std::vector<int> freeTriangleStack;
protected:
    ///
    std::vector<int>     freeEdgeStack;

    ///
    std::vector<int>  freeVertexStack;

};

#endif