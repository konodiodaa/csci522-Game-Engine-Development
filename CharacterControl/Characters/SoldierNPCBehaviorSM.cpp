#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Scene/DebugRenderer.h"
#include "../ClientGameObjectManagerAddon.h"
#include "../CharacterControlContext.h"
#include "SoldierNPCMovementSM.h"
#include "SoldierNPCAnimationSM.h"
#include "SoldierNPCBehaviorSM.h"
#include "SoldierNPC.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Render/IRenderer.h"
using namespace PE::Components;
using namespace PE::Events;
using namespace CharacterControl::Events;

namespace CharacterControl{

namespace Components{

PE_IMPLEMENT_CLASS1(SoldierNPCBehaviorSM, Component);

SoldierNPCBehaviorSM::SoldierNPCBehaviorSM(PE::GameContext &context, PE::MemoryArena arena, PE::Handle hMyself, PE::Handle hMovementSM) 
: Component(context, arena, hMyself)
, m_hMovementSM(hMovementSM)
{

}

void SoldierNPCBehaviorSM::start()
{
	if (m_havePatrolWayPoint)
	{
		m_state = WAITING_FOR_WAYPOINT; // will update on next do_UPDATE()
	}
	else
	{
		m_state = IDLE; // stand in place

		PE::Handle h("SoldierNPCMovementSM_Event_STOP", sizeof(SoldierNPCMovementSM_Event_STOP));
		SoldierNPCMovementSM_Event_STOP *pEvt = new(h) SoldierNPCMovementSM_Event_STOP();

		m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
		// release memory now that event is processed
		h.release();
		
	}	
}

void SoldierNPCBehaviorSM::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_TARGET_REACHED, SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED);
	PE_REGISTER_EVENT_HANDLER(SoldierNPCMovementSM_Event_TARGET_SHOOT, SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_SHOOT);
	PE_REGISTER_EVENT_HANDLER(Event_UPDATE, SoldierNPCBehaviorSM::do_UPDATE);

	PE_REGISTER_EVENT_HANDLER(Event_PRE_RENDER_needsRC, SoldierNPCBehaviorSM::do_PRE_RENDER_needsRC);
}

// sent by movement state machine whenever it reaches current target
void SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED(PE::Events::Event *pEvt)
{
	PEINFO("SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_REACHED\n");

	if (m_state == PATROLLING_WAYPOINTS)
	{
		ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
		if (pGameObjectManagerAddon)
		{
			// search for waypoint object
			WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
			if (pWP && (StringOps::length(pWP->m_nextWayPointName1) > 0 
				|| StringOps::length(pWP->m_nextWayPointName2) > 0 || StringOps::length(pWP->m_nextWayPointName3) > 0))
			{
				// have next waypoint to go to
				int rd = rand() % 3;

				if (rd == 0) {
					pWP = pGameObjectManagerAddon->getWayPoint(pWP->m_nextWayPointName1);
				}
				else if (rd == 1) {
					pWP = pGameObjectManagerAddon->getWayPoint(pWP->m_nextWayPointName2);
				}
				else
					pWP = pGameObjectManagerAddon->getWayPoint(pWP->m_nextWayPointName3);

				if (pWP)
				{
					StringOps::writeToString(pWP->m_name, m_curPatrolWayPoint, 32);

					m_state = PATROLLING_WAYPOINTS;
					PE::Handle h("SoldierNPCMovementSM_Event_MOVE_TO", sizeof(SoldierNPCMovementSM_Event_MOVE_TO));
					Events::SoldierNPCMovementSM_Event_MOVE_TO *pEvt = new(h) SoldierNPCMovementSM_Event_MOVE_TO(pWP->m_base.getPos()); // pWp new waypoint

					pEvt->m_running = pWP->m_needToRunToThisWayPoint > 0;

					m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
					// release memory now that event is processed
					h.release();
				}
			}
			else
			{
				m_state = IDLE;
				// no need to send the event. movement state machine will automatically send event to animation state machine to play idle animation
			}
		}
	}
}

void SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_SHOOT(PE::Events::Event* pEvt)
{
	PEINFO("SoldierNPCBehaviorSM::do_SoldierNPCMovementSM_Event_TARGET_SHOOT\n");
	m_state = IDLE;
	if (m_state == IDLE) {
		ClientGameObjectManagerAddon* pGameObjectManagerAddon = (ClientGameObjectManagerAddon*)
			(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
		if (pGameObjectManagerAddon)
		{
			SoldierNPC* pSN = pGameObjectManagerAddon->getTarget(2);
			if (pSN) {

				PE::Handle targetSceneNode = pSN->getFirstComponentHandle<PE::Components::SceneNode>();
				Vector3 targetPos = targetSceneNode.getObject<PE::Components::SceneNode>()->m_base.getPos();

				PE::Handle h("SoldierNPCMovementSM_Event_STAND_SHOOT", sizeof(SoldierNPCMovementSM_Event_STAND_SHOOT));
				Events::SoldierNPCMovementSM_Event_STAND_SHOOT* pEvt = new(h) SoldierNPCMovementSM_Event_STAND_SHOOT(targetPos);

				m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
				// release memory now that event is processed
				h.release();
			}
		}
	}
}

// this event is executed when thread has RC
void SoldierNPCBehaviorSM::do_PRE_RENDER_needsRC(PE::Events::Event *pEvt)
{
	Event_PRE_RENDER_needsRC *pRealEvent = (Event_PRE_RENDER_needsRC *)(pEvt);
	if (m_havePatrolWayPoint)
	{
		char buf[80];
		sprintf(buf, "Patrol Waypoint: %s",m_curPatrolWayPoint);
		SoldierNPC *pSol = getFirstParentByTypePtr<SoldierNPC>();
		PE::Handle hSoldierSceneNode = pSol->getFirstComponentHandle<PE::Components::SceneNode>();
		Matrix4x4 base = hSoldierSceneNode.getObject<PE::Components::SceneNode>()->m_worldTransform;
		
		DebugRenderer::Instance()->createTextMesh(
			buf, false, false, true, false, 0,
			base.getPos(), 0.01f, pRealEvent->m_threadOwnershipMask);
		
		{
			//we can also construct points ourself
			bool sent = false;
			ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
			if (pGameObjectManagerAddon)
			{
				WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
				if (pWP)
				{
					Vector3 target = pWP->m_base.getPos();
					Vector3 pos = base.getPos();
					Vector3 color(1.0f, 1.0f, 0);
					Vector3 linepts[] = {pos, color, target, color};
					
					DebugRenderer::Instance()->createLineMesh(true, base,  &linepts[0].m_x, 2, 0);// send event while the array is on the stack
					sent = true;
				}
			}
			if (!sent) // if for whatever reason we didnt retrieve waypoint info, send the event with transform only
				DebugRenderer::Instance()->createLineMesh(true, base, NULL, 0, 0);// send event while the array is on the stack
		}
	}
}

void SoldierNPCBehaviorSM::do_UPDATE(PE::Events::Event *pEvt)
{
	if (m_state == WAITING_FOR_WAYPOINT)
	{
		if (m_havePatrolWayPoint)
		{
			ClientGameObjectManagerAddon *pGameObjectManagerAddon = (ClientGameObjectManagerAddon *)(m_pContext->get<CharacterControlContext>()->getGameObjectManagerAddon());
			if (pGameObjectManagerAddon)
			{
				// search for waypoint object
				WayPoint *pWP = pGameObjectManagerAddon->getWayPoint(m_curPatrolWayPoint);
				if (pWP)
				{
					m_state = PATROLLING_WAYPOINTS;
					PE::Handle h("SoldierNPCMovementSM_Event_MOVE_TO", sizeof(SoldierNPCMovementSM_Event_MOVE_TO));
					Events::SoldierNPCMovementSM_Event_MOVE_TO *pEvt = new(h) SoldierNPCMovementSM_Event_MOVE_TO(pWP->m_base.getPos());

					m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
					// release memory now that event is processed
					h.release();
				}
			}
		}
		else
		{
			// should not happen, but in any case, set state to idle
			m_state = IDLE;

			PE::Handle h("SoldierNPCMovementSM_Event_STOP", sizeof(SoldierNPCMovementSM_Event_STOP));
			SoldierNPCMovementSM_Event_STOP *pEvt = new(h) SoldierNPCMovementSM_Event_STOP();

			m_hMovementSM.getObject<Component>()->handleEvent(pEvt);
			// release memory now that event is processed
			h.release();
		}
	}
}


}}




