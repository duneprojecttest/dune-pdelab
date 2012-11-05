// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_PDELAB_GRIDFUNCTIONSPACE_HH
#define DUNE_PDELAB_GRIDFUNCTIONSPACE_HH

#include <cstddef>
#include <map>
#include <ostream>
#include <set>
#include <vector>

#include <dune/common/exceptions.hh>
#include <dune/geometry/type.hh>
#include <dune/common/shared_ptr.hh>
#include <dune/common/static_assert.hh>
#include <dune/common/stdstreams.hh>
#include <dune/common/typetraits.hh>

#include <dune/geometry/referenceelements.hh>
#include <dune/geometry/type.hh>

#include <dune/localfunctions/common/interfaceswitch.hh>
#include <dune/localfunctions/common/localkey.hh>

#include <dune/pdelab/backend/backendselector.hh>
#include <dune/pdelab/common/geometrywrapper.hh>
#include <dune/pdelab/common/typetree.hh>
#include <dune/pdelab/gridfunctionspace/tags.hh>
#include <dune/pdelab/gridfunctionspace/gridfunctionspaceutilities.hh>
#include <dune/pdelab/ordering/gridviewordering.hh>
#include <dune/pdelab/ordering/lexicographicordering.hh>
#include <dune/pdelab/gridfunctionspace/localfunctionspace.hh>
#include <dune/pdelab/gridfunctionspace/datahandleprovider.hh>
#include <dune/pdelab/gridfunctionspace/powergridfunctionspace.hh>
#include <dune/pdelab/gridfunctionspace/compositegridfunctionspace.hh>

#include <dune/pdelab/backend/istl/tags.hh>

namespace Dune {
  namespace PDELab {

    //! \addtogroup GridFunctionSpace grid function space
    //! \ingroup PDELab
    //! \{

    //=======================================
    // grid function space : single component case
    //=======================================

    //! collect types exported by a leaf grid function space
    /**
     * This is based on a global FiniteElementMap
     */
    template<typename G, typename L, typename C, typename B, typename O>
    struct GridFunctionSpaceTraits
    {
      //! True if this grid function space is composed of others.
      static const bool isComposite = false;

      //! the grid view where grid function is defined upon
      typedef G GridViewType;

      typedef G GridView;

      //! vector backend
      typedef B BackendType;

      typedef B Backend;

      //! short cut for size type exported by Backend
      typedef typename B::size_type SizeType;

      //! finite element map
      typedef L FiniteElementMapType;

      //! finite element map
      typedef L FiniteElementMap;

      //! finite element
      typedef typename L::Traits::FiniteElementType FiniteElementType;

      typedef typename L::Traits::FiniteElementType FiniteElement;

      //! type representing constraints
      typedef C ConstraintsType;

      //! tag describing the ordering.
      /**
       * The tag type may contain additional constants and typedefs to
       * control the behavior of the created ordering.
       */
      typedef O OrderingTag;

    };


    // Empty constraints assembler class
    class NoConstraints
    {
    public:
      enum { doBoundary = false };
      enum { doProcessor = false }; // added ParallelStuff
      enum { doSkeleton = false };
      enum { doVolume = false }; // might be necessary for cell-centered in parallel

      // methods are here just to show interfaces; they are never called because doX are false above
      template<typename F, typename I, typename LFS, typename T>
      void boundary (const F& f, const IntersectionGeometry<I>& ig, const LFS& lfs, T& trafo) const
      {
      }

      template<typename I, typename LFS, typename T>
      void processor (const IntersectionGeometry<I>& ig, const LFS& lfs, T& trafo) const
      {
      }

      template<typename I, typename LFS, typename T>
      void skeleton (const IntersectionGeometry<I>& ig, const LFS& lfs, T& trafo) const
      {
      }

      template<typename E, typename LFS, typename T>
      void volume (const ElementGeometry<E>& eg, const LFS& lfs, T& trafo) const
      {
      }

    };


