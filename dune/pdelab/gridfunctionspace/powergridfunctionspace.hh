// -*- tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=8 sw=2 sts=2:

#ifndef DUNE_PDELAB_POWERGRIDFUNCTIONSPACE_HH
#define DUNE_PDELAB_POWERGRIDFUNCTIONSPACE_HH

#include <cstddef>

#include <dune/common/shared_ptr.hh>

#include <dune/pdelab/common/typetree/powernode.hh>
#include <dune/pdelab/gridfunctionspace/powercompositegridfunctionspacebase.hh>
#include <dune/pdelab/gridfunctionspace/datahandleprovider.hh>
#include <dune/pdelab/gridfunctionspace/tags.hh>

namespace Dune {
  namespace PDELab {

    //=======================================
    // power grid function space
    //=======================================

    /** \brief base class for tuples of grid function spaces
        product of identical grid function spaces
        base class that holds implementation of the methods

        PGFS(T,k) = {T}^k

        \tparam T the underlying are all grid function spaces
        \tparam k power factor
        \tparam Mapper is the ordering parameter. Use e.g.
        \link GridFunctionSpaceLexicographicMapper GridFunctionSpaceLexicographicMapper \endlink
        or \link  GridFunctionSpaceComponentBlockwiseMapper  GridFunctionSpaceComponentBlockwiseMapper \endlink
        or \link  GridFunctionSpaceBlockwiseMapper  GridFunctionSpaceBlockwiseMapper \endlink
        or \link  GridFunctionSpaceDynamicBlockwiseMapper  GridFunctionSpaceDynamicBlockwiseMapper \endlink
    */
    template<typename T, std::size_t k,
             typename Backend,
             typename OrderingTag = LexicographicOrderingTag>
    class PowerGridFunctionSpace
      : public TypeTree::PowerNode<T,k>
      , public PowerCompositeGridFunctionSpaceBase<
          PowerGridFunctionSpace<T, k, Backend, OrderingTag>,
          typename T::Traits::GridViewType,
          Backend,
          OrderingTag,
          k>
      , public DataHandleProvider<PowerGridFunctionSpace<T,k,Backend,OrderingTag> >
    {

    public:

      typedef PowerGridFunctionSpaceTag ImplementationTag;

      typedef TypeTree::PowerNode<T,k> BaseT;

      typedef PowerCompositeGridFunctionSpaceBase<
        PowerGridFunctionSpace,
        typename T::Traits::GridViewType,
        Backend,
        OrderingTag,
        k
        > ImplementationBase;

      friend class PowerCompositeGridFunctionSpaceBase<
        PowerGridFunctionSpace,
        typename T::Traits::GridViewType,
        Backend,
        OrderingTag,
        k>;

      typedef TypeTree::TransformTree<PowerGridFunctionSpace,
                                      gfs_to_ordering<PowerGridFunctionSpace>
                                      > ordering_transformation;

    public:

      typedef typename ordering_transformation::Type Ordering;

      //! export traits class
      typedef typename ImplementationBase::Traits Traits;

      PowerGridFunctionSpace(T& c, const Backend& backend = Backend())
        : BaseT(c)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              const Backend& backend = Backend())
        : BaseT(c0,c1)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              T& c2,
                              const Backend& backend = Backend())
        : BaseT(c0,c1,c2)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              T& c2,
                              T& c3,
                              const Backend& backend = Backend())
        : BaseT(c0,c1,c2,c3)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              T& c2,
                              T& c3,
                              T& c4,
                              const Backend& backend = Backend())
        : BaseT(c0,c1,c2,c3,c4)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              T& c2,
                              T& c3,
                              T& c4,
                              T& c5,
                              const Backend& backend = Backend())
        : BaseT(c0,c1,c2,c3,c4,c5)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              T& c2,
                              T& c3,
                              T& c4,
                              T& c5,
                              T& c6,
                              const Backend& backend = Backend())
        : BaseT(c0,c1,c2,c3,c4,c5,c6)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              T& c2,
                              T& c3,
                              T& c4,
                              T& c5,
                              T& c6,
                              T& c7,
                              const Backend& backend = Backend())
        : BaseT(c0,c1,c2,c3,c4,c5,c6,c7)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              T& c2,
                              T& c3,
                              T& c4,
                              T& c5,
                              T& c6,
                              T& c7,
                              T& c8,
                              const Backend& backend = Backend())
        : BaseT(c0,c1,c2,c3,c4,c5,c6,c7,c8)
        , ImplementationBase(backend)
      {
      }

      PowerGridFunctionSpace (T& c0,
                              T& c1,
                              T& c2,
                              T& c3,
                              T& c4,
                              T& c5,
                              T& c6,
                              T& c7,
                              T& c8,
                              T& c9,
                              const Backend& backend = Backend())
        : BaseT(c0,c1,c2,c3,c4,c5,c6,c7,c8,c9)
        , ImplementationBase(backend)
      {
      }

      //! Direct access to the DOF ordering.
      const Ordering &ordering() const
      {
        return *orderingStorage();
      }

      //! Direct access to the DOF ordering.
      Ordering &ordering()
      {
        return *orderingStorage();
      }

      //! Direct access to the storage of the DOF ordering.
      shared_ptr<const Ordering> orderingStorage() const
      {
        if (!_ordering)
          {
            _ordering = make_shared<Ordering>(ordering_transformation::transform(*this));
            _ordering->update();
          }
        return _ordering;
      }

      //! Direct access to the storage of the DOF ordering.
      shared_ptr<Ordering> orderingStorage()
      {
        if (!_ordering)
          {
            _ordering = make_shared<Ordering>(ordering_transformation::transform(*this));
            _ordering->update();
          }
        return _ordering;
      }

    private:

      mutable shared_ptr<Ordering> _ordering;

    };

  } // namespace PDELab
} // namespace Dune

#endif // DUNE_PDELAB_POWERGRIDFUNCTIONSPACE_HH
