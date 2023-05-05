#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"

#include "PrimeEngine/Physics/PhysicsComponent.h"

#include "SoldierNPCMovementSM.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPC.h"
using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl{

// Events sent by behavior state machine (or other high level state machines)
// these are events that specify where a soldier should move
namespace Events{

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_MOVE_TO, Event);

SoldierNPCMovementSM_Event_MOVE_TO::SoldierNPCMovementSM_Event_MOVE_TO(Vector3 targetPos /* = Vector3 */)
: m_targetPosition(targetPos)
, m_running(false)
{ }

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_STOP, Event);

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_STAND_SHOOT, Event);

SoldierNPCMovementSM_Event_STAND_SHOOT::SoldierNPCMovementSM_Event_STAND_SHOOT(Vector3 targetPos /* = Vector3 */)
	: m_targetPosition(targetPos)
{ }

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_TARGET_REACHED, Event);

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM_Event_TARGET_SHOOT, Event);
}

namespace Components{

PE_IMPLEMENT_CLASS1(SoldierNPCMovementSM, Component);


SoldierNPCMovementSM::SoldierNPCMovementSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself) 
: Component(context, arena, hMyself)
, m_state(STANDING)
{}

SceneNode *SoldierNPCMovementSM::getParentsSceneNode()
{
	PE::Handle hParent = getFirstParentByType<Component>();
	if (hParent.isValid())
	{
		// see if parent has scene node component
		return hParent.getObject<Component>()->getFirstComponent<SceneNode>();
		
	}
	return NULL;
}

void SoldierNPCMovementSM::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_MOVE_TO, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO);
	PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_STOP, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STOP);
	PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_STAND_SHOOT, SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STAND_SHOOT);

	PE_REGISTER_EVENT_HANDLER(Event_UPDATE, SoldierNPCMovementSM::do_UPDATE);
}

void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO(PE::Events::Event *pEvt)
{
	SoldierNPCMovementSM_Event_MOVE_TO *pRealEvt = (SoldierNPCMovementSM_Event_MOVE_TO *)(pEvt);
	
	// change state of this state machine
	m_state = WALKING_TO_TARGET;
	m_targetPostion = pRealEvt->m_targetPosition;

	OutputDebugStringA("PE: PROGRESS: SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_MOVE_TO(): received event ,running:");
	OutputDebugStringA(pRealEvt->m_running ? "true\n" : "false\n");

	if (pRealEvt->m_running)
	{
		m_state = RUNNING_TO_TARGET;
		// make sure the animations are playing

		PE::Handle h("SoldierNPCAnimSM_Event_RUN", sizeof(SoldierNPCAnimSM_Event_RUN));
		Events::SoldierNPCAnimSM_Event_RUN* pOutEvt = new(h) SoldierNPCAnimSM_Event_RUN();

		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(pOutEvt);

		// release memory now that event is processed
		h.release();
	}
	else
	{
		m_state = WALKING_TO_TARGET;
		// make sure the animations are playing

		PE::Handle h("SoldierNPCAnimSM_Event_WALK", sizeof(SoldierNPCAnimSM_Event_WALK));
		Events::SoldierNPCAnimSM_Event_WALK* pOutEvt = new(h) SoldierNPCAnimSM_Event_WALK();

		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(pOutEvt);

		// release memory now that event is processed
		h.release();
	}


}

void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STAND_SHOOT(PE::Events::Event* pEvt)
{
	SoldierNPCMovementSM_Event_STAND_SHOOT* pRealEvt = (SoldierNPCMovementSM_Event_STAND_SHOOT*)(pEvt);

	m_state = STANDING_SHOOTING;
	m_targetPostion = pRealEvt->m_targetPosition;
}

void SoldierNPCMovementSM::do_SoldierNPCMovementSM_Event_STOP(PE::Events::Event *pEvt)
{
	Events::SoldierNPCAnimSM_Event_STOP Evt;

	SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
	pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(&Evt);
}

