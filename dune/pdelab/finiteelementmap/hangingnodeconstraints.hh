// -*- tab-width: 4; indent-tabs-mode: nil -*-
#ifndef DUNE_PDELAB_HANGINGNODECONSTRAINTS_HH
#define DUNE_PDELAB_HANGINGNODECONSTRAINTS_HH

#include <cstddef>

#include<dune/common/exceptions.hh>
#include<dune/geometry/referenceelements.hh>
#include<dune/geometry/type.hh>
#include<dune/pdelab/common/geometrywrapper.hh>
#include<dune/pdelab/finiteelementmap/conformingconstraints.hh>
#include<dune/pdelab/finiteelementmap/hangingnodemanager.hh>

namespace Dune {
  namespace PDELab {

    //! \addtogroup Constraints
    //! \ingroup FiniteElementMap
    //! \{

    class HangingNodesConstraintsAssemblers
    {
    public:
      class CubeGridQ1Assembler
      {
      public:
        template<typename IG, typename LFS, typename T, typename FlagVector>
        static void assembleConstraints(const FlagVector & nodeState_e, const FlagVector & nodeState_f,
                                        const bool & e_has_hangingnodes, const bool & f_has_hangingnodes,
                                        const LFS & lfs_e, const LFS & lfs_f,
                                        T& trafo_e, T& trafo_f,
                                        const IG& ig)
        {
          typedef IG Intersection;
          typedef typename Intersection::EntityPointer CellEntityPointer;
          typedef typename Intersection::Entity Cell;
          typedef typename Intersection::Geometry FaceGeometry;
          typedef typename FaceGeometry::ctype DT;
          typedef typename LFS::Traits::FiniteElementType FiniteElementType;
          typedef typename FiniteElementType::Traits::LocalBasisType::Traits::DomainFieldType DFT;
          typedef typename FiniteElementType::Traits::LocalBasisType::Traits::RangeFieldType RFT;
          typedef typename LFS::Traits::SizeType SizeType;

          typedef typename LFS::Traits::GridFunctionSpace::Traits::GridView::IndexSet IndexSet;
          const CellEntityPointer e = ig.inside();
          const CellEntityPointer f = ! ig.boundary() ? ig.outside() : ig.inside();

          const std::size_t dimension = Intersection::dimension;

          typedef Dune::GenericReferenceElement<DT,dimension> GRE;
          const GRE& refelement_e = Dune::GenericReferenceElements<DT,dimension>::general(e->type());
          const GRE& refelement_f = Dune::GenericReferenceElements<DT,dimension>::general(f->type());

          // If both entities have hangingnodes, then the face is
          // conforming and no constraints have to be applied.
          if(e_has_hangingnodes && f_has_hangingnodes)
            return;

          // Choose local function space etc for element with hanging nodes
          const LFS & lfs = e_has_hangingnodes ? lfs_e : lfs_f;
          const IndexSet& indexSet = lfs.gridFunctionSpace().gridView().indexSet();

          const Cell& cell = *(e_has_hangingnodes ? e : f);
          const int faceindex = e_has_hangingnodes ? ig.indexInInside() : ig.indexInOutside();
          const GRE & refelement = e_has_hangingnodes ? refelement_e : refelement_f;
          const FlagVector & nodeState = e_has_hangingnodes ? nodeState_e : nodeState_f;
          T & trafo = e_has_hangingnodes ? trafo_e : trafo_f;

          // A map mapping the local indices from the face to local
          // indices of the cell
          std::vector<int> m(refelement.size(faceindex,1,dimension));
          for (int j=0; j<refelement.size(faceindex,1,dimension); j++)
            m[j] = refelement.subEntity(faceindex,1,j,dimension);

          // A map mapping the local indices from the face to global gridview indices
          std::vector<std::size_t> global_vertex_idx(refelement.size(faceindex,1,dimension));
          for (int j=0; j<refelement.size(faceindex,1,dimension); ++j)
            global_vertex_idx[j] = indexSet.subIndex(cell,refelement.subEntity(faceindex,1,j,dimension),dimension);

          // Create a DOFIndex that we will use to manually craft the correct dof indices for the constraints trafo
          // We copy one of the indices from the LocalFunctionSpace; that way, we automatically get the correct
          // TreeIndex into the DOFIndex and only have to fiddle with the EntityIndex.
          typename LFS::Traits::DOFIndex dof_index(lfs.dofIndex(0));

          typedef typename LFS::Traits::GridFunctionSpace::Ordering::Traits::DOFIndexAccessor::GeometryIndex GeometryIndexAccessor;
          const GeometryType vertex_gt(0);

          // Find the corresponding entity in the reference element
          for (int j=0; j<refelement.size(faceindex,1,dimension); j++){

            // The contribution factors of the base function bound to entity j
            typename T::RowType contribution;

            if(dimension == 3){

              assert(nodeState.size() == 8);

              const SizeType i = 4*j;

              // Neigbor relations in local indices on a quadrilateral face:
              // {Node, Direct Neighbor, Direct Neighbor, Diagonal Neighbor, Node, ...}
              const unsigned int fi[16] = {0,1,2,3, 1,0,3,2, 2,0,3,1, 3,1,2,0};

              // Only hanging nodes have contribution to other nodes
              if(nodeState[m[j]].isHanging()){

                // If both neighbors are hanging nodes, then this node
                // is diagonal to the target of the contribution
                if(nodeState[m[fi[i+1]]].isHanging() && nodeState[m[fi[i+2]]].isHanging())
                  {
                    GeometryIndexAccessor::store(dof_index.entityIndex(),
                                                 vertex_gt,
                                                 global_vertex_idx[fi[i+3]]);

                    contribution[dof_index] = 0.25;

                    GeometryIndexAccessor::store(dof_index.entityIndex(),
                                                 vertex_gt,
                                                 global_vertex_idx[j]);

                    trafo[dof_index] = contribution;
                  }
                // Direct neigbor
                else if(!nodeState[m[fi[i+1]]].isHanging())
                  {
                    GeometryIndexAccessor::store(dof_index.entityIndex(),
                                                 vertex_gt,
                                                 global_vertex_idx[fi[i+1]]);

                    contribution[dof_index] = 0.5;

                    GeometryIndexAccessor::store(dof_index.entityIndex(),
                                                 vertex_gt,
                                                 global_vertex_idx[j]);

                    trafo[dof_index] = contribution;
                  }
                // Direct neigbor
                else if(!nodeState[m[fi[i+2]]].isHanging())
                  {
                    GeometryIndexAccessor::store(dof_index.entityIndex(),
                                                 vertex_gt,
                                                 global_vertex_idx[fi[i+2]]);

                    contribution[dof_index] = 0.5;

                    GeometryIndexAccessor::store(dof_index.entityIndex(),
                                                 vertex_gt,
                                                 global_vertex_idx[j]);

                    trafo[dof_index] = contribution;
                  }
              }

            } else if(dimension == 2){

              assert(nodeState.size() == 4);


              // Only hanging nodes have contribution to other nodes
              if(nodeState[m[j]].isHanging()){

                const SizeType n_j = 1-j;

                assert( !nodeState[m[n_j]].isHanging() );

                GeometryIndexAccessor::store(dof_index.entityIndex(),
                                             vertex_gt,
                                             global_vertex_idx[n_j]);

                contribution[dof_index] = 0.5;

                GeometryIndexAccessor::store(dof_index.entityIndex(),
                                             vertex_gt,
                                             global_vertex_idx[j]);

                trafo[dof_index] = contribution;
              }

            } // end if(dimension==3)

          } // j

        } // end of static void assembleConstraints

      }; // end of class CubeGridQ1Assembler


