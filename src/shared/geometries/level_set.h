/* ------------------------------------------------------------------------- *
 *                                SPHinXsys                                  *
 * ------------------------------------------------------------------------- *
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle *
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for    *
 * physical accurate simulation and aims to model coupled industrial dynamic *
 * systems including fluid, solid, multi-body dynamics and beyond with SPH   *
 * (smoothed particle hydrodynamics), a meshless computational method using  *
 * particle discretization.                                                  *
 *                                                                           *
 * SPHinXsys is partially funded by German Research Foundation               *
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,            *
 *  HU1527/12-1 and HU1527/12-4.                                             *
 *                                                                           *
 * Portions copyright (c) 2017-2023 Technical University of Munich and       *
 * the authors' affiliations.                                                *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may   *
 * not use this file except in compliance with the License. You may obtain a *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
 *                                                                           *
 * ------------------------------------------------------------------------- */
/**
 * @file 	level_set.h
 * @brief 	Level set is a function which is defined as signed distance to a surface or interface.
 * @author	Chi Zhang, Yongchuan Yu and Xiangyu Hu
 */

#ifndef LEVEL_SET_H
#define LEVEL_SET_H

#include "adaptation.h"
#include "base_geometry.h"
#include "mesh_with_data_packages.hpp"
#include "mesh_dynamics.h"
#include "mesh_local_dynamics.h"
#include "all_mesh_dynamics.h"

namespace SPH
{
/**
 * @class BaseLevelSet
 * @brief A abstract describes a level set field defined on a mesh.
 * Level set is a signed distance function to an interface where the zero level set locates.
 * Here, the region with negative level set is considered as the region enclose by the interface.
 */
class BaseLevelSet : public BaseMeshField
{
  public:
    BaseLevelSet(Shape &shape, SPHAdaptation &sph_adaptation);
    virtual ~BaseLevelSet(){};

    virtual void cleanInterface(Real small_shift_factor) = 0;
    virtual void correctTopology(Real small_shift_factor) = 0;
    virtual bool probeIsWithinMeshBound(const Vecd &position) = 0;
    virtual Real probeSignedDistance(const Vecd &position) = 0;
    virtual Vecd probeNormalDirection(const Vecd &position) = 0;
    virtual Vecd probeLevelSetGradient(const Vecd &position) = 0;
    virtual Real probeKernelIntegral(const Vecd &position, Real h_ratio = 1.0) = 0;
    virtual Vecd probeKernelGradientIntegral(const Vecd &position, Real h_ratio = 1.0) = 0;

  protected:
    Shape &shape_; /**< the geometry is described by the level set. */
    SPHAdaptation &sph_adaptation_;
};

/**
 * @class LevelSet
 * @brief Mesh with level set data as packages.
 * Note that the mesh containing the data packages are cell-based
 * but within the data package, the data is grid-based.
 * Note that the level set data is initialized after the constructor.
 */
class LevelSet : public MeshWithGridDataPackages<4>,
                 public BaseLevelSet
{
  public:
    Real global_h_ratio_;

    /** This constructor only initialize far field. */
    LevelSet(BoundingBox tentative_bounds, Real data_spacing, size_t buffer_size, Shape &shape, SPHAdaptation &sph_adaptation);
    /** This constructor generate inner packages too. */
    LevelSet(BoundingBox tentative_bounds, Real data_spacing, Shape &shape, SPHAdaptation &sph_adaptation);
    virtual ~LevelSet(){};

    virtual void cleanInterface(Real small_shift_factor) override;
    virtual void correctTopology(Real small_shift_factor) override;
    virtual bool probeIsWithinMeshBound(const Vecd &position) override;
    virtual Real probeSignedDistance(const Vecd &position) override;
    virtual Vecd probeNormalDirection(const Vecd &position) override;
    virtual Vecd probeLevelSetGradient(const Vecd &position) override;
    virtual Real probeKernelIntegral(const Vecd &position, Real h_ratio = 1.0) override;
    virtual Vecd probeKernelGradientIntegral(const Vecd &position, Real h_ratio = 1.0) override;
    virtual void writeMeshFieldToPlt(std::ofstream &output_file) override;
    bool isWithinCorePackage(Vecd position);
    Kernel &kernel_;

  protected:
    MeshVariable<Real> &phi_;
    MeshVariable<int> &near_interface_id_;
    MeshVariable<Vecd> &phi_gradient_;
    MeshVariable<Real> &kernel_weight_;
    MeshVariable<Vecd> &kernel_gradient_;
    MeshWithGridDataPackages<4> &mesh_data_ = *this;

    FinishDataPackages finish_data_packages{mesh_data_, shape_, kernel_, global_h_ratio_};
    MeshAllDynamics<InitializeDataInACell> initialize_data_in_a_cell{mesh_data_, shape_};
    MeshInnerDynamics<UpdateLevelSetGradient> update_level_set_gradient{mesh_data_};
    MeshInnerDynamics<UpdateKernelIntegrals> update_kernel_integrals{mesh_data_, kernel_, global_h_ratio_};
    MeshInnerDynamics<DiffuseLevelSetSign> diffuse_level_set_sign{mesh_data_};
    MeshInnerDynamics<ReinitializeLevelSet> reinitialize_level_set{mesh_data_};
    MeshInnerDynamics<MarkNearInterface> mark_near_interface{mesh_data_};
    MeshCoreDynamics<RedistanceInterface> redistance_interface{mesh_data_};

    // upwind algorithm choosing candidate difference by the sign
    Real upwindDifference(Real sign, Real df_p, Real df_n);
};

/**
 * @class RefinedLevelSet
 * @brief level set  which has double resolution of a coarse level set.
 */
class RefinedLevelSet : public RefinedMesh<LevelSet>
{
  public:
    RefinedLevelSet(BoundingBox tentative_bounds, LevelSet &coarse_level_set, Shape &shape, SPHAdaptation &sph_adaptation);
    virtual ~RefinedLevelSet(){};
};

/**
 * @class MultilevelLevelSet
 * @brief Defining a multilevel level set for a complex region.
 */
class MultilevelLevelSet : public MultilevelMesh<BaseLevelSet, LevelSet, RefinedLevelSet>
{
  public:
    MultilevelLevelSet(BoundingBox tentative_bounds, Real reference_data_spacing, size_t total_levels, Shape &shape, SPHAdaptation &sph_adaptation);
    virtual ~MultilevelLevelSet(){};

    virtual void cleanInterface(Real small_shift_factor) override;
    virtual void correctTopology(Real small_shift_factor) override;
    virtual bool probeIsWithinMeshBound(const Vecd &position) override;
    virtual Real probeSignedDistance(const Vecd &position) override;
    virtual Vecd probeNormalDirection(const Vecd &position) override;
    virtual Vecd probeLevelSetGradient(const Vecd &position) override;
    virtual Real probeKernelIntegral(const Vecd &position, Real h_ratio = 1.0) override;
    virtual Vecd probeKernelGradientIntegral(const Vecd &position, Real h_ratio = 1.0) override;

  protected:
    inline size_t getProbeLevel(const Vecd &position);
    inline size_t getCoarseLevel(Real h_ratio);
};

class MeshBody
{
  public:
    MeshBody(BoundingBox tentative_bounds, Real reference_data_spacing, size_t total_levels, Shape &shape, SPHAdaptation &sph_adaptation){};
    ~MeshBody(){};

  private:
  
};
} // namespace SPH
#endif // LEVEL_SET_H
