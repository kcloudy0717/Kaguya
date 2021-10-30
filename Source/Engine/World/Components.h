#pragma once
#include <span>
#include <string>
#include <string_view>
#include <DirectXMath.h>
#include <PxPhysicsAPI.h>

namespace physx
{
class PxRigidStatic;
class PxRigidDynamic;
class PxController;
} // namespace physx

#include "Attribute.h"

#include "Components/CoreComponent.h"
#include "Components/CameraComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CharacterControllerComponent.h"
#include "Components/NativeScriptComponent.h"

#include "Components/BoxColliderComponent.h"
#include "Components/CapsuleColliderComponent.h"
#include "Components/MeshColliderComponent.h"

#include "Components/StaticRigidBodyComponent.h"
#include "Components/DynamicRigidBodyComponent.h"
