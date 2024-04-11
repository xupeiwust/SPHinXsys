#include "slender_structure_dynamics.h"

namespace SPH
{
namespace slender_structure_dynamics
{
//=================================================================================================//
BarAcousticTimeStepSize::BarAcousticTimeStepSize(SPHBody &sph_body, Real CFL)
    : LocalDynamicsReduce<ReduceMin>(sph_body),
      BarDataSimple(sph_body), CFL_(CFL),
      vel_(*particles_->getVariableByName<Vecd>("Velocity")),
      force_(*particles_->getVariableByName<Vecd>("Force")),
      angular_vel_(particles_->angular_vel_),
      dangular_vel_dt_(particles_->dangular_vel_dt_),
      force_prior_(*particles_->getVariableByName<Vecd>("ForcePrior")),
      thickness_(particles_->thickness_),
      mass_(*particles_->getVariableByName<Real>("Mass")),
      rho0_(particles_->elastic_solid_.ReferenceDensity()),
      E0_(particles_->elastic_solid_.YoungsModulus()),
      nu_(particles_->elastic_solid_.PoissonRatio()),
      c0_(particles_->elastic_solid_.ReferenceSoundSpeed()),
      smoothing_length_(sph_body.sph_adaptation_->ReferenceSmoothingLength()),
      angular_b_vel_(particles_->angular_b_vel_), dangular_b_vel_dt_(particles_->dangular_b_vel_dt_),
      width_(particles_->thickness_) {}
//=================================================================================================//
Real BarAcousticTimeStepSize::reduce(size_t index_i, Real dt)
{
    // Since the particle does not change its configuration in pressure relaxation step,
    // I chose a time-step size according to Eulerian method.
    Real time_setp_0 = SMIN((Real)sqrt(smoothing_length_ / (((force_[index_i] + force_prior_[index_i]) / mass_[index_i]).norm() + TinyReal)),
                            smoothing_length_ / (c0_ + vel_[index_i].norm()));
    Real time_setp_1 = SMIN((Real)sqrt(1.0 / (dangular_vel_dt_[index_i].norm() + TinyReal)),
                            Real(1.0) / (angular_vel_[index_i].norm() + TinyReal));
    Real time_setp_2 = smoothing_length_ *
                       (Real)sqrt(rho0_ * (1.0 - nu_ * nu_) / E0_ /
                                  (2.0 + (Pi * Pi / 12.0) * (1.0 - nu_) *
                                             (1.0 + 1.5 * pow(smoothing_length_ / thickness_[index_i], 2))));
    return CFL_ * SMIN(time_setp_0, time_setp_1, time_setp_2);
}
//=================================================================================================//
BarCorrectConfiguration::
    BarCorrectConfiguration(BaseInnerRelation &inner_relation)
    : LocalDynamics(inner_relation.getSPHBody()), BarDataInner(inner_relation),
      Vol_(particles_->VolumetricMeasures()),
      B_(particles_->B_),
      n0_(particles_->n0_), b_n0_(particles_->b_n0_),
      transformation_matrix0_(*particles_->getVariableByName<Matd>("TransformationMatrix")) {}
//=================================================================================================//
BarDeformationGradientTensor::
    BarDeformationGradientTensor(BaseInnerRelation &inner_relation)
    : LocalDynamics(inner_relation.getSPHBody()), BarDataInner(inner_relation),
      Vol_(particles_->VolumetricMeasures()),
      pos_(particles_->ParticlePositions()), pseudo_n_(particles_->pseudo_n_), n0_(particles_->n0_),
      B_(particles_->B_), F_(particles_->F_), F_bending_(particles_->F_bending_),
      transformation_matrix0_(*particles_->getVariableByName<Matd>("TransformationMatrix")),
      pseudo_b_n_(particles_->pseudo_n_), b_n0_(particles_->n0_), F_b_bending_(particles_->F_b_bending_) {}
//=================================================================================================//
BaseBarRelaxation::BaseBarRelaxation(BaseInnerRelation &inner_relation)
    : LocalDynamics(inner_relation.getSPHBody()), BarDataInner(inner_relation),
      rho_(*particles_->getVariableByName<Real>("Density")),
      thickness_(particles_->thickness_),
      mass_(*particles_->getVariableByName<Real>("Mass")),
      Vol_(particles_->VolumetricMeasures()),
      pos_(particles_->ParticlePositions()),
      vel_(*particles_->getVariableByName<Vecd>("Velocity")),
      force_(*particles_->getVariableByName<Vecd>("Force")),
      force_prior_(*particles_->getVariableByName<Vecd>("ForcePrior")),
      n0_(particles_->n0_), pseudo_n_(particles_->pseudo_n_),
      dpseudo_n_dt_(particles_->dpseudo_n_dt_), dpseudo_n_d2t_(particles_->dpseudo_n_d2t_),
      rotation_(particles_->rotation_), angular_vel_(particles_->angular_vel_),
      dangular_vel_dt_(particles_->dangular_vel_dt_),
      B_(particles_->B_), F_(particles_->F_), dF_dt_(particles_->dF_dt_),
      F_bending_(particles_->F_bending_), dF_bending_dt_(particles_->dF_bending_dt_),
      width_(particles_->width_),
      b_n0_(particles_->b_n0_), pseudo_b_n_(particles_->pseudo_b_n_),
      dpseudo_b_n_dt_(particles_->dpseudo_b_n_dt_), dpseudo_b_n_d2t_(particles_->dpseudo_b_n_d2t_),
      rotation_b_(particles_->rotation_b_), angular_b_vel_(particles_->angular_b_vel_),
      dangular_b_vel_dt_(particles_->dangular_b_vel_dt_),
      transformation_matrix0_(*particles_->getVariableByName<Matd>("TransformationMatrix")),
      F_b_bending_(particles_->F_b_bending_),
      dF_b_bending_dt_(particles_->dF_b_bending_dt_)
{
}
//=================================================================================================//
BarStressRelaxationFirstHalf::
    BarStressRelaxationFirstHalf(BaseInnerRelation &inner_relation,
                                 int number_of_gaussian_points, bool hourglass_control)
    : BaseBarRelaxation(inner_relation),
      elastic_solid_(particles_->elastic_solid_),
      smoothing_length_(sph_body_.sph_adaptation_->ReferenceSmoothingLength()),
      numerical_damping_scaling_matrix_(Matd::Identity() * smoothing_length_),
      Vol_(particles_->VolumetricMeasures()),
      global_stress_(particles_->global_stress_),
      global_moment_(particles_->global_moment_),
      mid_surface_cauchy_stress_(particles_->mid_surface_cauchy_stress_),
      global_shear_stress_(particles_->global_shear_stress_),
      n_(particles_->n_),
      rho0_(elastic_solid_.ReferenceDensity()),
      inv_rho0_(1.0 / rho0_),
      E0_(elastic_solid_.YoungsModulus()),
      G0_(elastic_solid_.ShearModulus()),
      nu_(elastic_solid_.PoissonRatio()),
      hourglass_control_(hourglass_control),
      number_of_gaussian_points_(number_of_gaussian_points),
      global_b_stress_(particles_->global_b_stress_),
      global_b_moment_(particles_->global_b_moment_),
      global_b_shear_stress_(particles_->global_b_shear_stress_),
      b_n_(particles_->b_n_)
{

    /** Note that, only three-point and five-point Gaussian quadrature rules are defined. */
    switch (number_of_gaussian_points)
    {

    case 9:
        gaussian_point_x = nine_gaussian_points_2d_vector_x;
        gaussian_point_y = nine_gaussian_points_2d_vector_y;
        gaussian_weight_ = nine_gaussian_weights_2d_;
        break;
    default:
        gaussian_point_x = four_gaussian_points_2d_vector_x;
        gaussian_point_y = four_gaussian_points_2d_vector_y;
        gaussian_weight_ = four_gaussian_weights_2d_;
    }
    /** Define the factor of hourglass control algorithm. */
    hourglass_control_factor_ = 0.005;
}
//=================================================================================================//
void BarStressRelaxationFirstHalf::initialization(size_t index_i, Real dt)
{
    // Note that F_[index_i], F_bending_[index_i], dF_dt_[index_i], dF_bending_dt_[index_i]
    // and rotation_[index_i], angular_vel_[index_i], dangular_vel_dt_[index_i], B_[index_i]
    // are defined in local coordinates, while others in global coordinates.
    pos_[index_i] += vel_[index_i] * dt * 0.5;
    rotation_[index_i] += angular_vel_[index_i] * dt * 0.5;
    pseudo_n_[index_i] += dpseudo_n_dt_[index_i] * dt * 0.5;

    rotation_b_[index_i] += angular_b_vel_[index_i] * dt * 0.5;
    pseudo_b_n_[index_i] += dpseudo_b_n_dt_[index_i] * dt * 0.5;

    F_[index_i] += dF_dt_[index_i] * dt * 0.5;
    F_bending_[index_i] += dF_bending_dt_[index_i] * dt * 0.5;
    F_b_bending_[index_i] += dF_b_bending_dt_[index_i] * dt * 0.5;

    Real J = F_[index_i].determinant();
    Matd inverse_F = F_[index_i].inverse();

    rho_[index_i] = rho0_ / J;

    /** Calculate the current normal direction of mid-surface. */
    n_[index_i] = transformation_matrix0_[index_i].transpose() * getNormalFromDeformationGradientTensor(F_[index_i]);
    b_n_[index_i] = transformation_matrix0_[index_i].transpose() * getBinormalFromDeformationGradientTensor(F_[index_i]);

    /** Get transformation matrix from global coordinates to current local coordinates. */
    Matd current_transformation_matrix = getTransformationMatrix(n_[index_i], b_n_[index_i]);

    Matd resultant_stress = Matd::Zero();
    Matd resultant_moment = Matd::Zero();
    Vecd resultant_shear_stress = Vecd::Zero();
    Matd resultant_b_moment = Matd::Zero();
    Vecd resultant_b_shear_stress = Vecd::Zero();

    for (int i = 0; i != number_of_gaussian_points_; ++i)
    {
        Matd F_gaussian_point = F_[index_i] + gaussian_point_y[i] * F_bending_[index_i] * thickness_[index_i] * 0.5 +
                                gaussian_point_x[i] * F_b_bending_[index_i] * width_[index_i] * 0.5;
        Matd dF_gaussian_point_dt = dF_dt_[index_i] + gaussian_point_y[i] * dF_bending_dt_[index_i] * thickness_[index_i] * 0.5 +
                                    gaussian_point_x[i] * dF_b_bending_dt_[index_i] * width_[index_i] * 0.5;
        Matd inverse_F_gaussian_point = F_gaussian_point.inverse();
        Matd current_local_almansi_strain = current_transformation_matrix * transformation_matrix0_[index_i].transpose() * 0.5 *
                                            (Matd::Identity() - inverse_F_gaussian_point.transpose() * inverse_F_gaussian_point) *
                                            transformation_matrix0_[index_i] * current_transformation_matrix.transpose();

        /** correct Almansi strain tensor according to plane stress problem. */
        current_local_almansi_strain = getCorrectedAlmansiStrain(current_local_almansi_strain, nu_);

        /** correct out-plane numerical damping. */
        numerical_damping_scaling_matrix_(1, 1) = width_[i] < smoothing_length_ ? width_[i] : smoothing_length_;
        numerical_damping_scaling_matrix_(2, 2) = thickness_[i] < smoothing_length_ ? thickness_[i] : smoothing_length_;
        Matd cauchy_stress = elastic_solid_.StressCauchy(current_local_almansi_strain, index_i) +
                             current_transformation_matrix * transformation_matrix0_[index_i].transpose() * F_gaussian_point *
                                 elastic_solid_.NumericalDampingRightCauchy(F_gaussian_point, dF_gaussian_point_dt, numerical_damping_scaling_matrix_, index_i) *
                                 F_gaussian_point.transpose() * transformation_matrix0_[index_i] * current_transformation_matrix.transpose() / F_gaussian_point.determinant();

        /** Impose modeling assumptions. */
        cauchy_stress.row(2) *= shear_correction_factor_;
        cauchy_stress.col(2) *= shear_correction_factor_;
        cauchy_stress.row(1) *= shear_correction_factor_;
        cauchy_stress.col(1) *= shear_correction_factor_;

        cauchy_stress(2, 2) = 0.0;
        cauchy_stress(1, 1) = 0.0;
        if (i == 0)
        {
            mid_surface_cauchy_stress_[index_i] = cauchy_stress;
        }

        /** Integrate Cauchy stress along thickness. */
        resultant_stress +=
            0.5 * width_[index_i] * 0.5 * thickness_[index_i] * gaussian_weight_[i] * cauchy_stress;
        resultant_moment +=
            0.5 * width_[index_i] * 0.5 * thickness_[index_i] * gaussian_weight_[i] * ((cauchy_stress)*gaussian_point_y[i] * thickness_[index_i] * 0.5);
        resultant_b_moment +=
            0.5 * width_[index_i] * 0.5 * thickness_[index_i] * gaussian_weight_[i] * ((cauchy_stress)*gaussian_point_x[i] * width_[index_i] * 0.5);
        resultant_shear_stress -=
            0.5 * width_[index_i] * 0.5 * thickness_[index_i] * gaussian_weight_[i] * cauchy_stress.col(2);
        resultant_b_shear_stress -=
            0.5 * width_[index_i] * 0.5 * thickness_[index_i] * gaussian_weight_[i] * cauchy_stress.col(1);

        resultant_stress.row(2) = Vecd::Zero().transpose();
        resultant_stress.col(2) = Vecd::Zero();

        resultant_stress.row(1) = Vecd::Zero().transpose();
        resultant_stress.col(1) = Vecd::Zero();

        resultant_moment.row(2) = Vecd::Zero().transpose();
        resultant_moment.col(2) = Vecd::Zero();

        resultant_b_moment.row(1) = Vecd::Zero().transpose();
        resultant_b_moment.col(1) = Vecd::Zero();

        resultant_shear_stress[1] = 0.0;
        resultant_b_shear_stress[2] = 0.0;
    }

    /** stress and moment in global coordinates for pair interaction */
    global_stress_[index_i] = J * current_transformation_matrix.transpose() *
                              resultant_stress * current_transformation_matrix * transformation_matrix0_[index_i].transpose() *
                              inverse_F.transpose() * transformation_matrix0_[index_i];
    global_moment_[index_i] = J * current_transformation_matrix.transpose() * resultant_moment * current_transformation_matrix *
                              transformation_matrix0_[index_i].transpose() * inverse_F.transpose() * transformation_matrix0_[index_i];
    global_shear_stress_[index_i] = J * current_transformation_matrix.transpose() * resultant_shear_stress;

    global_b_moment_[index_i] = J * current_transformation_matrix.transpose() * resultant_b_moment * current_transformation_matrix *
                                transformation_matrix0_[index_i].transpose() * inverse_F.transpose() * transformation_matrix0_[index_i];
    global_b_shear_stress_[index_i] = J * current_transformation_matrix.transpose() * resultant_b_shear_stress;
}
//=================================================================================================//
void BarStressRelaxationFirstHalf::update(size_t index_i, Real dt)
{
    vel_[index_i] += (force_prior_[index_i] + force_[index_i]) / mass_[index_i] * dt;
    angular_vel_[index_i] += (dangular_vel_dt_[index_i]) * dt;
    angular_b_vel_[index_i] += (dangular_b_vel_dt_[index_i]) * dt;
}
//=================================================================================================//
void BarStressRelaxationSecondHalf::initialization(size_t index_i, Real dt)
{
    pos_[index_i] += vel_[index_i] * dt * 0.5;
    rotation_b_[index_i] += angular_b_vel_[index_i] * dt * 0.5;
    rotation_[index_i] += angular_vel_[index_i] * dt * 0.5;
    Vecd pseudo_n_temp = pseudo_n_[index_i];
    Vecd pseudo_b_n_temp = pseudo_b_n_[index_i];

    pseudo_n_[index_i] = transformation_matrix0_[index_i].transpose() *
                         getVectorAfterThinStructureRotation(local_pseudo_n_0, rotation_[index_i]);

    pseudo_b_n_[index_i] = transformation_matrix0_[index_i].transpose() *
                           getVectorAfterThinStructureRotation(local_pseudo_b_n_0, rotation_b_[index_i]);

    if (dt < 1e-10)
    {
        dpseudo_n_dt_[index_i] = Vecd::Zero();
        dpseudo_b_n_dt_[index_i] = Vecd::Zero();
    }
    else
    {
        dpseudo_n_dt_[index_i] = (pseudo_n_[index_i] - pseudo_n_temp) / (0.5 * dt);
        dpseudo_b_n_dt_[index_i] = (pseudo_b_n_[index_i] - pseudo_b_n_temp) / (0.5 * dt);
    }
}
//=================================================================================================//
void BarStressRelaxationSecondHalf::update(size_t index_i, Real dt)
{
    F_[index_i] += dF_dt_[index_i] * dt * 0.5;
    F_bending_[index_i] += dF_bending_dt_[index_i] * dt * 0.5;
    F_b_bending_[index_i] += dF_b_bending_dt_[index_i] * dt * 0.5;
}
//=================================================================================================//
ConstrainBarBodyRegion::
    ConstrainBarBodyRegion(BodyPartByParticle &body_part)
    : BaseLocalDynamics<BodyPartByParticle>(body_part), BarDataSimple(sph_body_),
      vel_(*particles_->getVariableByName<Vecd>("Velocity")),
      angular_vel_(particles_->angular_vel_),
      angular_b_vel_(particles_->angular_b_vel_) {}
//=================================================================================================//
void ConstrainBarBodyRegion::update(size_t index_i, Real dt)
{
    vel_[index_i] = Vecd::Zero();
    angular_vel_[index_i] = Vecd::Zero();
    angular_b_vel_[index_i] = Vecd::Zero();
}
//=================================================================================================//
ConstrainBarBodyRegionAlongAxis::ConstrainBarBodyRegionAlongAxis(BodyPartByParticle &body_part, int axis)
    : BaseLocalDynamics<BodyPartByParticle>(body_part), BarDataSimple(sph_body_),
      axis_(axis), pos_(particles_->ParticlePositions()),
      pos0_(particles_->pos0_),
      vel_(*particles_->getVariableByName<Vecd>("Velocity")),
      force_(*particles_->getVariableByName<Vecd>("Force")),
      rotation_(particles_->rotation_), angular_vel_(particles_->angular_vel_),
      dangular_vel_dt_(particles_->dangular_vel_dt_), rotation_b_(particles_->rotation_b_),
      angular_b_vel_(particles_->angular_b_vel_), dangular_b_vel_dt_(particles_->dangular_b_vel_dt_) {}
//=================================================================================================//
void ConstrainBarBodyRegionAlongAxis::update(size_t index_i, Real dt)
{
    vel_[index_i] = Vecd::Zero();
}
//=================================================================================================//
DistributingPointForcesToBar::
    DistributingPointForcesToBar(SPHBody &sph_body, std::vector<Vecd> point_forces,
                                 std::vector<Vecd> reference_positions, Real time_to_full_external_force,
                                 Real particle_spacing_ref, Real h_spacing_ratio)
    : LocalDynamics(sph_body), BarDataSimple(sph_body),
      point_forces_(point_forces), reference_positions_(reference_positions),
      time_to_full_external_force_(time_to_full_external_force),
      particle_spacing_ref_(particle_spacing_ref), h_spacing_ratio_(h_spacing_ratio),
      pos0_(particles_->pos0_),
      force_prior_(*particles_->getVariableByName<Vecd>("ForcePrior")),
      thickness_(particles_->thickness_)
{
    for (size_t i = 0; i < point_forces_.size(); i++)
    {
        weight_.push_back(StdLargeVec<Real>(0.0));
        time_dependent_point_forces_.push_back(Vecd::Zero());
        sum_of_weight_.push_back(0.0);
        particles_->registerVariable(weight_[i], "Weight_" + std::to_string(i));
    }

    getWeight(); // TODO: should be revised and parallelized, using SimpleDynamics
}
//=================================================================================================//
void DistributingPointForcesToBar::getWeight()
{
    Kernel *kernel_ = sph_body_.sph_adaptation_->getKernel();
    Real reference_smoothing_length = sph_body_.sph_adaptation_->ReferenceSmoothingLength();
    Real smoothing_length = h_spacing_ratio_ * particle_spacing_ref_;
    Real h_ratio = reference_smoothing_length / smoothing_length;
    Real cutoff_radius_sqr = pow(2.0 * smoothing_length, 2);
    for (size_t i = 0; i < point_forces_.size(); ++i)
    {
        sum_of_weight_[i] = 0.0;
        for (size_t index = 0; index < particles_->total_real_particles_; ++index)
        {
            weight_[i][index] = 0.0;
            Vecd displacement = reference_positions_[i] - pos0_[index];
            if (displacement.squaredNorm() <= cutoff_radius_sqr)
            {
                weight_[i][index] = kernel_->W(h_ratio, displacement.norm(), displacement);
                sum_of_weight_[i] += weight_[i][index];
            }
        }
    }
}
//=================================================================================================//
void DistributingPointForcesToBar::setupDynamics(Real dt)
{
    Real current_time = GlobalStaticVariables::physical_time_;
    for (size_t i = 0; i < point_forces_.size(); ++i)
    {
        time_dependent_point_forces_[i] = current_time < time_to_full_external_force_
                                              ? current_time * point_forces_[i] / time_to_full_external_force_
                                              : point_forces_[i];
    }
}
//=================================================================================================//
void DistributingPointForcesToBar::update(size_t index_i, Real dt)
{
    force_prior_[index_i] = Vecd::Zero();
    for (size_t i = 0; i < point_forces_.size(); ++i)
    {
        Vecd force = weight_[i][index_i] / (sum_of_weight_[i] + TinyReal) * time_dependent_point_forces_[i];
        force_prior_[index_i] += force;
    }
}
//=================================================================================================//
} // namespace slender_structure_dynamics
} // namespace SPH