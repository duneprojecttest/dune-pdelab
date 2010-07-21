// -*- tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=8 sw=2 sts=2:
#ifndef DUNE_PDELAB_FUNCTION_DIVISION_HH
#define DUNE_PDELAB_FUNCTION_DIVISION_HH

#include <dune/common/static_assert.hh>

#include <dune/pdelab/common/function.hh>

namespace Dune {
  namespace PDELab {

    //! Substract two GridFunctions
    /**
     * \tparam GF1 Type for the GridFunction on the left side of the division
     * \tparam GF2 Type for the GridFunction on the right side of the division
     */
    template<typename GF1, typename GF2>
    class DivisionGridFunctionAdapter
      : public GridFunctionBase<typename GF1::Traits,
                                DivisionGridFunctionAdapter<GF1,GF2> >
    {
      dune_static_assert(GF2::Traits::dimRange == 1, "Range dimension must be "
                         "1 for the divisor of a DivisionGridFunctionAdapter");
      typedef typename GF1::Traits T;
      typedef GridFunctionBase<T, DivisionGridFunctionAdapter<GF1,GF2>
              > Base;

      const GF1& gf1;
      const GF2& gf2;

    public:
      typedef typename Base::Traits Traits;

      DivisionGridFunctionAdapter(const GF1& gf1_, const GF2& gf2_)
        : gf1(gf1_), gf2(gf2_)
      { }

      void evaluate(const typename Traits::ElementType &e,
                    const typename Traits::DomainType &x,
                    typename Traits::RangeType &y) const {
        gf1.evaluate(e,x,y);
        typename GF2::Traits::RangeType y2;
        gf2.evaluate(e,x,y2);
        y /= y2;
      }

      const typename Traits::GridViewType& getGridView() const {
        return gf1.getGridView();
      }
    };

  } // namspace PDELab
} // namspace Dune

#endif // DUNE_PDELAB_FUNCTION_DIVISION_HH