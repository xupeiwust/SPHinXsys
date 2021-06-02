/* -------------------------------------------------------------------------*
*								SPHinXsys									*
* --------------------------------------------------------------------------*
* SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle	*
* Hydrodynamics for industrial compleX systems. It provides C++ APIs for	*
* physical accurate simulation and aims to model coupled industrial dynamic *
* systems including fluid, solid, multi-body dynamics and beyond with SPH	*
* (smoothed particle hydrodynamics), a meshless computational method using	*
* particle discretization.													*
*																			*
* SPHinXsys is partially funded by German Research Foundation				*
* (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1				*
* and HU1527/12-1.															*
*                                                                           *
* Portions copyright (c) 2017-2020 Technical University of Munich and		*
* the authors' affiliations.												*
*                                                                           *
* Licensed under the Apache License, Version 2.0 (the "License"); you may   *
* not use this file except in compliance with the License. You may obtain a *
* copy of the License at http://www.apache.org/licenses/LICENSE-2.0.        *
*                                                                           *
* --------------------------------------------------------------------------*/
/**
* @file 	solid_dynamics.h
* @brief 	Here, we define the algorithm classes for solid dynamics. 
* @details 	We consider here a weakly compressible solids.   
* @author	Luhui Han, Chi ZHang and Xiangyu Hu
*/

#ifndef SOLID_DYNAMICS_H
#define SOLID_DYNAMICS_H


#include "all_particle_dynamics.h"
#include "elastic_solid.h"
#include "weakly_compressible_fluid.h"
#include "base_kernel.h"
#include "all_fluid_dynamics.h"

namespace SPH
{
	template<int DataTypeIndex, typename VariableType>
	class BodySummation;
	template<int DataTypeIndex, typename VariableType>
	class BodyMoment;

	namespace solid_dynamics
	{
		//----------------------------------------------------------------------
		//		for general solid dynamics 
		//----------------------------------------------------------------------
		typedef DataDelegateSimple<SolidBody, SolidParticles, Solid> SolidDataSimple;
		typedef DataDelegateInner<SolidBody, SolidParticles, Solid> SolidDataInner;
		typedef DataDelegateContact<SolidBody, SolidParticles, Solid, SolidBody, SolidParticles, Solid> ContactDynamicsData;

		/**
		 * @class SolidDynamicsInitialCondition
		 * @brief  set initial condition for solid fluid body
		 * This is a abstract class to be override for case specific initial conditions.
		 */
		class SolidDynamicsInitialCondition :
			public ParticleDynamicsSimple, public SolidDataSimple
		{
		public:
			SolidDynamicsInitialCondition(SolidBody* body) :
				ParticleDynamicsSimple(body), SolidDataSimple(body) {};
			virtual ~SolidDynamicsInitialCondition() {};
		};

		/**
		* @class ContactDensitySummation
		* @brief Computing the summation density due to solid-solid contact model.
		*/
		class ContactDensitySummation :
			public PartInteractionDynamicsByParticle, public ContactDynamicsData
		{
		public:
			ContactDensitySummation(SolidContactBodyRelation* solid_body_contact_relation);
			virtual ~ContactDensitySummation() {};
		protected:
			StdLargeVec<Real>& mass_, & contact_density_;
			StdVec<StdLargeVec<Real>*> contact_mass_;

			virtual void Interaction(size_t index_i, Real dt = 0.0) override;
		};

		/**
		* @class ContactForceAcceleration
		* @brief Computing the contact force.
		*/
		class ContactForce :
			public PartInteractionDynamicsByParticle, public ContactDynamicsData
		{
		public:
			ContactForce(SolidContactBodyRelation* solid_body_contact_relation);
			virtual ~ContactForce() {};
		protected:
			StdLargeVec<Real>& contact_density_, & Vol_, & mass_;
			StdLargeVec<Vecd>& dvel_dt_others_, & contact_force_;
			StdVec<StdLargeVec<Real>*> contact_contact_density_, contact_Vol_;

			virtual void Interaction(size_t index_i, Real dt = 0.0) override;
		};

