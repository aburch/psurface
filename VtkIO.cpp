#define  HAVE_AMIRAMESH
#include <vector>
#include <string.h>
#include <memory>
#include <tr1/memory>
#include <fstream>

#include "StaticVector.h"
#include "Domains.h"
#include "PSurface.h"
#include "PSurfaceFactory.h"
#include "VtkIO.h"


using namespace psurface;

  template<class ctype,int dim>
  psurface::VTKIO<ctype,dim>::VTKIO(PSurface<dim,ctype>* psurface)
  {
    par = psurface;
    //interate over element
    int i,j,k;
    numVertices  = par->getNumVertices();
    numTriangles = par->getNumTriangles();

    numNodes      = 0;
    numParamEdges = 0;
    numEdgePoints = 0;

    numNodesAndEdgesArray.resize(11*numTriangles);
    for (i=0; i<numTriangles; i++) {
        const DomainTriangle<ctype>& cT = par->triangles(i);

        int numIntersectionNodes;
        int numTouchingNodes;
        int numInteriorNodes;

        cT.countNodes(numIntersectionNodes, numTouchingNodes, numInteriorNodes);
        int numEdges = cT.getNumRegularEdges();
        numNodesAndEdgesArray[11*i+0] = numIntersectionNodes;
        numNodesAndEdgesArray[11*i+1] = numTouchingNodes;
        numNodesAndEdgesArray[11*i+2] = numInteriorNodes;
        numNodesAndEdgesArray[11*i+3] = numEdges;
        numNodesAndEdgesArray[11*i+4] = cT.patch;

        numNodesAndEdgesArray[11*i+5] = cT.edgePoints[0].size()-2;
        numNodesAndEdgesArray[11*i+6] = cT.edgePoints[1].size()-2;
        numNodesAndEdgesArray[11*i+7] = cT.edgePoints[2].size()-2;

        numNodesAndEdgesArray[11*i+8] = cT.nodes[cT.cornerNode(0)].getNodeNumber();
        numNodesAndEdgesArray[11*i+9] = cT.nodes[cT.cornerNode(1)].getNodeNumber();
        numNodesAndEdgesArray[11*i+10] = cT.nodes[cT.cornerNode(2)].getNodeNumber();

        numNodes += numIntersectionNodes;
        numNodes += numTouchingNodes;
        numNodes += numInteriorNodes;

        numEdgePoints += cT.edgePoints[0].size() + cT.edgePoints[1].size() + cT.edgePoints[2].size() - 6;

        numParamEdges += numEdges;
    }

    ncells = numTriangles + numParamEdges;
    nvertices = numVertices + numNodes;
    /////////////////////////////////////////////////////////////////////
    StaticVector<ctype,3> imagepos;

    //plane graph on each base grid triangle, saved as a list of nodes and a list of edges.
    int arrayIdx           = 0;
    int edgeArrayIdx       = 0;
    int edgePointsArrayIdx = 0;
    ctype cCoords[3][3];

    nodeType.resize(numVertices + numNodes);
    nodePositions.resize(numNodes);
    parameterEdgeArray.resize(numParamEdges);
    for(i = 0; i < numVertices; i++) nodeType[i] = Node<ctype>::CORNER_NODE;

    for (i=0; i<numTriangles; i++) {
        const DomainTriangle<ctype>& cT = par->triangles(i);

        std::vector<int> newIdx(cT.nodes.size());
        for(j = 0; j < 3; j++)
          for(k = 0; k < 3; k++)
            cCoords[j][k] = par->vertices(par->triangles(i).vertices[j])[k];

        // the cornerNode are not saved, because everything about them
        // can be deduced from the base grid
        // the three remaining types are saved separately, in order to avoid
        // explicitly saving the type for each node.
        for(size_t cN = 0; cN < 3; cN++)
        {
            if(!cT.nodes[cN].isCORNER_NODE())printf("error in the corner node indx!\n");
            newIdx[cN] = par->triangles(i).vertices[cN] - numVertices;
        }
        int localArrayIdx = 3;
        for (size_t cN=0; cN<cT.nodes.size(); cN++) {
            if (cT.nodes[cN].isINTERSECTION_NODE()){
                for(k = 0; k < 3; k++)
                (nodePositions[arrayIdx])[k] = cCoords[0][k]*cT.nodes[cN].domainPos()[0]
                                              +cCoords[1][k]*cT.nodes[cN].domainPos()[1]
                                              +cCoords[2][k]*(1 - cT.nodes[cN].domainPos()[0] - cT.nodes[cN].domainPos()[1]);
                nodeType[arrayIdx+ numVertices] = Node<ctype>::INTERSECTION_NODE;
                newIdx[cN] = arrayIdx;
                arrayIdx++;
            }
        }

        for (size_t cN=0; cN<cT.nodes.size(); cN++) {
            if (cT.nodes[cN].isTOUCHING_NODE()){
                for(k = 0; k < 3; k++)
                (nodePositions[arrayIdx])[k] = cCoords[0][k]*cT.nodes[cN].domainPos()[0]
                                              +cCoords[1][k]*cT.nodes[cN].domainPos()[1]
                                              +cCoords[2][k]*(1 - cT.nodes[cN].domainPos()[0] - cT.nodes[cN].domainPos()[1]);
                nodeType[arrayIdx+ numVertices] = Node<ctype>::TOUCHING_NODE;
                newIdx[cN] = arrayIdx;
                arrayIdx++;
            }
        }

        for (size_t cN=0; cN<cT.nodes.size(); cN++) {
            if (cT.nodes[cN].isINTERIOR_NODE()){
                for(k = 0; k < 3; k++)
                (nodePositions[arrayIdx])[k] = cCoords[0][k]*cT.nodes[cN].domainPos()[0]
                                              +cCoords[1][k]*cT.nodes[cN].domainPos()[1]
                                              +cCoords[2][k]*(1 - cT.nodes[cN].domainPos()[0] - cT.nodes[cN].domainPos()[1]);
                nodeType[arrayIdx+ numVertices] = Node<ctype>::INTERIOR_NODE;
                newIdx[cN] = arrayIdx;
                arrayIdx++;
            }
        }
        // the parameterEdges for this triangle
        typename PlaneParam<ctype>::UndirectedEdgeIterator cE;
        for (cE = cT.firstUndirectedEdge(); cE.isValid(); ++cE){
            if(cE.isRegularEdge())
            {
                parameterEdgeArray[edgeArrayIdx][0] = newIdx[cE.from()] + numVertices;
                parameterEdgeArray[edgeArrayIdx][1] = newIdx[cE.to()] + numVertices;
                edgeArrayIdx++;
            }
        }
    }
  };

  //write the psurface into vtu file
  template<class ctype,int dim>
  void psurface::VTKIO<ctype,dim>::creatVTU(const char* filename, bool basegrid)
  {
    std::ofstream file;
    file.open(filename);
    if (! file.is_open()) printf("%s does not exits!\n", filename);
    writeDataFile(file, basegrid);
    file.close();
    return;
  }

  //write data file to stream
  template<class ctype,int dim>
  void psurface::VTKIO<ctype,dim>::writeDataFile(std::ostream& s, bool basegrid)
  {
    VTK::FileType fileType = VTK::unstructuredGrid;

    VTK::VTUWriter writer(s, outputtype,fileType);//Most inportant structure used here

    if(basegrid)
      writer.beginMain(numTriangles, numVertices);
    else
      writer.beginMain(numTriangles + numParamEdges, numVertices + numNodes);

    writeAllData(writer, basegrid);
    writer.endMain();
  }

  //write the data section in vtu
  template<class ctype,int dim>
  void psurface::VTKIO<ctype,dim>::writeAllData(VTK::VTUWriter& writer, bool basegrid) {
    //PointData
    writePointData(writer,basegrid);
    // Points
    writeGridPoints(writer,basegrid);
    // Cells
    writeGridCells(writer,basegrid);
  }

  // write point data
  template<class ctype,int dim>
  void psurface::VTKIO<ctype,dim>::writePointData(VTK::VTUWriter& writer, bool basegrid)
  {
      std::string scalars = "nodetype";
      std::string vectors= "";
      int numpoints;
      if(basegrid)
          numpoints = numVertices;
      else
          numpoints = nvertices;

      writer.beginPointData(scalars, vectors);

      {
            std::tr1::shared_ptr<VTK::DataArrayWriter<ctype> > p
            (writer.makeArrayWriter<ctype>(scalars, 1, numpoints));
            for( int i = 0; i < numpoints;i++)
                p->write(nodeType[i]);
      }

      writer.endPointData();
  }

  // write the positions of vertices
  template<class ctype,int dim>
  void psurface::VTKIO<ctype,dim>::writeGridPoints(VTK::VTUWriter& writer, bool basegrid)
  {
      int numpoints;
      if(basegrid)
          numpoints = numVertices;
      else
          numpoints = nvertices;

      writer.beginPoints();
      {
            std::tr1::shared_ptr<VTK::DataArrayWriter<ctype> > p
            (writer.makeArrayWriter<ctype>("Coordinates", 3, numpoints));
            if(!p->writeIsNoop()) {
                  for(int i = 0; i < numVertices; i++)
                        for(int l = 0; l < 3; l++)
                            p->write((par->vertices(i))[l]);

                  if(!basegrid)
                  {
                        for(int i = 0; i < numNodes; i++)
                              for(int l = 0; l < 3; l++)
                                    p->write((nodePositions[i])[l]);
                  }
            }
      }
      writer.endPoints();
  }

  // write the connectivity array
  template<class ctype,int dim>
  void psurface::VTKIO<ctype,dim>::writeGridCells(VTK::VTUWriter& writer, bool basegrid)
  {
      int numconn, num_cell;
      if(basegrid)
      {
          num_cell = numTriangles;
          numconn =  3*numTriangles;
      }
      else
      {
          num_cell = ncells;
          numconn =  3*numTriangles +2*numParamEdges;
      }

      writer.beginCells();
      // connectivity
      {
          std::tr1::shared_ptr<VTK::DataArrayWriter<int> > p1
          (writer.makeArrayWriter<int>("connectivity", 1, 3*numTriangles));
          if(!p1->writeIsNoop())
          {
              for(int i = 0; i < numTriangles; i++)
                  for( int l = 0; l < 3; l++)
                      p1->write(par->triangles(i).vertices[l]);

              if(!basegrid)
              {
                  for( int i = 0; i <  parameterEdgeArray.size(); i++)
                      for(int l = 0; l < 2; l++)
                          p1->write((parameterEdgeArray[i])[l]);
              }
          }
      }

      // offsets
      {
          std::tr1::shared_ptr<VTK::DataArrayWriter<int> > p2
          (writer.makeArrayWriter<int>("offsets", 1, numTriangles));
          if(!p2->writeIsNoop()) {
              int offset = 0;
              for(int i = 0; i < numTriangles; i++)
              {
                  offset += 3;
                  p2->write(offset);
              }
              if(!basegrid)
              {
                  for(int i = 0; i < parameterEdgeArray.size(); i++)
                  {
                      offset += 2;
                      p2->write(offset);
                  }
              }
          }
      }

      // types
      {
          std::tr1::shared_ptr<VTK::DataArrayWriter<unsigned char> > p3
          (writer.makeArrayWriter<unsigned char>("types", 1, num_cell));
          if(!p3->writeIsNoop())
          {
              for(int i = 0; i < numTriangles;i++)
              p3->write(5); //vtktype of triangle
              if(!basegrid)
              {
                  for(int i = 0; i < parameterEdgeArray.size(); i++)
                      p3->write(3);//vtktype of edges
              }
          }
      }

      writer.endCells();
  }

//   Explicit template instantiations.
template class VTKIO<float,2>;
