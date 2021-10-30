#include "PhysicsActor.h"

using namespace physx;

void PhysicsActor::UpdateTransform()
{
	if (Actor->is<PxRigidStatic>())
	{
		auto&		Transform = Entity.GetComponent<CoreComponent>().Transform;
		PxTransform Pose(ToPxVec3(Transform.Position), ToPxQuat(Transform.Orientation));
		Actor->setGlobalPose(Pose);
	}
	else
	{
		auto& Transform = Entity.GetComponent<CoreComponent>().Transform;

		PxTransform GlobalPose = Actor->getGlobalPose();
		Transform.Position	   = ToXMFloat3(GlobalPose.p);
		Transform.Orientation  = ToXMFloat4(GlobalPose.q);
	}
}
