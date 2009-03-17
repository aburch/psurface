#include <psurface/Parametrization.h>
#include <psurface/contact.h>
#include <psurface/ContactToolBox.h>
#include <psurface/IntersectionPrimitive.h>

#include <vector>

static Parametrization* cPar;
static Surface* surf1;
static Surface* surf2;

void buildContactMapping(const std::vector<double>& coords1,  ///< The vertices of the first surface as \f$x_0 ,y_0 ,z_0, x_1, y_1, z_1 ...\f$
                         const std::vector<int>& tri1,       ///< The triangles of the first surface
                         const std::vector<double>& coords2,  ///< The vertices of the second surface
                         const std::vector<int>& tri2,
                         float epsilon, void (*obsDirections)(const double* pos, double* dir))
{
    int nVert1 = coords1.size()/3;
    int nVert2 = coords2.size()/3;
    int nTri1  = tri1.size()/3;
    int nTri2  = tri2.size()/3;

    // Create a first Surface object
    surf1 = new Surface;
    
    surf1->patches.resize(1);
    surf1->patches[0] = new Surface::Patch;
    surf1->patches[0]->innerRegion = 0;
    surf1->patches[0]->outerRegion = 1;
    surf1->patches[0]->boundaryId  = 0;

    surf1->points.resize(nVert1);
    for (int i=0; i<nVert1; i++) 
        for (int j=0; j<3; j++)
            surf1->points[i][j] = coords1[3*i+j];

    surf1->triangles.resize(nTri1);
    surf1->patches[0]->triangles.resize(nTri1);
    for (int i=0; i<nTri1; i++) {

        surf1->triangles[i].points[0] = tri1[3*i];
        surf1->triangles[i].points[1] = tri1[3*i+1];
        surf1->triangles[i].points[2] = tri1[3*i+2];

        surf1->triangles[i].patch = 0;
        surf1->patches[0]->triangles[i] = i;
    }

    // Create a second Surface object
    surf2 = new Surface;
    
    surf2->patches.resize(1);
    surf2->patches[0] = new Surface::Patch;
    surf2->patches[0]->innerRegion = 0;
    surf2->patches[0]->outerRegion = 1;
    surf2->patches[0]->boundaryId  = 0;

    surf2->points.resize(nVert2);
    for (int i=0; i<nVert2; i++)
        for (int j=0; j<3; j++)
            surf2->points[i][j] = coords2[3*i+j];

    surf2->triangles.resize(nTri2);
    surf2->patches[0]->triangles.resize(nTri2);
    for (int i=0; i<nTri2; i++) {

        surf2->triangles[i].points[0] = tri2[3*i];
        surf2->triangles[i].points[1] = tri2[3*i+1];
        surf2->triangles[i].points[2] = tri2[3*i+2];

        surf2->triangles[i].patch = 0;
        surf2->patches[0]->triangles[i] = i;
    }

    // For debugging
    surf1->write("testSurf1.surf", 1);
    surf2->write("testSurf2.surf", 1);

    // Create the parametrization
    if (!cPar)
        delete cPar;

    cPar = new Parametrization;

    ContactToolBox::buildContactSurface(cPar, surf1, surf2, epsilon, obsDirections);

}


void getMergedGrid(std::vector<IntPrimitive>& overlaps)
{
    /** \todo Do without IntersectionPrimitive altogether
        because it uses mclib-stuff */
    std::vector<IntersectionPrimitive> mergedGrid;
    
    ContactToolBox::extractMergedGrid(cPar, mergedGrid);

    overlaps.resize(mergedGrid.size());

    for (size_t i=0; i<mergedGrid.size(); i++) {
        
        ((McVec3f*)overlaps[i].points)[0] = mergedGrid[i].points[0];
        ((McVec3f*)overlaps[i].points)[1] = mergedGrid[i].points[1];
        ((McVec3f*)overlaps[i].points)[2] = mergedGrid[i].points[2];
    
        overlaps[i].tris[0] = mergedGrid[i].tris[0];
        overlaps[i].tris[1] = mergedGrid[i].tris[1];
        
        for (int j=0; j<12; j++)
            overlaps[i].localCoords[0][0][j] = ((float*)&(mergedGrid[i].localCoords))[j];
        
    }

}

void deleteContactSurface()
{
    delete surf1;
    delete surf2;
    delete cPar;
}

