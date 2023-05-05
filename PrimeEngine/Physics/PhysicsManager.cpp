#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

#include "PhysicsManager.h"
#include "PhysicsComponent.h"

namespace PE {
namespace Components
{
	Handle PhysicsManager::s_hInstance;

	PE_IMPLEMENT_CLASS1(PhysicsManager, Component);

	PhysicsManager::PhysicsManager(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself)
		: Component(context, arena, hMyself)
		, m_PCs(context, arena, 64)
	{}

	void PhysicsManager::addComponent(Handle hComponent, int* pAllowedEvents)
	{
		if (hComponent.getObject<Component>()->isInstanceOf<PhysicsComponent>())
		{
			m_PCs.add(hComponent);
		}

		Component::addComponent(hComponent, pAllowedEvents);
	}

	void PhysicsManager::addDefaultComponents()
	{
		PE_REGISTER_EVENT_HANDLER(PE::Events::Event_PHYSICS_START, PhysicsManager::do_PHYSICS_START);
		PE_REGISTER_EVENT_HANDLER(PE::Events::Event_UPDATE, PhysicsManager::do_UPDATE);
		PE_REGISTER_EVENT_HANDLER(PE::Events::Event_PHYSICS_END, PhysicsManager::do_PHYSICS_END);
		PE_REGISTER_EVENT_HANDLER(PE::Events::Event_PRE_RENDER_needsRC, PhysicsManager::do_PRE_RENDER_needsRC);

		Component::addDefaultComponents();
	}

	void PhysicsManager::Construct(PE::GameContext& context, PE::MemoryArena arena)
	{
		Handle h("PHYSICS_MANAGER", sizeof(PhysicsManager));
		PhysicsManager* pPhysicsManager = new(h) PhysicsManager(context, arena, h);
		pPhysicsManager->addDefaultComponents();
		SetInstance(h);
	}

	void PhysicsManager::do_PHYSICS_START(PE::Events::Event* pEvt) {
		for (unsigned int i = 0; i < m_PCs.m_size; i++) {
			PhysicsComponent* pPC = m_PCs[i].getObject<PhysicsComponent>();
			for (unsigned int j = 0; j < m_PCs.m_size; j++) {
				if (pPC->m_isStatic || i == j) continue;
				PhysicsComponent* pPC_other = m_PCs[j].getObject<PhysicsComponent>();
				pPC->OnCollision(pPC_other);
			}
		}
	}

	void PhysicsManager::do_UPDATE(PE::Events::Event* pEvt) {

	}

	void PhysicsManager::do_PHYSICS_END(PE::Events::Event* pEvt) {
		for (unsigned int i = 0; i < m_PCs.m_size; i++) {
			PhysicsComponent* pPC = m_PCs[i].getObject<PhysicsComponent>();
			pPC->UpdatePosition();
			//pPC->GravityHandler();
		}
	}

	void PhysicsManager::do_PRE_RENDER_needsRC(PE::Events::Event* pEvt) {
		for (unsigned int i = 0; i < m_PCs.m_size; i++) {
			PhysicsComponent* pPC = m_PCs[i].getObject<PhysicsComponent>();
			if (!pPC->m_isStatic)
				pPC->DrawBox();
			else
				pPC->DrawStaticBox();
		}
	}
};
};