#include <tr1/array>
#include <vector>
#include <set>

#include <psurface/ContactToolBox.h>
#include <psurface/ContactBoundary.h>
#include <psurface/NormalProjector.h>

#include <mclib/McSArray.h>

#include <psurface/MultiDimOctree.h>
#include <psurface/PointIntersectionFunctor.h>

void ContactToolBox::buildContactSurface(Parametrization* cPar, 
                                         const Surface* surf1,  const Surface* surf2,
                                         float epsilon, 
                                         void (*obsDirections)(const double* pos, double* dir))
{
    int i;
    
    // set up parametrization
    cPar->surface = const_cast<Surface*>(surf2);
    cPar->patches.resize(1);
    cPar->patches[0].innerRegion = 0;
    cPar->patches[0].outerRegion = 1;
    cPar->patches[0].boundaryId  = 0;
            
    // ///////
    McSArray<ContactBoundary, 2> contactBoundary;
    
    const_cast<Surface*>(surf1)->removeUnusedPoints();
    const_cast<Surface*>(surf2)->removeUnusedPoints();
    
    contactBoundary[0].init(surf1);
    contactBoundary[1].init(surf2);
    
    contactOracle(surf1, surf2, contactBoundary[0].vertices, contactBoundary[1].vertices, epsilon);
    
    if (!contactBoundary[0].vertices.size()) {
        
        printf("No contact surface found!\n");
        return;
        
    }
    
    printf("%ld resp. %ld contact nodes found!\n", contactBoundary[0].vertices.size(),
           contactBoundary[1].vertices.size());


    // //////////////////
    // BEGIN DEBUG
    // //////////////////
#if 0
    char nodesName[50];
    // display nodes
    sprintf(nodesName, "Poly.0.nodes");
    HxCluster* theNodes = new HxCluster();
    theNodes->setLabel(nodesName);

    theNodes->points.resize(contactBoundary[0].vertices.size());
    theNodes->ids.resize(contactBoundary[0].vertices.size());

    theNodes->dataColumns.resize(0);

    for (i=0; i<contactBoundary[0].vertices.size(); i++) {
        theNodes->points[i] = McVec3f(contactBoundary[0].surf->points[contactBoundary[0].vertices[i]].x,
                                      contactBoundary[0].surf->points[contactBoundary[0].vertices[i]].y,
                                      contactBoundary[0].surf->points[contactBoundary[0].vertices[i]].z);

        theNodes->ids[i] = 0;
    }

    theNodes->writePSI(nodesName);
    theObjectPool->addObject(theNodes);

    // display nodes
    sprintf(nodesName, "Poly.1.nodes");
    theNodes = new HxCluster();
    theNodes->setLabel(nodesName);

    theNodes->points.resize(contactBoundary[1].vertices.size());
    theNodes->ids.resize(contactBoundary[1].vertices.size());

    theNodes->dataColumns.resize(0);

    for (i=0; i<contactBoundary[1].vertices.size(); i++) {
        theNodes->points[i] = McVec3f(contactBoundary[1].surf->points[contactBoundary[1].vertices[i]].x,
                                      contactBoundary[1].surf->points[contactBoundary[1].vertices[i]].y,
                                      contactBoundary[1].surf->points[contactBoundary[1].vertices[i]].z);

        theNodes->ids[i] = 0;
    }

    theNodes->writePSI(nodesName);
    theObjectPool->addObject(theNodes);
#endif
    // ///////////////////////////////////
    // END DEBUG
    // //////////////////////////////////

    // create the two contact patches
    
    computeContactPatch(surf1, contactBoundary[0]);
    computeContactPatch(surf2, contactBoundary[1]);
    
    std::cout << "Contact patches contain " << contactBoundary[0].triIdx.size() 
              << " (resp. " << contactBoundary[1].triIdx.size() << ") triangles." << std::endl;
    
    // the nonmortar side becomes the base grid of the parametrization
    for (i=0; i<contactBoundary[0].vertices.size(); i++)
        cPar->newVertex(surf1->points[contactBoundary[0].vertices[i]]);
    
    cPar->params->insert(new HxParameter("targetTris",
                                         contactBoundary[0].triIdx.size(),
                                         &contactBoundary[0].triIdx[0]));
    
    std::vector<int> vertexOffsets = contactBoundary[0].getVertexOffsets();
    for (i=0; i<contactBoundary[0].triIdx.size(); i++) {
        
        int newTri = cPar->createSpaceForTriangle(vertexOffsets[contactBoundary[0].triangles(i).points[0]],
                                                  vertexOffsets[contactBoundary[0].triangles(i).points[1]],
                                                  vertexOffsets[contactBoundary[0].triangles(i).points[2]]);
        cPar->integrateTriangle(newTri);
        cPar->triangles(newTri).patch = 0;
        
    }
    
    // compute projection
    NormalProjector* projector = new NormalProjector;
    
    projector->handleSide(cPar, contactBoundary[1], obsDirections);
    
    delete projector;

}


