// -*- tab-width: 4; indent-tabs-mode: nil -*-
#ifndef DUNE_PDELAB_P0FEM_HH
#define DUNE_PDELAB_P0FEM_HH

#include <dune/common/geometrytype.hh>

#include<dune/finiteelements/p0.hh>
#include"finiteelementmap.hh"

namespace Dune {
  namespace PDELab {

	//! wrap up element from local functions
    //! \ingroup FiniteElementMap
	template<class D, class R, int d>
	class P0LocalFiniteElementMap
	  : public SimpleLocalFiniteElementMap< Dune::P0LocalFiniteElement<D,R,d> >
	{
    public:
      P0LocalFiniteElementMap (Dune::GeometryType::BasicType basicType)
        : SimpleLocalFiniteElementMap< Dune::P0LocalFiniteElement<D,R,d> >(Dune::P0LocalFiniteElement<D,R,d>(basicType))
      {
      }
    };

  }
}

#endif