	   /**
		* @class CorrectConfiguration
		* @brief obtain the corrected initial configuration in strong form
		*/
		class CorrectConfiguration :
			public InteractionDynamics, public SolidDataInner
		{
		public:
			CorrectConfiguration(BaseInnerBodyRelation* body_inner_relation);
			virtual ~CorrectConfiguration() {};
		protected:
			StdLargeVec<Real>& Vol_;
			StdLargeVec<Matd>& B_;
			virtual void Interaction(size_t index_i, Real dt = 0.0) override;
		};

		/**
		 * @class ConstrainSolidBodyRegion
		 * @brief Constrain a solid body part with prescribed motion.
		 * Note the average values for FSI are prescirbed also.
		 */
		class ConstrainSolidBodyRegion :
			public PartSimpleDynamicsByParticle, public SolidDataSimple
		{
		public:
			ConstrainSolidBodyRegion(SPHBody* body, BodyPartByParticle* body_part);
			virtual ~ConstrainSolidBodyRegion() {};
		protected:
			StdLargeVec<Vecd>& pos_n_, & pos_0_;
			StdLargeVec<Vecd>& n_, & n_0_;
			StdLargeVec<Vecd>& vel_n_, & dvel_dt_, & vel_ave_, & dvel_dt_ave_;
			virtual Vecd getDisplacement(Vecd& pos_0, Vecd& pos_n) { return pos_n; };
			virtual Vecd getVelocity(Vecd& pos_0, Vecd& pos_n, Vecd& vel_n) { return Vecd(0); };
			virtual Vecd getAcceleration(Vecd& pos_0, Vecd& pos_n, Vecd& dvel_dt) { return Vecd(0); };
			virtual SimTK::Rotation getBodyRotation(Vecd& pos_0, Vecd& pos_n, Vecd& dvel_dt) { return SimTK::Rotation(); }
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		};


		/**
		 * @class ConstrainSolidBodyRegionVelocity
		 * @brief Constrain the velocity of a solid body part.
		 */
		class ConstrainSolidBodyRegionVelocity : public ConstrainSolidBodyRegion
		{
		public:
			ConstrainSolidBodyRegionVelocity(SPHBody* body, BodyPartByParticle* body_part,
				Vecd constrained_direction = Vecd(0)) :
				solid_dynamics::ConstrainSolidBodyRegion(body, body_part),
				constrain_matrix_(Matd(1.0))
			{
				for (int k = 0; k != Dimensions; ++k) 
					constrain_matrix_[k][k] = constrained_direction[k];
			};
			virtual ~ConstrainSolidBodyRegionVelocity() {};
		protected:
			Matd constrain_matrix_;
			virtual Vecd getVelocity(Vecd& pos_0, Vecd& pos_n, Vecd& vel_n)
			{
				return constrain_matrix_ * vel_n;
			};
		};

		/**
		 * @class SoftConstrainSolidBodyRegion
		 * @brief Soft the constrain of a solid body part
		 */
		class SoftConstrainSolidBodyRegion :
			public PartInteractionDynamicsByParticleWithUpdate,
			public SolidDataInner
		{
		public:
			SoftConstrainSolidBodyRegion(BaseInnerBodyRelation* body_inner_relation, BodyPartByParticle* body_part);
			virtual ~SoftConstrainSolidBodyRegion() {};
		protected:
			StdLargeVec<Real>& Vol_;
			StdLargeVec<Vecd>& vel_n_, & dvel_dt_, & vel_ave_, & dvel_dt_ave_;
			StdLargeVec<Vecd>& vel_temp_, & dvel_dt_temp_;
			virtual void Interaction(size_t index_i, Real dt = 0.0) override;
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		};

