#ifndef HDF5IO_H
#define HDF5IO_H
#include <stdio.h>
#include <stdlib.h>

#include "psurfaceAPI.h"
//#include "psurface_convert_new.h"
namespace psurface{
template<class ctype,int dim>
class Hdf5IO{
    public:
    PSurface<dim,ctype>* par;
    std::vector<StaticVector<ctype,3> > nodePositions;
    std::vector<StaticVector<int,2> > parameterEdgeArray;
    
    int numVertices;
    int numTriangles;
    int numNodes;
    int numParamEdges;
    int numEdgePoints;
    
    int ncells;
    int nvertices;

    public:
    Hdf5IO(PSurface<dim,ctype>* psurface);

    void creatHdfAndXdmf(const std::string&  xdf_filename, const std::string&  hdf_filename, bool base);
    void writeHdf5Data(const std::string& filename);    
    void writeBaseHdf5Data(const std::string& filename);
    void readHdf5Data(const std::string&  filename);
    void writeXdmf(const std::string&  xdf_filename, const std::string&  hdf_filename);
    void initCompletePSurface(Surface* surf, const std::string&  filename);
};
}
#endif
