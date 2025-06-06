// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2025 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef PXS_RIGID_BODY_H
#define PXS_RIGID_BODY_H

#include "PxvDynamics.h"
#include "CmSpatialVector.h"
#include "foundation/PxMutex.h"

namespace physx
{
struct PxsCCDBody;

PX_ALIGN_PREFIX(16)
class PxsRigidBody
{
	public:

	enum PxsRigidBodyFlag
	{
		eFROZEN					=	1 << 0,		//This flag indicates that the stabilization is enabled and the body is
												//"frozen". By "frozen", we mean that the body's transform is unchanged
												//from the previous frame. This permits various optimizations.
		eFREEZE_THIS_FRAME		=	1 << 1,
		eUNFREEZE_THIS_FRAME	=	1 << 2,
		eACTIVATE_THIS_FRAME	=	1 << 3,
		eDEACTIVATE_THIS_FRAME	=	1 << 4,
		// PT: this flag is now only used on the GPU. For the CPU the data is now stored directly in PxsBodyCore.
		eDISABLE_GRAVITY_GPU	=	1 << 5,
		eSPECULATIVE_CCD		=	1 << 6,
		eENABLE_GYROSCOPIC		=	1 << 7,
		eRETAIN_ACCELERATION	=	1 << 8,
		eFIRST_BODY_COPY_GPU	=	1 << 9, // Flag to raise to indicate that the body is DMA'd to the GPU for the first time
		eVELOCITY_COPY_GPU		=	1 << 10	 // Flag to raise to indicate that linear and angular velocities should be  DMA'd to the GPU
	};

	PX_FORCE_INLINE						PxsRigidBody(PxsBodyCore* core, PxReal freeze_count) :
											mLastTransform			(core->body2World),
											mInternalFlags			(0),
											mSolverIterationCounts	(core->solverIterationCounts),
											mCCD					(NULL),
											mCore					(core),
											mSleepLinVelAcc			(PxVec3(0.0f)),
											mFreezeCount			(freeze_count),
											mSleepAngVelAcc			(PxVec3(0.0f)),
											mAccelScale				(1.0f)
																	{}

	PX_FORCE_INLINE						~PxsRigidBody()																			{}

	PX_FORCE_INLINE	const PxTransform&	getPose()							const	{ PX_ASSERT(mCore->body2World.isSane()); return mCore->body2World;				}

	PX_FORCE_INLINE	const PxVec3&		getLinearVelocity()					const	{ PX_ASSERT(mCore->linearVelocity.isFinite()); return mCore->linearVelocity;	}
	PX_FORCE_INLINE	const PxVec3&		getAngularVelocity()				const	{ PX_ASSERT(mCore->angularVelocity.isFinite()); return mCore->angularVelocity;	}
	
	PX_FORCE_INLINE	void	 			setVelocity(const PxVec3& linear,
													const PxVec3& angular)			{ PX_ASSERT(linear.isFinite()); PX_ASSERT(angular.isFinite());
																					  mCore->linearVelocity = linear;
																					  mCore->angularVelocity = angular; }
	PX_FORCE_INLINE	void				setLinearVelocity(const PxVec3& linear)		{ PX_ASSERT(linear.isFinite()); mCore->linearVelocity = linear;		}
	PX_FORCE_INLINE	void				setAngularVelocity(const PxVec3& angular)	{ PX_ASSERT(angular.isFinite()); mCore->angularVelocity = angular;	}

	PX_FORCE_INLINE	void				constrainLinearVelocity();
	PX_FORCE_INLINE	void				constrainAngularVelocity();

	PX_FORCE_INLINE	PxU32				getIterationCounts()						{ return mCore->solverIterationCounts;	}
	PX_FORCE_INLINE	PxReal				getReportThreshold()				const	{ return mCore->contactReportThreshold;	}

	PX_FORCE_INLINE	const PxTransform&	getLastCCDTransform()				const	{ return mLastTransform;				}
	PX_FORCE_INLINE	void				saveLastCCDTransform()						{ mLastTransform = mCore->body2World;	}

	PX_FORCE_INLINE	bool				isKinematic()						const	{ return mCore->inverseMass == 0.0f;	}
						
	PX_FORCE_INLINE	void				setPose(const PxTransform& pose)			{ mCore->body2World = pose;			}
	PX_FORCE_INLINE	void				setPosition(const PxVec3& position)			{ mCore->body2World.p = position;	}
	PX_FORCE_INLINE	PxReal				getInvMass()						const	{ return mCore->inverseMass;		}
	PX_FORCE_INLINE	PxVec3				getInvInertia()						const	{ return mCore->inverseInertia;		}
	PX_FORCE_INLINE	PxReal				getMass()							const	{ return 1.0f/mCore->inverseMass;	}
	PX_FORCE_INLINE	PxVec3				getInertia()						const	{ return PxVec3(1.0f/mCore->inverseInertia.x,
																									1.0f/mCore->inverseInertia.y,
																									1.0f/mCore->inverseInertia.z);	}
	PX_FORCE_INLINE	PxsBodyCore&		getCore()									{ return *mCore;	}
	PX_FORCE_INLINE	const PxsBodyCore&	getCore()							const	{ return *mCore;	}