      class SimplexGridP1Assembler
      {
      public:
        template<typename IG,
                 typename LFS,
                 typename T,
                 typename FlagVector>
        static void assembleConstraints( const FlagVector & nodeState_e,
                                         const FlagVector & nodeState_f,
                                         const bool & e_has_hangingnodes,
                                         const bool & f_has_hangingnodes,
                                         const LFS & lfs_e, const LFS & lfs_f,
                                         T& trafo_e, T& trafo_f,
                                         const IG& ig)
        {
          typedef IG Intersection;
          typedef typename Intersection::EntityPointer CellEntityPointer;
          typedef typename Intersection::Entity Cell;
          typedef typename Intersection::Geometry FaceGeometry;
          typedef typename FaceGeometry::ctype DT;
          typedef typename LFS::Traits::FiniteElementType FiniteElementType;
          typedef typename FiniteElementType::Traits::LocalBasisType::Traits::DomainFieldType DFT;
          typedef typename FiniteElementType::Traits::LocalBasisType::Traits::RangeFieldType RFT;
          typedef typename LFS::Traits::SizeType SizeType;
          typedef typename LFS::Traits::GridFunctionSpace::Traits::GridView::IndexSet IndexSet;

          const CellEntityPointer e = ig.inside();
          const CellEntityPointer f = ! ig.boundary() ? ig.outside() : ig.inside();

          const std::size_t dimension = Intersection::dimension;

          typedef Dune::GenericReferenceElement<DT,dimension> GRE;
          const GRE& refelement_e = Dune::GenericReferenceElements<DT,dimension>::general(e->type());
          const GRE& refelement_f = Dune::GenericReferenceElements<DT,dimension>::general(f->type());

          // If both entities have hangingnodes, then the face is
          // conforming and no constraints have to be applied.
          if(e_has_hangingnodes && f_has_hangingnodes)
            return;

          // Choose local function space etc for element with hanging nodes
          const LFS & lfs = e_has_hangingnodes ? lfs_e : lfs_f;
          const IndexSet& indexSet = lfs.gridFunctionSpace().gridView().indexSet();

          const Cell& cell = *(e_has_hangingnodes ? e : f);
          const int faceindex = e_has_hangingnodes ? ig.indexInInside() : ig.indexInOutside();
          const GRE & refelement = e_has_hangingnodes ? refelement_e : refelement_f;
          const FlagVector & nodeState = e_has_hangingnodes ? nodeState_e : nodeState_f;
          T & trafo = e_has_hangingnodes ? trafo_e : trafo_f;

          // A map mapping the local indices from the face to local
          // indices of the cell
          std::vector<int> m(refelement.size(faceindex,1,dimension));
          for (int j=0; j<refelement.size(faceindex,1,dimension); j++)
            m[j] = refelement.subEntity(faceindex,1,j,dimension);

          // A map mapping the local indices from the face to global gridview indices
          std::vector<std::size_t> global_vertex_idx(refelement.size(faceindex,1,dimension));
          for (int j=0; j<refelement.size(faceindex,1,dimension); ++j)
            global_vertex_idx[j] = indexSet.subIndex(cell,refelement.subEntity(faceindex,1,j,dimension),dimension);

          // Create a DOFIndex that we will use to manually craft the correct dof indices for the constraints trafo
          // We copy one of the indices from the LocalFunctionSpace; that way, we automatically get the correct
          // TreeIndex into the DOFIndex and only have to fiddle with the EntityIndex.
          typename LFS::Traits::DOFIndex dof_index(lfs.dofIndex(0));

          typedef typename LFS::Traits::GridFunctionSpace::Ordering::Traits::DOFIndexAccessor::GeometryIndex GeometryIndexAccessor;
          const GeometryType vertex_gt(0);

          // Find the corresponding entity in the reference element
          for (int j=0; j<refelement.size(faceindex,1,dimension); j++){

            // The contribution factors of the base function bound to entity j
            typename T::RowType contribution;

            if(dimension == 3){

              assert(nodeState.size() == 4);
              // Only hanging nodes have contribution to other nodes
              if(nodeState[m[j]].isHanging()){
                for( int k=1; k<=2; ++k ){

                  const SizeType n_j = (j+k)%3;

                  if( !nodeState[m[n_j]].isHanging() )
                    {
                      GeometryIndexAccessor::store(dof_index.entityIndex(),
                                                   vertex_gt,
                                                   global_vertex_idx[n_j]);

                      contribution[dof_index] = 0.5;

                      GeometryIndexAccessor::store(dof_index.entityIndex(),
                                                   vertex_gt,
                                                   global_vertex_idx[j]);

                      trafo[dof_index] = contribution;
                    }
                }
              }
            } else if(dimension == 2){

              assert(nodeState.size() == 3);
              // Only hanging nodes have contribution to other nodes
              if(nodeState[m[j]].isHanging()){
                const SizeType n_j = 1-j;
                assert( !nodeState[m[n_j]].isHanging() );
                // If both neighbors are hanging nodes, then this node
                // is diagonal to the target of the contribution
                GeometryIndexAccessor::store(dof_index.entityIndex(),
                                             vertex_gt,
                                             global_vertex_idx[n_j]);

                contribution[dof_index] = 0.5;

                GeometryIndexAccessor::store(dof_index.entityIndex(),
                                             vertex_gt,
                                             global_vertex_idx[j]);

                trafo[dof_index] = contribution;
              }


            } // end if(dimension==3)

          } // j

        } // end of static void assembleConstraints

      }; // end of class SimplexGridP1Assembler

    }; // end of class HangingNodesConstraintsAssemblers