void ContactToolBox::contactOracle(const Surface* surf1, const Surface* surf2,
                                   std::vector<int>& contactNodes1, std::vector<int>& contactNodes2,
                                   float epsilon)
{
    int i, j;
    const float epsSquared = epsilon*epsilon;
    
    Box<std::tr1::array<float,3>,3> bbox1, bbox2;

    // stupid hack: The Amira Surface class wants a float* as the bounding box
    float bbox1_raw[6], bbox2_raw[6];
    surf1->getBoundingBox(bbox1_raw);
    surf2->getBoundingBox(bbox2_raw);

    std::tr1::array<float,3> lower1, upper1, lower2, upper2;

    for (i=0; i<3; i++) {
        lower1[i] = bbox1_raw[2*i];
        upper1[i] = bbox1_raw[2*i+1];

        lower2[i] = bbox2_raw[2*i];
        upper2[i] = bbox2_raw[2*i+1];
    }

    bbox1.set(lower1, upper1);
    bbox2.set(lower2, upper2);

    bbox1.extendByEps(epsilon);
    bbox2.extendByEps(epsilon);

    // The possible contact patches must be in intersectBox
    Box<std::tr1::array<float,3>,3> intersectBox = bbox1.intersectWith(bbox2);

    // We first put the vertices of surface1 into an octree
    std::tr1::array<float,3> lower, upper;
    lower = bbox1.lower();
    upper = bbox1.upper();

    Box<std::tr1::array<float,3>, 3> mdBBox1(lower, upper);
    MultiDimOctree<McVec3f, PointIntersectionFunctor, std::tr1::array<float,3>, 3, true> mdOctree1(mdBBox1);
    PointIntersectionFunctor intersectionFunctor;

    std::vector<McVec3f> points1(surf1->points.size());        

    for (int i=0; i<surf1->points.size(); i++) {
        points1[i] = surf1->points[i];
        mdOctree1.insert(&points1[i], &intersectionFunctor);
    }

    /** \todo Don't hand over a pointer here */
    mdOctree1.enableUniqueLookup(points1.size(), &points1[0]);
    
    // We first put the vertices of surface2 into an octree
    lower = intersectBox.lower();
    upper = intersectBox.upper();

    Box<std::tr1::array<float,3>, 3> mdIntersectBox(lower, upper);
    MultiDimOctree<McVec3f, PointIntersectionFunctor, std::tr1::array<float,3>, 3, true> mdOctree2(mdIntersectBox);

    std::vector<McVec3f> points2(surf2->points.size());        

    for (int i=0; i<surf2->points.size(); i++){
        
        points2[i] = surf2->points[i];
        if (intersectBox.contains(surf2->points[i]))
            mdOctree2.insert(&points2[i], &intersectionFunctor);
        
    }
    
    /** \todo Don't hand over a pointer here */
    mdOctree2.enableUniqueLookup(points2.size(), &points2[0]);
    
    // Two bitfields to mark the contact nodes
    std::vector<bool> contactField2(surf2->points.size(), false);
    
    // ///////////////////////////////////////////////////////////////////////
    //   Completely keep the domain (nonmortar) side, and make
    //   each vertex on the mortar side be part of the contact surface if it
    //   is less than epsilon away from any nonmortar triangle.
    // ///////////////////////////////////////////////////////////////////////


    // ///////////////////////////////////////////////////////////////////////
    //   Loop over all vertices in surface2 and check whether it is no more
    //   than epsilon away from surface1
    // ///////////////////////////////////////////////////////////////////////

    //  Loop over all triangles in surface1
    for (int i=0; i<surf1->triangles.size(); i++) {

        const McVec3f& p0 = surf1->points[surf1->triangles[i].points[0]];
        const McVec3f& p1 = surf1->points[surf1->triangles[i].points[1]];
        const McVec3f& p2 = surf1->points[surf1->triangles[i].points[2]];

        //  Look up the octree for points in a conservative neighborhood
        //  of the triangle.  The triangle's boundingbox + epsilon will do
        std::vector<int> result;
        Box<std::tr1::array<float,3>, 3> mdQueryBox(p0,p1);
        mdQueryBox.extendBy(p2);
        mdQueryBox.extendByEps(epsilon);
        mdOctree2.lookupIndex(mdQueryBox, result);

        for (j=0; j<result.size(); j++) {

            // Don't recompute everything if the vertex is already marked
            if (contactField2[result[j]])
                continue;

            const McVec3f& candidatePoint = surf2->points[result[j]];

            McVec3f q = getClosestPointOnTriangle(p0, p1, p2, candidatePoint);

            if ( (q-candidatePoint).length2() < epsSquared)
                contactField2[result[j]] = true;

        }
        
    }

    //  Loop over all triangles in surface2
    for (int i=0; i<surf2->triangles.size(); i++) {

        const McVec3f& p0 = surf2->points[surf2->triangles[i].points[0]];
        const McVec3f& p1 = surf2->points[surf2->triangles[i].points[1]];
        const McVec3f& p2 = surf2->points[surf2->triangles[i].points[2]];

        //  Look up the octree for points in a conversative neighborhood
        //  of the triangle.  The triangle's boundingbox + epsilon will do
        std::vector<int> result;
        Box<std::tr1::array<float,3>, 3> mdQueryBox(p0,p1);
        mdQueryBox.extendBy(p2);
        mdQueryBox.extendByEps(epsilon);
        mdOctree1.lookupIndex(mdQueryBox, result);

        // If the bounding box contains any vertices from surface one we keep
        // the whole triangle
        if (result.size() > 0) 
            for (int j=0; j<3; j++)
                contactField2[surf2->triangles[i].points[j]] = true;

    }



    //  All vertices of surface1 belong to the result contact surface

    int c = 0;

    contactNodes1.resize(surf1->points.size());
    for (i=0; i<surf1->points.size(); i++)
        contactNodes1[c++] = i;

    c = 0;
    int nSetBits2 = 0;
    for (i=0; i<contactField2.size(); i++)
        if (contactField2[i])
            nSetBits2++;

    contactNodes2.resize(nSetBits2);
    for (i=0; i<contactField2.size(); i++)
        if (contactField2[i])
            contactNodes2[c++] = i;

}