	PX_FORCE_INLINE	PxU32				isActivateThisFrame()				const	{ return PxU32(mInternalFlags & eACTIVATE_THIS_FRAME);		}
	PX_FORCE_INLINE	PxU32				isDeactivateThisFrame()				const	{ return PxU32(mInternalFlags & eDEACTIVATE_THIS_FRAME);	}
	PX_FORCE_INLINE	PxU32				isFreezeThisFrame()					const	{ return PxU32(mInternalFlags & eFREEZE_THIS_FRAME);		}
	PX_FORCE_INLINE	PxU32				isUnfreezeThisFrame()				const	{ return PxU32(mInternalFlags & eUNFREEZE_THIS_FRAME);		}
	PX_FORCE_INLINE	void				clearFreezeFlag()							{ mInternalFlags &= ~eFREEZE_THIS_FRAME;					}
	PX_FORCE_INLINE	void				clearUnfreezeFlag()							{ mInternalFlags &= ~eUNFREEZE_THIS_FRAME;					}
	PX_FORCE_INLINE	void				clearAllFrameFlags()						{ mInternalFlags &= ~(eFREEZE_THIS_FRAME | eUNFREEZE_THIS_FRAME | eACTIVATE_THIS_FRAME | eDEACTIVATE_THIS_FRAME);	}

	PX_FORCE_INLINE	void				resetSleepFilter()							{ mSleepAngVelAcc = mSleepLinVelAcc = PxVec3(0.0f);			}

	// PT: implemented in PxsCCD.cpp:
					void				advanceToToi(PxReal toi, PxReal dt, bool clip);
					void				advancePrevPoseToToi(PxReal toi);
//					PxTransform			getAdvancedTransform(PxReal toi) const;
					Cm::SpatialVector	getPreSolverVelocities() const;

					PxTransform			mLastTransform;			//28

					PxU16				mInternalFlags;			//30
					PxU16				mSolverIterationCounts;	//32

					PxsCCDBody*			mCCD;					//40	// only valid during CCD	

					PxsBodyCore*		mCore;					//48

					PxVec3				mSleepLinVelAcc;		//60
					PxReal				mFreezeCount;			//64
	   
					PxVec3				mSleepAngVelAcc;		//76
					PxReal				mAccelScale;			//80
}
PX_ALIGN_SUFFIX(16);
PX_COMPILE_TIME_ASSERT(0 == (sizeof(PxsRigidBody) & 0x0f));

void PxsRigidBody::constrainLinearVelocity()
{
	const PxU32 lockFlags = mCore->lockFlags;
	if(lockFlags)
	{
		if(lockFlags & PxRigidDynamicLockFlag::eLOCK_LINEAR_X)
			mCore->linearVelocity.x = 0.0f;
		if(lockFlags & PxRigidDynamicLockFlag::eLOCK_LINEAR_Y)
			mCore->linearVelocity.y = 0.0f;
		if(lockFlags & PxRigidDynamicLockFlag::eLOCK_LINEAR_Z)
			mCore->linearVelocity.z = 0.0f;
	}
}

void PxsRigidBody::constrainAngularVelocity()
{
	const PxU32 lockFlags = mCore->lockFlags;
	if(lockFlags)
	{
		if(lockFlags & PxRigidDynamicLockFlag::eLOCK_ANGULAR_X)
			mCore->angularVelocity.x = 0.0f;
		if(lockFlags & PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y)
			mCore->angularVelocity.y = 0.0f;
		if(lockFlags & PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z)
			mCore->angularVelocity.z = 0.0f;
	}
}


struct PxsRigidBodyExternalAcceleration
{
	PxVec3 linearAcceleration;
	PxVec3 angularAcceleration;

	PxsRigidBodyExternalAcceleration() : linearAcceleration(PxVec3(0.0f)), angularAcceleration(PxVec3(0.0f))
	{ }

	PxsRigidBodyExternalAcceleration(const PxVec3& linearAcc, const PxVec3& angularAcc) : 
		linearAcceleration(linearAcc), angularAcceleration(angularAcc)
	{ }
};

struct PxsExternalAccelerationProvider
{
	PxArray<PxsRigidBodyExternalAcceleration> mAccelerations;
	PxMutex mLock;
	volatile PxU32 mArraySize; //Required because of multi threading

	PxsExternalAccelerationProvider() : mArraySize(0)
	{ }

	PX_FORCE_INLINE void setValue(PxsRigidBodyExternalAcceleration& value, PxU32 index, PxU32 maxNumBodies)
	{
		if (mArraySize < maxNumBodies)
		{
			PxMutex::ScopedLock lock(mLock);
			if (mArraySize < maxNumBodies) //Test again because only after the lock we are sure that only one thread is active at a time
			{
				mAccelerations.resize(maxNumBodies);
				mArraySize = maxNumBodies; //Only now the resize is complete - mAccelerations.size() might already change before the array actually allocated the new memory
			}
		}
		PX_ASSERT(index < mArraySize);
		mAccelerations[index] = value;
	}

	PX_FORCE_INLINE bool hasAccelerations() const
	{
		return mArraySize > 0;
	}

	PX_FORCE_INLINE const PxsRigidBodyExternalAcceleration& get(PxU32 index) const
	{
		PX_ASSERT(index < mArraySize);
		return mAccelerations[index];
	}

	PX_FORCE_INLINE void clearAll()
	{
		if (mArraySize > 0)
		{
			mAccelerations.clear();
			mArraySize = 0;
		}
		else if (mAccelerations.capacity() > 0)
			mAccelerations.reset();
	}
};

}

#endif