    //! Hanging Node constraints construction
    // works in any dimension and on all element types
    template <class Grid, class HangingNodesConstraintsAssemblerType, class BoundaryFunction>
    class HangingNodesDirichletConstraints : public ConformingDirichletConstraints
    {
    private:
      typedef Dune::PDELab::HangingNodeManager<Grid,BoundaryFunction> HangingNodeManager;
      HangingNodeManager manager;

    public:
      enum { doBoundary = true };
      enum { doSkeleton = true };
      enum { doVolume = false };
      enum { dimension = Grid::dimension };

      HangingNodesDirichletConstraints( Grid & grid,
                                        bool adaptToIsolatedHangingNodes,
                                        const BoundaryFunction & _boundaryFunction )
        : manager(grid, _boundaryFunction)
      {
        if(adaptToIsolatedHangingNodes)
          manager.adaptToIsolatedHangingNodes();
      }

      void update( Grid & grid ){
        manager.analyzeView();
        manager.adaptToIsolatedHangingNodes();
      }


      //! skeleton constraints
      /**
       * \tparam I  intersection geometry
       * \tparam LFS local function space
       * \tparam T   TransformationType
       */
      template<typename IG, typename LFS, typename T>
      void skeleton (const IG& ig,
                     const LFS& lfs_e, const LFS& lfs_f,
                     T& trafo_e, T& trafo_f) const
      {
        typedef IG Intersection;
        typedef typename Intersection::EntityPointer CellEntityPointer;
        typedef typename Intersection::Geometry FaceGeometry;
        typedef typename FaceGeometry::ctype DT;

        const CellEntityPointer e = ig.inside();
        const CellEntityPointer f = ig.outside();

        const Dune::GenericReferenceElement<DT,dimension>& refelem_e
          = Dune::GenericReferenceElements<DT,dimension>::general(e->type());
        const Dune::GenericReferenceElement<DT,dimension>& refelem_f
          = Dune::GenericReferenceElements<DT,dimension>::general(f->type());

        // the return values of the hanging node manager
        typedef typename std::vector<typename HangingNodeManager::NodeState> FlagVector;
        const FlagVector isHangingNode_e(manager.hangingNodes(*e));
        const FlagVector isHangingNode_f(manager.hangingNodes(*f));

        // just to make sure that the hanging node manager is doing
        // what is expected of him
        assert(std::size_t(refelem_e.size(dimension))
               == isHangingNode_e.size());
        assert(std::size_t(refelem_f.size(dimension))
               == isHangingNode_f.size());

        // the LOCAL indices of the faces in the reference element
        const int faceindex_e = ig.indexInInside();
        const int faceindex_f = ig.indexInOutside();

        typedef typename LFS::Traits::FiniteElementType FiniteElementType;
        typedef typename FiniteElementType::Traits::LocalCoefficientsType LocalCoefficientType;
        typedef typename FiniteElementType::Traits::LocalBasisType::Traits::DomainFieldType DFT;
        typedef typename LFS::Traits::SizeType SizeType;

        bool e_has_hangingnodes = false;
        {
          for (int j=0; j<refelem_e.size(faceindex_e,1,dimension); j++){
            const int index = refelem_e.subEntity(faceindex_e,1,j,dimension);
            if(isHangingNode_e[index].isHanging())
              {
                e_has_hangingnodes = true;
                break;
              }
          }
        }
        bool f_has_hangingnodes = false;
        {
          for (int j=0; j<refelem_f.size(faceindex_f,1,dimension); j++){
            const int index = refelem_f.subEntity(faceindex_f,1,j,dimension);
            if(isHangingNode_f[index].isHanging())
              {
                f_has_hangingnodes = true;
                break;
              }
          }
        }

        if(! e_has_hangingnodes && ! f_has_hangingnodes)
          return;

        HangingNodesConstraintsAssemblerType::
          assembleConstraints(isHangingNode_e, isHangingNode_f,
                              e_has_hangingnodes, f_has_hangingnodes,
                              lfs_e,lfs_f,
                              trafo_e, trafo_f,
                              ig);
      } // skeleton

    }; // end of class HangingNodesDirichletConstraints
    //! \}

  }
}

#endif