		/**
		 * @class ClampConstrainSolidBodyRegion
		 * @brief Constrain a solid body part with prescribed motion and smoothing to mimic the clamping effect.
		 */
		class ClampConstrainSolidBodyRegion : public ParticleDynamics<void>
		{
		public:
			ConstrainSolidBodyRegion* constrianing_;
			SoftConstrainSolidBodyRegion* softing_;

			ClampConstrainSolidBodyRegion(BaseInnerBodyRelation* body_inner_relation, BodyPartByParticle* body_part);
			virtual ~ClampConstrainSolidBodyRegion() {};

			virtual void exec(Real dt = 0.0) override;
			virtual void parallel_exec(Real dt = 0.0) override;
		};

		/**
		 * @class ConstrainSolidBodyMassCenter
		 * @brief Constrain the mass center of a solid body.
		 */
		class ConstrainSolidBodyMassCenter : 
			public ParticleDynamicsSimple, public SolidDataSimple
		{
		public:
			ConstrainSolidBodyMassCenter(SPHBody* body, Vecd constrain_direction = Vecd(1.0));
			virtual ~ConstrainSolidBodyMassCenter() {};
		protected:
			virtual void setupDynamics(Real dt = 0.0) override;
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		private:
			Real total_mass_;
			Matd correction_matrix_;
			Vecd velocity_correction_;
			StdLargeVec<Vecd>& vel_n_;
			BodyMoment<indexVector, Vecd>* compute_total_momentum_;
		};

		/**@class ImposeExternalForce
		 * @brief impose external force on a solid body part
		 * by add extra acceleration
		 */
		class ImposeExternalForce :
			public PartSimpleDynamicsByParticle, public SolidDataSimple
		{
		public:
			ImposeExternalForce(SolidBody* body, SolidBodyPartForSimbody* body_part);
			virtual ~ImposeExternalForce() {};
		protected:
			StdLargeVec<Vecd>& pos_0_, & vel_n_, & vel_ave_;
			/**
			 * @brief acceleration will be specified by the application
			 */
			virtual Vecd getAcceleration(Vecd& pos) = 0;
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		};
		/**
		* @class SpringDamperConstraintParticleWise
		* @brief Exerts spring force and damping force in the form of acceleration to each particle.
		* The spring force is calculated based on the difference from the particle's initial position.
		* The damping force is calculated based on the particle's current velocity.
		*/
		class SpringDamperConstraintParticleWise 
			: public ParticleDynamicsSimple, public SolidDataSimple
		{
		public:
			SpringDamperConstraintParticleWise(SolidBody* body, Vecd stiffness, Real damping_ratio = 0.01);
			virtual ~SpringDamperConstraintParticleWise() {};
		protected:
			StdLargeVec<Real>& mass_;
			StdLargeVec<Vecd>& pos_n_,& pos_0_,& vel_n_,& dvel_dt_others_;
			Vecd stiffness_;
			// damping component parallel to the spring force component
			// damping coefficient = stiffness_ * damping_ratio_
			Real damping_ratio_;
			virtual void setupDynamics(Real dt = 0.0) override;
			virtual Vecd getAcceleration(Vecd& disp, Real mass);
			virtual Vecd getDampingForce(size_t index_i, Real mass);
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		};
		/**
		* @class AccelerationForBodyPartInBoundingBox
		* @brief Adds acceleration to the part of the body that's inside a bounding box
		*/
		class AccelerationForBodyPartInBoundingBox 
			: public ParticleDynamicsSimple, public SolidDataSimple
		{
		public:
			AccelerationForBodyPartInBoundingBox(SolidBody* body, BoundingBox* bounding_box, Vecd acceleration);
			virtual ~AccelerationForBodyPartInBoundingBox() {};
		protected:
			StdLargeVec<Vecd>& pos_n_,& dvel_dt_others_;
			BoundingBox* bounding_box_;
			Vecd acceleration_;
			virtual void setupDynamics(Real dt = 0.0) override;
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		};