void SoldierNPCMovementSM::do_UPDATE(PE::Events::Event *pEvt)
{
	if (m_state == WALKING_TO_TARGET || m_state == RUNNING_TO_TARGET)
	{
		// see if parent has scene node component
		SceneNode *pSN = getParentsSceneNode();
		if (pSN)
		{
			PE::Handle hPC = pSN->m_hPC;
			PhysicsComponent* pPC = hPC.getObject<PhysicsComponent>();

			Vector3 curPos = pPC->m_base.getPos();
			Vector3 correction = pPC->m_tarCorrect;
			float dsqr = (m_targetPostion - curPos).lengthSqr();

			bool reached = true;
			if (dsqr > 0.01f)
			{
				// not at the spot yet
				Event_UPDATE *pRealEvt = (Event_UPDATE *)(pEvt);
				//static float speed = 1.4f;

				float speed = (m_state == WALKING_TO_TARGET) ? 1.4f : 3.0f;

				float allowedDisp = speed * pRealEvt->m_frameTime;

				Vector3 dir = Vector3((m_targetPostion - curPos).m_x,0.0f, (m_targetPostion - curPos).m_z);

				// A4 direction correction;
				float costheta = dir.dotProduct(correction);
				if (costheta != 0) {
					correction = - dir.projectionOnVector(correction);
				}

				dir = dir + correction;
				dir.normalize();
				float dist = sqrt(dsqr);

				if (dist > allowedDisp)
				{
					dist = allowedDisp; // can move up to allowedDisp
					reached = false; // not reaching destination yet
				}

				if (!pPC->m_onGround)
				{
					Vector3 gravity(0, -5.0f, 0);
					dir += gravity;
				}

				// instantaneous turn
				pSN->m_base.turnInDirection(dir, 3.1415f);
				pSN->m_base.setPos(curPos + dir * dist);
			}

			if (reached)
			{
				m_state = STANDING;
				
				// target has been reached. need to notify all same level state machines (components of parent)
				{
					PE::Handle h("SoldierNPCMovementSM_Event_TARGET_REACHED", sizeof(SoldierNPCMovementSM_Event_TARGET_REACHED));
					Events::SoldierNPCMovementSM_Event_TARGET_REACHED *pOutEvt = new(h) SoldierNPCMovementSM_Event_TARGET_REACHED();

					PE::Handle hParent = getFirstParentByType<Component>();
					if (hParent.isValid())
					{
						hParent.getObject<Component>()->handleEvent(pOutEvt);
					}
					
					// release memory now that event is processed
					h.release();
				}

				if (m_state == STANDING)
				{
					// no one has modified our state based on TARGET_REACHED callback
					// this means we are not going anywhere right now
					// so can send event to animation state machine to stop
					{
						Events::SoldierNPCAnimSM_Event_STOP evt;
						
						SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
						pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(&evt);
					}

					m_state = STANDING_SHOOTING;
				}
			}
		}
	}

	if (m_state == STANDING_SHOOTING)
	{
		SceneNode* pSN = getParentsSceneNode();
		
		if (pSN)
		{
			Vector3 curPos = pSN->m_base.getPos();
			Vector3 dir = (m_targetPostion - curPos);
			dir.normalize();

			pSN->m_base.turnInDirection(dir, 3.1415f);
		}

		Events::SoldierNPCAnimSM_Event_STAND_SHOOT evt;

		SoldierNPC* pSol = getFirstParentByTypePtr<SoldierNPC>();
		pSol->getFirstComponent<PE::Components::SceneNode>()->handleEvent(&evt);

		PE::Handle h("SoldierNPCMovementSM_Event_TARGET_SHOOT", sizeof(SoldierNPCMovementSM_Event_TARGET_SHOOT));
		Events::SoldierNPCMovementSM_Event_TARGET_SHOOT* pOutEvt = new(h) SoldierNPCMovementSM_Event_TARGET_SHOOT();

		PE::Handle hParent = getFirstParentByType<Component>();
		if (hParent.isValid())
		{
			hParent.getObject<Component>()->handleEvent(pOutEvt);
		}

		// release memory now that event is processed
		h.release();
	}
}

}}




