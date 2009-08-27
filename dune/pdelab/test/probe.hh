#ifndef DUNE_PDELAB_PROBE_HH
#define DUNE_PDELAB_PROBE_HH

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <dune/common/tuples.hh>
#include <dune/pdelab/common/function.hh>

namespace Dune {
  namespace PDELab {

    //
    // Interface
    //

    class ProbeInterface
    {
    public:
      template<typename GF>
      void measure(const GF &gf, double time = 0);

      template<typename GF>
      void measureFinal(const GF &gf, double time = 0);
    };

    template<template<typename GridView> class T>
    class LevelProbeFactoryInterface
    {
    public:
      template<typename GV>
      struct Traits : public T<GV> {};

      template<typename GV>
      SmartPointer<typename Traits<GV>::Probe>
      getProbe(const GV &gv, unsigned level);
    };

    template<template<typename Grid> class T>
    class GridProbeFactoryInterface
    {
    public:
      template<typename G>
      struct Traits : public T<G> {};

      template<typename G>
      SmartPointer<typename Traits<G>::LevelProbeFactory>
      levelProbeFactory(const G &grid, const std::string &tag);
    };

    //
    // ErrorProbe
    //

    // A probe which extracts some kind of error measure from the solution,
    // and provides this information afterwards

    class ErrorProbeInterface
      : public ProbeInterface
    {
    public:
      double get_error() const;
    };

    //
    // DummyProbe
    //

    class DummyProbe
    {
    public:
      template<typename GF>
      void measure(const GF &gf, double time = 0) { /* do nothing */ }

      template<typename GF>
      void measureFinal(const GF &gf, double time = 0) { /* do nothing */ }
    };

    //
    // Pair
    //

    template<typename P1, typename P2>
    class ProbePair
    {
      SmartPointer<P1> p1;
      SmartPointer<P2> p2;

    public:
      ProbePair(SmartPointer<P1> p1_, SmartPointer<P2> p2_)
        : p1(p1_), p2(p2_)
      { }

      template<typename GF>
      void measure(const GF &gf, double time = 0) {
        p1->measure(gf, time);
        p2->measure(gf, time);
      }

      template<typename GF>
      void measureFinal(const GF &gf, double time = 0) {
        p1->measureFinal(gf, time);
        p2->measureFinal(gf, time);
      }
    };

    template<typename LPF1, typename LPF2>
    class LevelProbeFactoryPair
    {
      SmartPointer<LPF1> lpf1;
      SmartPointer<LPF2> lpf2;

    public:
      template<typename GV>
      struct Traits {
        typedef ProbePair<typename LPF1::template Traits<GV>::Probe,
                          typename LPF2::template Traits<GV>::Probe> Probe;
      };

      LevelProbeFactoryPair(SmartPointer<LPF1> lpf1_, SmartPointer<LPF2> lpf2_)
        : lpf1(lpf1_), lpf2(lpf2_)
      { }

      template<typename GV>
      SmartPointer<typename Traits<GV>::Probe>
      getProbe(const GV &gv, unsigned level)
      {
        return new typename Traits<GV>::Probe(lpf1->getProbe(gv, level),
                                              lpf2->getProbe(gv, level));
      }
    };

    template<typename GPF1, typename GPF2>
    class GridProbeFactoryPair
    {
      SmartPointer<GPF1> gpf1;
      SmartPointer<GPF2> gpf2;

    public:
      template<typename G>
      struct Traits {
        typedef LevelProbeFactoryPair<
          typename GPF1::template Traits<G>::LevelProbeFactory,
          typename GPF2::template Traits<G>::LevelProbeFactory> LevelProbeFactory;
      };

      GridProbeFactoryPair(SmartPointer<GPF1> gpf1_, SmartPointer<GPF2> gpf2_)
        : gpf1(gpf1_), gpf2(gpf2_)
      { }

      template<typename G>
      SmartPointer<typename Traits<G>::LevelProbeFactory>
      levelProbeFactory(const G &grid, const std::string &tag)
      {
        return new typename Traits<G>::LevelProbeFactory(gpf1->levelProbeFactory(grid, tag),
                                                         gpf2->levelProbeFactory(grid, tag));
      }

    };
    
    template<typename GPF0, typename GPF1 = int, typename GPF2 = int, typename GPF3 = int,
             typename GPF4 = int, typename GPF5 = int, typename GPF6 = int,
             typename GPF7 = int, typename GPF8 = int, typename GPF9 = int>
    struct GridProbeFactoryListTraits;

    template<typename GPF0>
    struct GridProbeFactoryListTraits<GPF0> {
      typedef GPF0 GPF;
    };

    template<typename GPF0, typename GPF1, typename GPF2, typename GPF3,
             typename GPF4, typename GPF5, typename GPF6,
             typename GPF7, typename GPF8, typename GPF9>
    struct GridProbeFactoryListTraits {
      typedef GridProbeFactoryPair<
        GPF0,
        typename GridProbeFactoryListTraits<
          GPF1, GPF2, GPF3, GPF4, GPF5, GPF6, GPF7, GPF8, GPF9,
          int>::GPF> GPF;
    };

