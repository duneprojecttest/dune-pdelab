// -*- tab-width: 4; indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include "config.h"     
#endif

#include <string>
#include <sstream>

#include <dune/common/fvector.hh>
#include <dune/common/mpihelper.hh>
#include <dune/common/smartpointer.hh>

#include <dune/grid/io/file/vtk/subsamplingvtkwriter.hh>
#ifdef HAVE_ALBERTA
#include <dune/grid/albertagrid.hh>
#endif
#ifdef HAVE_ALUGRID
#include <dune/grid/alugrid.hh>
#endif
#ifdef HAVE_UG
#include <dune/grid/uggrid.hh>
#endif

#include "../common/vtkexport.hh"
#include "../gridfunctionspace/gridfunctionspace.hh"
#include "../gridfunctionspace/gridfunctionspaceutilities.hh"
#include "../finiteelementmap/edges03dfem.hh"

#include "gridexamples.hh"

template<typename GV>
void edgeS03DGridFunctionSpace (const GV& gv, const std::string &suffix = "")
{
  typedef typename GV::Grid::ctype D; // domain type
  typedef double R;                   // range type

  std::ostringstream filename;
  filename << "edges03dgridfunctionspace";
  if(suffix != "") filename << "-" << suffix;

  Dune::PDELab::EdgeS03DLocalFiniteElementMap<GV,R> fem(gv);   // maps entity to finite element

  typedef Dune::PDELab::GridFunctionSpace<
    GV,
    Dune::PDELab::EdgeS03DLocalFiniteElementMap<GV,R>
    > GFS;
  GFS gfs(gv,fem);                    // make grid function space

  typedef typename GFS::template VectorContainer<R>::Type X;
  std::vector<Dune::SmartPointer<X> > x(gfs.globalSize(),0);

  typedef Dune::PDELab::DiscreteGridFunctionGlobal<GFS,X> DGF;
  std::vector<Dune::SmartPointer<DGF> > dgf(gfs.globalSize(),0);

  typedef Dune::PDELab::DiscreteGridFunctionGlobalCurl<GFS,X> CurlGF;
  std::vector<Dune::SmartPointer<CurlGF> > curlgf(gfs.globalSize(),0);

  for(unsigned int i = 0; i < gfs.globalSize(); ++i) {
    x[i] = new X(gfs,0.0);
    (*x[i])[i] = 1.0;
    dgf[i] = new DGF(gfs,*x[i]);            // make a grid function
    curlgf[i] = new CurlGF(gfs,*x[i]);      // make a grid function of the curl
  }

  Dune::SubsamplingVTKWriter<GV> vtkwriter(gv,3);  // plot result
  for(unsigned int i = 0; i < gfs.globalSize(); ++i) {
    std::ostringstream num;
    num << i;
    vtkwriter.addVertexData(new Dune::PDELab::VTKGridFunctionAdapter<DGF>(*dgf[i],"edges03d_"+num.str()));
    vtkwriter.addVertexData(new Dune::PDELab::VTKGridFunctionAdapter<CurlGF>(*curlgf[i],"curl_edges03d_"+num.str()));
  }
  vtkwriter.write(filename.str(),Dune::VTKOptions::ascii);
}

template<typename Grid>
void test(Dune::SmartPointer<Grid> grid, int &result, std::string name = "", unsigned int refine = 0)
{
  grid->globalRefine(refine);

  if(name == "") name = grid->name();

  edgeS03DGridFunctionSpace(grid->leafView(), name);
  result = 0;
}

int main(int argc, char** argv)
{
  try{
    //Maybe initialize Mpi
    Dune::MPIHelper& helper = Dune::MPIHelper::instance(argc, argv);

    // default exitcode 77 (=skipped); returned in case none of the supported
    // Grids were found
    int result = 77;

#ifdef HAVE_ALBERTA
#if (ALBERTA_DIM != 3)
#error ALBERTA_DIM is not set to 3 -- please check the Makefile.am
#endif
    test(UnitTetrahedronMaker         <Dune::AlbertaGrid<3, 3>    >::create(),
         result, "alberta-tetrahedron", 0);
    test(KuhnTriangulatedUnitCubeMaker<Dune::AlbertaGrid<3, 3>    >::create(),
         result, "alberta-cube",        0);
#endif

#ifdef HAVE_ALUGRID
    test(UnitTetrahedronMaker         <Dune::ALUSimplexGrid<3, 3> >::create(),
         result, "alu-tetrahedron",     0);
    test(TriangulatedUnitCubeMaker    <Dune::ALUSimplexGrid<3, 3> >::create(),
         result, "alu-cube",            0);
#endif // HAVE_ALUGRID

#ifdef HAVE_UG
    test(UnitTetrahedronMaker         <Dune::UGGrid<3>            >::create(),
         result, "ug-tetrahedron",      0);
    test(TriangulatedUnitCubeMaker    <Dune::UGGrid<3>            >::create(),
         result, "ug-cube",             0);
#endif // HAVE_ALBERTA

    return result;
  }
  catch (Dune::Exception &e){
    std::cerr << "Dune reported error: " << e << std::endl;
    return 1;
  }
  catch (...){
    std::cerr << "Unknown exception thrown!" << std::endl;
    return 1;
  }
}