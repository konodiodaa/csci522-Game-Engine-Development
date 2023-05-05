#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PhysicsComponent.h"
#include "PhysicsManager.h"

#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/Mesh.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

#include "PrimeEngine/Scene/DebugRenderer.h"

namespace PE {
namespace Components
{
	PE_IMPLEMENT_CLASS1(PhysicsComponent, Component);

	PhysicsComponent::PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself, Handle hMeshInstance, Handle hSceneNode)
		:Component(context, arena, hMyself), m_aabb(context, arena,2), m_hMinst(hMeshInstance)
	{
		// initial sn
		m_hSN = hSceneNode;
		SceneNode* pSN = m_hSN.getObject<SceneNode>();

		// initial mesh and aabb
		MeshInstance* pmins = m_hMinst.getObject<MeshInstance>();
		m_hMesh = pmins->m_hAsset;
		Mesh* pMesh = m_hMesh.getObject<Mesh>();

		m_aabb.add(Vector3(pMesh->m_bbx[0][0], pMesh->m_bbx[0][1], pMesh->m_bbx[0][2])); 
		m_aabb.add(Vector3(pMesh->m_bbx[1][0], pMesh->m_bbx[1][1], pMesh->m_bbx[1][2]));

		m_radius = (m_aabb[1] - m_aabb[0]).length() / 2;

 		m_base = pSN->m_worldTransform * pSN->m_base;
		m_tarCorrect = Vector3(0.f,0.f,0.f);

		m_isStatic = false;

		PhysicsManager::Instance()->addComponent(m_hMyself);
	}

	PhysicsComponent::PhysicsComponent(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself,Vector3 pMin,Vector3 pMax, Handle hSceneNode,bool isStatic)
		:Component(context, arena, hMyself), m_aabb(context, arena, 2)
	{
		// initial sn
		m_hSN = hSceneNode;
		SceneNode* pSN = m_hSN.getObject<SceneNode>();

		m_tarCorrect = Vector3(0.f, 0.f, 0.f);

		m_base = pSN->m_worldTransform * pSN->m_base;

		m_aabb.add(pMin+m_base.getPos());
		m_aabb.add(pMax + m_base.getPos());

		//m_aabb.add(pMin);
		//m_aabb.add(pMax);

		m_radius = (m_aabb[1] - m_aabb[0]).length() / 4;

		m_tarCorrect = Vector3(0.f, 0.f, 0.f);

		m_isStatic = isStatic;

		PhysicsManager::Instance()->addComponent(m_hMyself);
	}

	void PhysicsComponent::addDefaultComponents()
	{
		Component::addDefaultComponents();
	}

	void PhysicsComponent::UpdatePosition()
	{
		m_tarCorrect = Vector3(0.f, 0.f, 0.f);
		if (!m_isStatic)
		{
			SceneNode* pSN = m_hSN.getObject<SceneNode>();
			m_base = pSN->m_worldTransform * pSN->m_base;
			m_onGround = false;
		}
	}

	void PhysicsComponent::GravityHandler()
	{
		m_gravity = Vector3(0.0f, -0.5f, 0.0f);
	}

	void PhysicsComponent::OnCollision(PhysicsComponent* pPC_other)
	{
		if (!m_isStatic && pPC_other->m_isStatic)
		{
			Collision_SphereBox(pPC_other);
		}

	}

	void PhysicsComponent::DrawStaticBox()
	{
		float pMin[] = { m_aabb[0].m_x,m_aabb[0].m_y,m_aabb[0].m_z };
		float pMax[] = { m_aabb[1].m_x,m_aabb[1].m_y,m_aabb[1].m_z };
		DebugRenderer::Instance()->createAABB(pMin, pMax);
	}

	void PhysicsComponent::DrawBox()
	{
		float pMin[] = { m_aabb[0].m_x + m_base.getPos().m_x,m_aabb[0].m_y + m_base.getPos().m_y,m_aabb[0].m_z + m_base.getPos().m_z };
		float pMax[] = { m_aabb[1].m_x + m_base.getPos().m_x,m_aabb[1].m_y + m_base.getPos().m_y ,m_aabb[1].m_z + m_base.getPos().m_z };
		DebugRenderer::Instance()->createAABB(pMin, pMax);
	}

	void PhysicsComponent::Collision_SphereBox(PhysicsComponent* pPC_other)
	{

		Vector3 closestP = ClosestPtPointAABB(pPC_other);

		Vector3 dir_block = closestP - m_base.getPos();

		float sqDist = dir_block.lengthSqr();
		
		if (sqDist < m_radius * m_radius)
		{
			if (closestP.m_y < m_base.getPos().m_y)
			{
				m_onGround = true;
				return;
			}

			dir_block.normalize();
			m_tarCorrect += Vector3(dir_block.m_x, 0, dir_block.m_z);
		}

	}

	float PhysicsComponent::SqDistPointAABB(PhysicsComponent* pPC_other)
	{
		float pMin[] = { pPC_other->m_aabb[0].m_x,pPC_other->m_aabb[0].m_y,pPC_other->m_aabb[0].m_z };
		float pMax[] = { pPC_other->m_aabb[1].m_x,pPC_other->m_aabb[1].m_y,pPC_other->m_aabb[1].m_z };
		Vector3 p = m_base.getPos();
		float p_array[] = { p.m_x,p.m_y,p.m_z };
		
		float sqDist = 0.0f;
		for (int i = 0; i < 3; i++) {
			float v = p_array[i];
			if (v < pMin[i]) sqDist += (pMin[i] - v) * (pMin[i] - v);
			if(v > pMax[i]) sqDist += (v - pMax[i]) * (v - pMax[i]);
		}

		return sqDist;
	}

	Vector3 PhysicsComponent::ClosestPtPointAABB(PhysicsComponent* pPC_other)
	{	
		float pMin[] = { pPC_other->m_aabb[0].m_x,pPC_other->m_aabb[0].m_y,pPC_other->m_aabb[0].m_z };
		float pMax[] = { pPC_other->m_aabb[1].m_x,pPC_other->m_aabb[1].m_y,pPC_other->m_aabb[1].m_z };
		Vector3 p = m_base.getPos();
		float p_array[] = { p.m_x,p.m_y,p.m_z };

		float intersectP[3];

		for (int i = 0; i < 3; i++) {
			float v = p_array[i];
			if (v < pMin[i])  v = pMin[i];
			if (v > pMax[i])  v = pMax[i];
			intersectP[i] = v;
		}

		return Vector3(intersectP[0], intersectP[1], intersectP[2]);
	}

};
};
