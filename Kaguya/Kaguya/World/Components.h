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

// Core
#include "Components/Component.h"
#include "Components/Tag.h"
#include "Components/Transform.h"
#include "Components/Camera.h"
#include "Components/Light.h"
#include "Components/MeshFilter.h"
#include "Components/MeshRenderer.h"
#include "Components/CharacterController.h"
#include "Components/NativeScript.h"

// Rigid Body

// struct RigidBody : Component
//{
//	RigidBody() noexcept = default;
//
//	enum EType
//	{
//		Static,
//		Dynamic
//	} Type = EType::Static;
//	physx::PxRigidActor* Actor;
//};

#include "Components/RigidBody/StaticRigidBody.h"
#include "Components/RigidBody/DynamicRigidBody.h"

// Collider
#include "Components/Collider/BoxCollider.h"
#include "Components/Collider/CapsuleCollider.h"
#include "Components/Collider/MeshCollider.h"