		//----------------------------------------------------------------------
		//		for elastic solid dynamics 
		//----------------------------------------------------------------------
		typedef DataDelegateSimple<SolidBody, ElasticSolidParticles, ElasticSolid> ElasticSolidDataSimple;
		typedef DataDelegateInner<SolidBody, ElasticSolidParticles, ElasticSolid> ElasticSolidDataInner;

		/**
		 * @class ElasticDynamicsInitialCondition
		 * @brief  set initial condition for a solid body with different material
		 * This is a abstract class to be override for case specific initial conditions.
		 */
		class ElasticDynamicsInitialCondition : 
			public ParticleDynamicsSimple, public ElasticSolidDataSimple
		{
		public:
			ElasticDynamicsInitialCondition(SolidBody *body);
			virtual ~ElasticDynamicsInitialCondition() {};
		protected:
			StdLargeVec<Vecd>& pos_n_, & vel_n_;
		};

		/**
		* @class UpdateElasticNormalDirection
		* @brief update particle normal directions for elastic solid
		*/
		class UpdateElasticNormalDirection :
			public ParticleDynamicsSimple, public ElasticSolidDataSimple
		{
		public:
			explicit UpdateElasticNormalDirection(SolidBody *elastic_body);
			virtual ~UpdateElasticNormalDirection() {};
		protected:
			StdLargeVec<Vecd>& n_, & n_0_;
			StdLargeVec<Matd>& F_;
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		};

		/**
		* @class AcousticTimeStepSize
		* @brief Computing the acoustic time step size
		* computing time step size
		*/
		class AcousticTimeStepSize :
			public ParticleDynamicsReduce<Real, ReduceMin>,
			public ElasticSolidDataSimple
		{
		public:
			explicit AcousticTimeStepSize(SolidBody* body);
			virtual ~AcousticTimeStepSize() {};
		protected:
			StdLargeVec<Vecd>& vel_n_, & dvel_dt_;
			Real smoothing_length_;
			Real ReduceFunction(size_t index_i, Real dt = 0.0) override;
		};

		/**
		* @function getSmallestTimeStepAmongSolidBodies
		* @brief computing smallest time step to use in a simulation
		*/
		Real getSmallestTimeStepAmongSolidBodies(SPHBodyVector solid_bodies);

		/**
		* @class DeformationGradientTensorBySummation
		* @brief computing deformation gradient tensor by summation
		*/
		class DeformationGradientTensorBySummation :
			public InteractionDynamics, public ElasticSolidDataInner
		{
		public:
			DeformationGradientTensorBySummation(BaseInnerBodyRelation* body_inner_relation);
			virtual ~DeformationGradientTensorBySummation() {};
		protected:
			StdLargeVec<Real>& Vol_;
			StdLargeVec<Vecd>& pos_n_;
			StdLargeVec<Matd>& B_, & F_;
			virtual void Interaction(size_t index_i, Real dt = 0.0) override;
		};

		/**
		* @class BaseElasticRelaxation
		* @brief base class for elastic relaxation
		*/
		class BaseElasticRelaxation
			: public ParticleDynamics1Level, public ElasticSolidDataInner
		{
		public:
			BaseElasticRelaxation(BaseInnerBodyRelation* body_inner_relation);
			virtual ~BaseElasticRelaxation() {};
		protected:
			StdLargeVec<Real>& Vol_, & rho_n_, & mass_;
			StdLargeVec<Vecd>& pos_n_, & vel_n_, & dvel_dt_;
			StdLargeVec<Matd>& B_, & F_, & dF_dt_;
		};