    template<typename GPF0>
    SmartPointer<GPF0> makeGridProbeFactoryList(SmartPointer<GPF0> gpf0,
                                                SmartPointer<int> = 0,
                                                SmartPointer<int> = 0,
                                                SmartPointer<int> = 0,
                                                SmartPointer<int> = 0,
                                                SmartPointer<int> = 0,
                                                SmartPointer<int> = 0,
                                                SmartPointer<int> = 0,
                                                SmartPointer<int> = 0,
                                                SmartPointer<int> = 0)
    {
      return gpf0;
    }

    template<typename GPF0, typename GPF1, typename GPF2 = int, typename GPF3 = int,
             typename GPF4 = int, typename GPF5 = int, typename GPF6 = int,
             typename GPF7 = int, typename GPF8 = int, typename GPF9 = int>
    SmartPointer<
      typename GridProbeFactoryListTraits<
        GPF0, GPF1, GPF2, GPF3, GPF4, GPF5, GPF6, GPF7, GPF8, GPF9>::GPF>
      makeGridProbeFactoryList(SmartPointer<GPF0> gpf0,
                               SmartPointer<GPF1> gpf1,
                               SmartPointer<GPF2> gpf2 = 0,
                               SmartPointer<GPF3> gpf3 = 0,
                               SmartPointer<GPF4> gpf4 = 0,
                               SmartPointer<GPF5> gpf5 = 0,
                               SmartPointer<GPF6> gpf6 = 0,
                               SmartPointer<GPF7> gpf7 = 0,
                               SmartPointer<GPF8> gpf8 = 0,
                               SmartPointer<GPF9> gpf9 = 0)
    {
      return new
        typename GridProbeFactoryListTraits<
          GPF0, GPF1, GPF2, GPF3, GPF4, GPF5, GPF6, GPF7, GPF8, GPF9
        >::GPF(gpf0,
               makeGridProbeFactoryList(gpf1, gpf2, gpf3, gpf4, gpf5, gpf6, gpf7, gpf8, gpf9));
    }

    //
    // Gnuplot
    //

    template<typename D>
    class GnuplotProbe
      : public DummyProbe
    {
      std::ostream &data;
      D x;

    public:
      GnuplotProbe(std::ostream &data_, const D &x_) : data(data_), x(x_) {}

      template<typename GF>
      void measure(const GF &gf, double time = 0) {
        typename GF::Traits::RangeType y;
        GridFunctionToFunctionAdapter<GF>(gf).evaluate(x, y);
        data << time;
        for(unsigned i = 0; i < GF::Traits::dimDomain; ++i)
          data << "\t" << y[i];
        data << std::endl;
      }

    };

    template<typename D>
    class GnuplotLevelProbeFactory
    {
      GnuplotGraph &graph;
      D x;
      std::ostream::pos_type datapos;
      unsigned &index;
      std::string lastplot;
      const std::string tag;

    public:
      template<typename GV>
      struct Traits {
        typedef GnuplotProbe<D> Probe;
      };

      GnuplotLevelProbeFactory(GnuplotGraph &graph_, const D &x_, unsigned &index_,
                               const std::string &tag_)
        : graph(graph_), x(x_), datapos(graph.dat().tellp()), index(index_), lastplot(""), tag(tag_)
      { }
      ~GnuplotLevelProbeFactory()
      {
        if(datapos != graph.dat().tellp()) {
          graph.dat() << "\n\n";
          graph.addPlot(lastplot);
          ++index;
        }
      }

      template<typename GV>
      SmartPointer<typename Traits<GV>::Probe>
      getProbe(const GV &gv, unsigned level)
      {
        if(datapos != graph.dat().tellp()) {
          graph.dat() << "\n\n";
          graph.addPlot(lastplot);
          ++index;
        }
        graph.dat() << "# LEVEL" << level << std::endl;
        datapos = graph.dat().tellp();
        std::ostringstream plotconstruct;
        plotconstruct << "'" << graph.datname() << "'"
                      << " index " << index
                      << " title '" << tag << " level " << level << "'"
                      << " with linespoints pt 1";
        lastplot = plotconstruct.str();
        return new typename Traits<GV>::Probe(graph.dat(), x);
      }
    };

    template<typename D>
    class GnuplotGridProbeFactory
    {
      const D x;
      GnuplotGraph graph;
      unsigned index;

    public:
      template<typename G>
      struct Traits {
        typedef GnuplotLevelProbeFactory<D> LevelProbeFactory;
      };

      GnuplotGridProbeFactory(const std::string &fileprefix, const D &x_)
        : x(x_), graph(fileprefix), index(0)
      {
        graph.addCommand("set terminal postscript eps color solid");
        graph.addCommand("set output '"+fileprefix+".eps'");
        graph.addCommand("");
        graph.addCommand("set key left top reverse Left");
        graph.addCommand("");
      }

      template<typename G>
      SmartPointer<typename Traits<G>::LevelProbeFactory>
      levelProbeFactory(const G &grid, const std::string &tag)
      {
        return new typename Traits<G>::LevelProbeFactory(graph, x, index, tag);
      }
    };
    
  } // namespace PDELab
} // namespace Dune

#endif //DUNE_PDELAB_PROBE_HH