McVec3f ContactToolBox::getClosestPointOnTriangle(const McVec3f& p0,
                                                  const McVec3f& p1,
                                                  const McVec3f& p2,          
                                                  const McVec3f& candidate)
{
        
    // local base
    McVec3f a = p1 - p0;
    McVec3f b = p2 - p0;
    McVec3f c = a.cross(b);
    c.normalize();
        
    McVec3f x = candidate - p0;
        
    // write x in the new base  (Cramer's rule)
    //McMat3f numerator(a, b, c);
    double denominatorDet = McMat3f(a, b, c).det();
    McMat3f alphaMat(x, b, c);
    McMat3f betaMat(a, x, c);
    McMat3f gammaMat(a, b, x);
    
    float alpha = alphaMat.det()/denominatorDet;
    float beta  = betaMat.det()/denominatorDet;
    //float gamma = gammaMat.det()/denominatorDet;
    
    // check whether orthogonal projection onto the ab plane is in triangle
    if (alpha>=0 && beta>=0 && (1-alpha-beta)>=0) {
        // The orthogonal projection of the candidate point on the plane
        // spanned by the triangle
        return p0 + alpha*a + beta*b;
    }
    
    // ////////////////////////////////////////////////////////////////////////////////
    //   The candidate point is not 'over' the triangle.  Then the close point to it
    //   _on_ the triangle is the closest point on the boundary of the triangle.
    // ////////////////////////////////////////////////////////////////////////////////

    float bestDist = std::numeric_limits<float>::max();
    McVec3f bestPoint;

    // I need the points in an array
    McVec3f points[3];
    points[0] = p0;
    points[1] = p1;
    points[2] = p2;
    
    // check point against edges
    for (int i=0; i<3; i++){
        
        McVec3f from = points[i];
        McVec3f to   = points[(i+1)%3];
        
        McVec3f edge = to - from;
        
        float projectLength = edge.dot(candidate - from)/edge.length();
        McVec3f projection = edge/edge.length() * projectLength;
        
        float orthoDist = ((candidate-from) - projection).length();
        
        if (projectLength>=0 && projectLength<=edge.length() && orthoDist<bestDist) {
            bestDist = orthoDist;
            bestPoint = projection + from;
        }
    }
    
    // check point against vertices
    for (int i=0; i<3; i++){
        float dist = (candidate - points[i]).length();
        if (dist < bestDist){
            bestDist = dist;
            bestPoint = points[i];
        }
    }
    
    
    return bestPoint;
}


void ContactToolBox::computeContactPatch(const Surface* surf, ContactBoundary& cBound)
{
    //////////////////////////////////////////////////
    // create triangles
    //////////////////////////////////////////////////
    //cBound.vertices.sort(&mcStandardCompare);

    /** \todo Maybe cbound.vertices can be a std::set?
        Then we wouldn't have to copy
    */
    std::set<int> vertexSet;
    for (int i=0; i<cBound.vertices.size(); i++)
        vertexSet.insert(cBound.vertices[i]);

    for (int i=0; i<surf->triangles.size(); i++) {
        
        const McVec3i& p = surf->triangles[i].points;
        
//         if (cBound.vertices.findSorted(p[0], mcStandardCompare)>=0 &&
//             cBound.vertices.findSorted(p[1], mcStandardCompare)>=0 &&
//             cBound.vertices.findSorted(p[2], mcStandardCompare)>=0) {
        if (vertexSet.find(p[0]) != vertexSet.end() &&
            vertexSet.find(p[1]) != vertexSet.end() &&
            vertexSet.find(p[1]) != vertexSet.end()) {
            
            cBound.triIdx.push_back(i);
        }
    }
}