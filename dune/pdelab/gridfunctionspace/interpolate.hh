// -*- tab-width: 4; indent-tabs-mode: nil -*-
#ifndef DUNE_PDELAB_INTERPOLATE_HH
#define DUNE_PDELAB_INTERPOLATE_HH

#include<vector>

#include<dune/common/exceptions.hh>
#include<dune/grid/common/referenceelements.hh>

#include"../common/countingptr.hh"
#include"../common/multitypetree.hh"
#include"../common/cpstoragepolicy.hh"
#include"../common/function.hh"

#include"gridfunctionspace.hh"

namespace Dune {
  namespace PDELab {

    //! \addtogroup GridFunctionSpace
    //! \ingroup PDELab
    //! \{

	template<typename F, bool FisLeaf, typename LFS, bool LFSisLeaf> 
	struct InterpolateVisitNodeMetaProgram;

	template<typename F, typename LFS, int n, int i> 
	struct InterpolateVisitChildMetaProgram // visit i'th child of inner node
	{
	  template<typename XG, typename E>
	  static void interpolate (const F& f, const LFS& lfs, XG& xg, const E& e)
	  {
        // vist children of both nodes in pairs
		typedef typename F::template Child<i>::Type FC;
		typedef typename LFS::template Child<i>::Type LFSC;

        const FC& fc=f.template getChild<i>();
        const LFSC& lfsc=lfs.template getChild<i>();

        InterpolateVisitNodeMetaProgram<FC,FC::isLeaf,LFSC,LFSC::isLeaf>::interpolate(fc,lfsc,xg,e);
		InterpolateVisitChildMetaProgram<F,LFS,n,i+1>::interpolate(f,lfs,xg,e);
	  }
	};

	template<typename F, typename LFS, int n> 
	struct InterpolateVisitChildMetaProgram<F,LFS,n,n> // end of child recursion
	{
      // end of child recursion
	  template<typename XG, typename E>
	  static void interpolate (const F& f, const LFS& lfs, XG& xg, const E& e)
	  {
        return;
	  }
	};

	template<typename F, bool FisLeaf, typename LFS, bool LFSisLeaf> 
	struct InterpolateVisitNodeMetaProgram // visit inner node
	{
	  template<typename XG, typename E>
	  static void interpolate (const F& f, const LFS& lfs, XG& xg, const E& e)
	  {
        // both are inner nodes, visit all children
        // check that both have same number of children
		dune_static_assert((static_cast<int>(F::CHILDREN)==static_cast<int>(LFS::CHILDREN)),  
						   "both nodes must have same number of children");
 
        // start child recursion
		InterpolateVisitChildMetaProgram<F,LFS,F::CHILDREN,0>::interpolate(f,lfs,xg,e);
	  }
	};

	template<typename F, typename LFS> 
	struct InterpolateVisitNodeMetaProgram<F,true,LFS,false> // try to interpolate components from vector valued function
	{
	  template<typename XG, typename E>
	  static void interpolate (const F& f, const LFS& lfs, XG& xg, const E& e)
	  {
		dune_static_assert((static_cast<int>(LFS::isPower)==1),  
						   "specialization only for power");
		dune_static_assert((static_cast<int>(LFS::template Child<0>::Type::isLeaf)==1),  
						   "children must be leaves");
		dune_static_assert((static_cast<int>(F::Traits::dimRange)==static_cast<int>(LFS::CHILDREN)),  
						   "number of components must coincide with number of children");
        for (int k=0; k<LFS::CHILDREN; k++)
          {
            // allocate vector where to store coefficients from basis
            std::vector<typename XG::ElementType> xl(lfs.getChild(k).size());

            // call interpolate for the basis
            typedef GridFunctionToLocalFunctionAdapter<F> LF;
            LF localf(f,e);
            typedef SelectComponentAdapter<LF> LFCOMP;
            LFCOMP localfcomp(localf,k);
            lfs.getChild(k).localFiniteElement().localInterpolation().interpolate(localfcomp,xl);

            // write coefficients into local vector 
            lfs.getChild(k).vwrite(xl,xg);
          }
	  }
	};

	template<typename F, typename LFS> 
	struct InterpolateVisitNodeMetaProgram<F,true,LFS,true> // leaf node node in both trees 
	{
	  template<typename XG, typename E>
	  static void interpolate (const F& f, const LFS& lfs, XG& xg, const E& e)
	  {
        // now we are at a single component local function space
        // which is part of a multi component local function space

		// allocate vector where to store coefficients from basis
		std::vector<typename XG::ElementType> xl(lfs.size());

		// call interpolate for the basis
		lfs.localFiniteElement().localInterpolation().interpolate(GridFunctionToLocalFunctionAdapter<F>(f,e),xl);

		// write coefficients into local vector 
		lfs.vwrite(xl,xg);
	  }
	};


    // interpolation from a given grid function
    template<typename F, typename GFS, typename XG>
    void interpolate (F& f, const GFS& gfs, XG& xg)
    {
      // this is the leaf version now

      // get some types
      typedef typename GFS::Traits::GridViewType GV;
      typedef typename GV::Traits::template Codim<0>::Iterator ElementIterator;

      // make local function space
      typedef typename GFS::LocalFunctionSpace LFS;
      LFS lfs(gfs);

      // loop once over the grid
      for (ElementIterator it = gfs.gridview().template begin<0>();
           it!=gfs.gridview().template end<0>(); ++it)
        {
          // bind local function space to element
          lfs.bind(*it);

          // call interpolate
		  InterpolateVisitNodeMetaProgram<F,F::isLeaf,LFS,LFS::isLeaf>::interpolate(f,lfs,xg,*it);
        }
    }

    //! \} group GridFunctionSpace
  } // namespace PDELab
} // namespace Dune

#endif