#ifndef __PE_PHYSICSCOMPONENT_H__
#define __PE_PHYSICSCOMPONENT_H__


#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"

namespace PE {
namespace Components {

	struct PhysicsComponent : public Component
	{
		PE_DECLARE_CLASS(PhysicsComponent);

		PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Handle hMeshInstance, Handle hSceneNode);
		PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Vector3 pMin, Vector3 pMax, Handle hSceneNode, bool isStatic);
		virtual ~PhysicsComponent() {}

		virtual void addDefaultComponents();
		void UpdatePosition();
		void GravityHandler();
		void DrawBox();
		void DrawStaticBox();
		void OnCollision(PhysicsComponent* pPC_other);
		void Collision_SphereBox(PhysicsComponent* pPC_other);
		float SqDistPointAABB(PhysicsComponent* pPC_other);
		Vector3 ClosestPtPointAABB(PhysicsComponent* pPC_other);

		Handle m_hMesh; // handle to mesh
		Handle m_hSN; // handle to scene node
		Handle m_hMinst; // handle to mesh instance

		Array<Vector3> m_aabb;
		Matrix4x4 m_base;
		Vector3 m_tarCorrect;
		Vector3 m_gravity;
		float m_radius;
		bool m_isStatic;
		bool m_onGround;
	};

};
};

#endif