#ifndef _PE_PHYSICSMANAGER_H_ 
#define _PE_PHYSICSMANAGER_H_ 

#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"


namespace PE {
namespace Components
{

	struct PhysicsManager : public Component
	{
		PE_DECLARE_CLASS(PhysicsManager);

		PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PHYSICS_START)
			virtual void do_PHYSICS_START(PE::Events::Event* pEvt);
		PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE)
			virtual void do_UPDATE(PE::Events::Event* pEvt);
		PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PHYSICS_END)
			virtual void do_PHYSICS_END(PE::Events::Event* pEvt);
		PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PRE_RENDER_needsRC)
			virtual void do_PRE_RENDER_needsRC(PE::Events::Event* pEvt);

		PhysicsManager(PE::GameContext& context, PE::MemoryArena arena, Handle hMyself);

		virtual ~PhysicsManager() {}

		virtual void addDefaultComponents();

		static void Construct(PE::GameContext& context, PE::MemoryArena arena);
		static void SetInstance(Handle h) { s_hInstance = h; };
		static PhysicsManager* Instance() { return s_hInstance.getObject<PhysicsManager>(); };


		virtual void addComponent(Handle hComponent, int* pAllowedEvents = NULL);

		static Handle s_hInstance;
		Array<Handle> m_PCs;
	};


};
};


#endif // ! 