		/**
		* @class StressRelaxationFirstHalf
		* @brief computing stress relaxation process by verlet time stepping
		* This is the first step
		*/
		class StressRelaxationFirstHalf : public BaseElasticRelaxation
		{
		public:
			StressRelaxationFirstHalf(BaseInnerBodyRelation* body_inner_relation);
			virtual ~StressRelaxationFirstHalf() {};
		protected:
			Real rho_0_, inv_rho_0_;
			StdLargeVec<Vecd>& dvel_dt_others_, & force_from_fluid_;
			StdLargeVec<Matd>& stress_PK1_, & corrected_stress_;
			Real numerical_viscosity_;

			virtual void Initialization(size_t index_i, Real dt = 0.0) override;
			virtual void Interaction(size_t index_i, Real dt = 0.0) override;
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		};

		/**
		* @class StressRelaxationSecondHalf
		* @brief computing stress relaxation process by verlet time stepping
		* This is the second step
		*/
		class StressRelaxationSecondHalf : public BaseElasticRelaxation
		{
		public:
			StressRelaxationSecondHalf(BaseInnerBodyRelation* body_inner_relation) :
				BaseElasticRelaxation(body_inner_relation) {};
			virtual ~StressRelaxationSecondHalf() {};
		protected:
			virtual void Initialization(size_t index_i, Real dt = 0.0) override;
			virtual void Interaction(size_t index_i, Real dt = 0.0) override;
			virtual void Update(size_t index_i, Real dt = 0.0) override;
		};

		/**
		 * @class ConstrainSolidBodyPartBySimBody
		 * @brief Constrain a solid body part from the motion
		 * computed from Simbody.
		 */
		class ConstrainSolidBodyPartBySimBody : public ConstrainSolidBodyRegion
		{
		public:
			ConstrainSolidBodyPartBySimBody(SolidBody* body,
				SolidBodyPartForSimbody* body_part,
				SimTK::MultibodySystem& MBsystem,
				SimTK::MobilizedBody& mobod,
				SimTK::Force::DiscreteForces& force_on_bodies,
				SimTK::RungeKuttaMersonIntegrator& integ);
			virtual ~ConstrainSolidBodyPartBySimBody() {};
		protected:
			SimTK::MultibodySystem& MBsystem_;
			SimTK::MobilizedBody& mobod_;
			SimTK::Force::DiscreteForces& force_on_bodies_;
			SimTK::RungeKuttaMersonIntegrator& integ_;
			const SimTK::State* simbody_state_;
			Vec3d initial_mobod_origin_location_;

			virtual void setupDynamics(Real dt = 0.0) override;
			void virtual Update(size_t index_i, Real dt = 0.0) override;
		};

		/**
		 * @class TotalForceOnSolidBodyPartForSimBody
		 * @brief Compute the force acting on the solid body part
		 * for applying to simbody forces latter
		 */
		class TotalForceOnSolidBodyPartForSimBody :
			public PartDynamicsByParticleReduce<SimTK::SpatialVec, ReduceSum<SimTK::SpatialVec>>,
			public SolidDataSimple
		{
		public:
			TotalForceOnSolidBodyPartForSimBody(SolidBody* body,
				SolidBodyPartForSimbody* body_part,
				SimTK::MultibodySystem& MBsystem,
				SimTK::MobilizedBody& mobod,
				SimTK::Force::DiscreteForces& force_on_bodies,
				SimTK::RungeKuttaMersonIntegrator& integ);
			virtual ~TotalForceOnSolidBodyPartForSimBody() {};
		protected:
			StdLargeVec<Vecd>& force_from_fluid_, & contact_force_, & pos_n_;
			SimTK::MultibodySystem& MBsystem_;
			SimTK::MobilizedBody& mobod_;
			SimTK::Force::DiscreteForces& force_on_bodies_;
			SimTK::RungeKuttaMersonIntegrator& integ_;
			const SimTK::State* simbody_state_;
			Vec3d current_mobod_origin_location_;

			virtual void SetupReduce() override;
			virtual SimTK::SpatialVec ReduceFunction(size_t index_i, Real dt = 0.0) override;
		};		
	}
}
#endif //SOLID_DYNAMICS_H