    /** \brief A grid function space.
     *
     *  \tparam GV   Type implementing GridView
     *  \tparam FEM  Type implementing FiniteElementMapInterface
     *  \tparam CE   Type for constraints assembler
     *  \tparam B    Backend type
     *  \tparam P    Parameter type. Possible types are
     * \link GridFunctionGeneralMapper \endlink (arbitrary number of unknowns per
     * entity) or \link GridFunctionRestrictedMapper \endlink (fixed number of unknowns per
     * entity) or \link GridFunctionStaticSize \endlink (number of unknowns per
     * entity, known at compile-time)
     */
    template<typename GV, typename FEM, typename CE=NoConstraints,
             typename B=ISTLVectorBackend<>, typename P=DefaultLeafOrderingTag>
    class GridFunctionSpace
      : public TypeTree::LeafNode
      , public GridFunctionOutputParameters
      , public DataHandleProvider<GridFunctionSpace<GV,FEM,CE,B,P> >
    {

      typedef TypeTree::TransformTree<GridFunctionSpace,gfs_to_ordering<GridFunctionSpace> > ordering_transformation;

    public:
      //! export Traits class
      typedef GridFunctionSpaceTraits<GV,FEM,CE,B,P> Traits;
      typedef typename GV::Traits::template Codim<0>::Entity Element;
      typedef typename GV::Traits::template Codim<0>::Iterator ElementIterator;

      typedef P SizeTag;

      typedef P OrderingTag;

      typedef LeafGridFunctionSpaceTag ImplementationTag;

      typedef typename ordering_transformation::Type Ordering;

       //! extract type for storing constraints
      template<typename E>
      struct ConstraintsContainer
      {
        //! \brief define Type as the Type of a container of E's
        typedef ConstraintsTransformation<typename Ordering::Traits::DOFIndex,typename Ordering::Traits::ContainerIndex,E> Type;
      private:
        ConstraintsContainer () {}
      };

      //! constructor
      GridFunctionSpace (const GV& gridview, const FEM& fem, const CE& ce_, const B& backend = B())
        : defaultce(ce_)
        , gv(gridview)
        , pfem(stackobject_to_shared_ptr(fem))
        , ce(ce_)
        , _backend(backend)
      {
      }

      //! constructor
      GridFunctionSpace (const GV& gridview, const shared_ptr<const FEM>& fem, const CE& ce_, const B& backend = B())
        : defaultce(ce_)
        , gv(gridview)
        , pfem(fem)
        , ce(ce_)
        , _backend(backend)
      {
      }

      //! constructor
      GridFunctionSpace (const GV& gridview, const FEM& fem, const B& backend = B())
        : gv(gridview)
        , pfem(stackobject_to_shared_ptr(fem))
        , ce(defaultce)
        , _backend(backend)
      {
      }

      //! constructor
      GridFunctionSpace (const GV& gridview, const shared_ptr<const FEM>& fem, const B& backend = B())
        : gv(gridview)
        , pfem(fem)
        , ce(defaultce)
        , _backend(backend)
      {
      }

      //! get grid view
      const GV& gridview () const DUNE_DEPRECATED
      {
        return gv;
      }

      //! get grid view
      const GV& gridView () const
      {
        return gv;
      }

      //! get finite element map
      const FEM& finiteElementMap () const
      {
        return *pfem;
      }

      //! get finite element map
      shared_ptr<const FEM> finiteElementMapStorage () const
      {
        return pfem;
      }

      //! get finite element map
      const FEM& localFiniteElementMap () const DUNE_DEPRECATED
      {
        return *pfem;
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

      //! get dimension of root finite element space
      typename Traits::SizeType globalSize () const
      {
        return ordering().size();
      }

      //! get dimension of this finite element space
      typename Traits::SizeType size () const
      {
        return ordering().size();
      }

      //! get max dimension of shape function space
      //! \todo What are the exact semantics of maxLocalSize?
      typename Traits::SizeType maxLocalSize () const
      {
        return ordering().maxLocalSize();
      }

      //! Returns whether this GridFunctionSpace contains entities with PartitionType partition.
      bool containsPartition(PartitionType partition) const
      {
        return ordering().containsPartition(partition);
      }

      //! map index from our index set [0,size()-1] to root index set
      typename Traits::SizeType upMap (typename Traits::SizeType i) const
      {
        return i;
      }

      // return constraints engine
      const typename Traits::ConstraintsType& constraints () const
      {
        return ce;
      }

      //------------------------------

      B& backend()
      {
        return _backend;
      }

      const B& backend() const
      {
        return _backend;
      }

      const std::string& name() const
      {
        return _name;
      }

      void name(const std::string& name)
      {
        _name = name;
      }

    private:
      CE defaultce;
      const GV& gv;
      shared_ptr<FEM const> pfem;
      typename Traits::SizeType nlocal;
      typename Traits::SizeType nglobal;
      const CE& ce;
      B _backend;
      bool fixed_size;
      std::string _name;

      typedef std::map<Dune::GeometryType,typename Traits::SizeType> GTOffsetMap;
      GTOffsetMap gtoffset; // offset in vector for given geometry type
      std::vector<typename Traits::SizeType> offset; // offset into big vector for each entity;
      std::set<unsigned int> codimUsed;

      mutable shared_ptr<Ordering> _ordering;
    };


#if 0

    //=======================================
    // Subspace construction
    //=======================================

    template<typename GFS, std::size_t, typename Tag> // primary template, only specializations are used !
    class GridFunctionSubSpaceBase
    {
    };

    template<typename GFS>
    class CompositeGridFunctionSubSpaceNode;


    template<typename Mapper, DUNE_TYPETREE_COMPOSITENODE_TEMPLATE_CHILDREN_FOR_SPECIALIZATION>
    class CompositeGridFunctionSubSpaceNode<CompositeGridFunctionSpace<Mapper,DUNE_TYPETREE_COMPOSITENODE_CHILDTYPES> >
      : public DUNE_TYPETREE_COMPOSITENODE_BASETYPE
    {

      typedef DUNE_TYPETREE_COMPOSITENODE_BASETYPE NodeType;

    public:

      CompositeGridFunctionSubSpaceNode(const typename NodeType::NodeStorage& nodeStorage)
        : NodeType(nodeStorage)
      {}

    };


    // CGFS is a composite
    template<typename GFS, std::size_t k>
    class GridFunctionSubSpaceBase<GFS,k,CompositeGridFunctionSpaceTag>
      : public CompositeGridFunctionSubSpaceNode<typename GFS::template Child<k>::Type>
    {
      typedef typename GFS::template Child<k>::Type CGFS;

    public:
      //! export traits class
      typedef typename CGFS::Traits Traits;
      typedef CompositeGridFunctionSpaceTag ImplementationTag;

      GridFunctionSubSpaceBase (const GFS& gfs)
        : CompositeGridFunctionSubSpaceNode<CGFS>(gfs.template child<k>().nodeStorage())
        , pgfs(stackobject_to_shared_ptr(gfs))
        , pcgfs(gfs.template childStorage<k>())
      {
      }

        //! extract type for storing constraints
      template<typename E>
      struct ConstraintsContainer
      {
        //! \brief define Type as the Type of a container of E's
        typedef ConstraintsTransformation<typename Traits::SizeType,E> Type;
      private:
        ConstraintsContainer () {}
      };

      // get grid view
      const typename Traits::GridViewType& gridview () const
      {
        return pgfs->gridview();
      }

      //! get dimension of root finite element space
      typename Traits::SizeType globalSize () const
      {
        return pgfs->globalSize();
      }

      //! get dimension of this finite element space
      typename Traits::SizeType size () const
      {
        return pcgfs->globalSize();
      }

      // get max dimension of shape function space
      typename Traits::SizeType maxLocalSize () const
      {
        return pcgfs->maxLocalSize();
      }

      //! map index from our index set [0,size()-1] to root index set
      typename Traits::SizeType upMap (typename Traits::SizeType i) const
      {
        return pgfs->upMap(pgfs->subMap(k,i));
      }

      //! map index from child i's index set into our index set
      template<int i>
      typename Traits::SizeType subMap (typename Traits::SizeType j) const
      {
        return this->subMap(i,j);
      }

      typename Traits::SizeType subMap (typename Traits::SizeType i, typename Traits::SizeType j) const
      {
        return pcgfs->subMap(i,j);
      }

    private:
      shared_ptr<GFS const> pgfs;
      shared_ptr<CGFS const> pcgfs;
    };


    // CGFS is a power
    template<typename GFS, std::size_t k>
    class GridFunctionSubSpaceBase<GFS,k,PowerGridFunctionSpaceTag>
      : public TypeTree::PowerNode<typename GFS::template Child<k>::Type::ChildType,GFS::template Child<k>::Type::CHILDREN>
    {
      typedef typename GFS::template Child<k>::Type CGFS;
      typedef TypeTree::PowerNode<typename GFS::template Child<k>::Type::ChildType,GFS::template Child<k>::Type::CHILDREN> NodeType;

    public:
      //! export traits class
      typedef typename CGFS::Traits Traits;
      typedef PowerGridFunctionSpaceTag ImplementationTag;

      GridFunctionSubSpaceBase (const GFS& gfs)
        : NodeType(gfs.template child<k>().nodeStorage())
        , pgfs(stackobject_to_shared_ptr(gfs))
        , pcgfs(gfs.template childStorage<k>())
      {
      }

       //! extract type for storing constraints
      template<typename E>
      struct ConstraintsContainer
      {
        //! \brief define Type as the Type of a container of E's
        typedef ConstraintsTransformation<typename Traits::SizeType,E> Type;
      private:
        ConstraintsContainer () {}
      };

      // get grid view
      const typename Traits::GridViewType& gridview () const
      {
        return pgfs->gridview();
      }

      //! get dimension of root finite element space
      typename Traits::SizeType globalSize () const
      {
        return pgfs->globalSize();
      }

      //! get dimension of this finite element space
      typename Traits::SizeType size () const
      {
        return pcgfs->globalSize();
      }

      // get max dimension of shape function space
      typename Traits::SizeType maxLocalSize () const
      {
        return pcgfs->maxLocalSize();
      }

      //! map index from our index set [0,size()-1] to root index set
      typename Traits::SizeType upMap (typename Traits::SizeType i) const
      {
        return pgfs->upMap(pgfs->subMap(k,i));
      }

      //! map index from child i's index set into our index set
      template<int i>
      typename Traits::SizeType subMap (typename Traits::SizeType j) const
      {
        return this->subMap(i,j);
      }

      typename Traits::SizeType subMap (typename Traits::SizeType i, typename Traits::SizeType j) const
      {
        return pcgfs->subMap(i,j);
      }

      bool fixedSize() const
      {
        return pcgfs->fixedSize();
      }

    private:
      shared_ptr<GFS const> pgfs;
      shared_ptr<CGFS const> pcgfs;
    };


    // CGFS is a leaf
    template<typename GFS, std::size_t k>
    class GridFunctionSubSpaceBase<GFS,k,LeafGridFunctionSpaceTag>
      : public TypeTree::LeafNode
    {
      typedef typename GFS::template Child<k>::Type CGFS;

    public:
      //! export traits class
      typedef typename CGFS::Traits Traits;
      typedef typename Traits::GridViewType GV;
      typedef typename Traits::FiniteElementMapType FEM;
      typedef typename GV::Traits::template Codim<0>::Entity Element;

      typedef LeafGridFunctionSpaceTag ImplementationTag;

      GridFunctionSubSpaceBase (const GFS& gfs)
        : pgfs(stackobject_to_shared_ptr(gfs))
        , pcgfs(gfs.template childStorage<k>())
      {
      }

        //! extract type for storing constraints
      template<typename E>
      struct ConstraintsContainer
      {
        //! \brief define Type as the Type of a container of E's
        typedef ConstraintsTransformation<typename Traits::SizeType,E> Type;
      private:
        ConstraintsContainer () {}
      };

      // get grid view
      const typename Traits::GridViewType& gridview () const
      {
        return pcgfs->gridview();
      }

      // get finite element map
      const FEM& finiteElementMap () const
      {
        return pcgfs->finiteElementMap();
      }

      // get finite element map
      const FEM& localFiniteElementMap () const DUNE_DEPRECATED
      {
        return pcgfs->finiteElementMap();
      }

      //! get dimension of root finite element space
      typename Traits::SizeType globalSize () const
      {
        return pgfs->globalSize();
      }

      //! get dimension of this finite element space
      typename Traits::SizeType size () const
      {
        return pcgfs->globalSize();
      }

      // get max dimension of shape function space
      typename Traits::SizeType maxLocalSize () const
      {
        return pcgfs->maxLocalSize();
      }

      //! map from our index set [0..size()-1] into root index set
      typename Traits::SizeType upMap (typename Traits::SizeType i) const
      {
        return pgfs->upMap(pgfs->subMap(k,i));
      }

      // compute global indices for one element
      template<typename StorageIterator>
      void globalIndices (const typename Traits::FiniteElementType& fe,
                          const Element& e, StorageIterator it, StorageIterator endit) const
      {
        pcgfs->globalIndices(fe,e,it,endit);
      }

      bool fixedSize() const
      {
        return pcgfs->fixedSize();
      }

    private:
      shared_ptr<GFS const> pgfs;
      shared_ptr<CGFS const> pcgfs;
    };

    template<typename GFS, std::size_t k>
    class GridFunctionSubSpace : public GridFunctionSubSpaceBase<GFS,k,typename GFS::template Child<k>::Type::ImplementationTag>
    {

      typedef GridFunctionSubSpaceBase<GFS,k,typename GFS::template Child<k>::Type::ImplementationTag> BaseT;

    public:

      GridFunctionSubSpace (const GFS& gfs)
        : BaseT(gfs)
      {
        Dune::dinfo << "GridFunctionSubSpace:" << std::endl;
        Dune::dinfo << "root space size = " << gfs.globalSize()
                    << " max local size = " << this->maxLocalSize()
                    << std::endl;
      }
    };

#endif // 0


  } // namespace PDELab
} // namespace Dune

#endif
