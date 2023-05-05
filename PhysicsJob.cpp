#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PhysicsJob.h"

#if APIABSTRACTION_IOS
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <CoreData/CoreData.h>
#endif

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};

#include "PrimeEngine/Game/Client/ClientGame.h"
#include "PrimeEngine/Application/Application.h"
#include "PrimeEngine/Physics/PhysicsManager.h"
#include "PrimeEngine/Physics/PhysicsComponent.h"

namespace PE
{
using namespace Events;
using namespace Components;

void physicsThreadFunctionJob(void* params)
{
	GameContext* pContext = static_cast<PE::GameContext*>(params);

	g_physicsThreadInitializationLock.lock();
	//initialization here..
	g_physicsThreadInitialized = true;
	g_physicsThreadExited = false;
	g_physicsThreadInitializationLock.unlock();

	//acquire rendring thread lock so that we can sleep on it until game thread wakes us up
	g_physicsThreadLock.lock();

	// now we can signal main thread that this thread is initialized
	g_physicsThreadInitializedCV.signal();
	while (1)
	{
		runPhysicsThreadSingleFrameThreaded();
		if (g_physicsThreadExited)
			return;
	}
	return;
}


void runPhysicsThreadSingleFrameThreaded()
{
	while (!g_physicsThreadCanStart && !g_physicsThreadShouldExit)
	{
		bool success = g_physicsCanStartCV.sleep();
		assert(success);
	}
	g_physicsThreadCanStart = false; // set to false for next frame

	if (g_physicsThreadShouldExit)
	{
		//right now game thread is waiting on this thread to finish
		g_physicsThreadLock.unlock();
		g_physicsThreadExited = true;
		return;
	}

	runPhysicsThreadSingleFrame();
}

void runPhysicsThreadSingleFrame()
{

	for (unsigned int i = 0; i < PhysicsManager::Instance()->m_PCs.m_size; i++) {
		PhysicsComponent* pPC = PhysicsManager::Instance()->m_PCs[i].getObject<PhysicsComponent>();
		pPC->UpdatePosition();
	}

	for (unsigned int i = 0; i < PhysicsManager::Instance()->m_PCs.m_size; i++) {
		PhysicsComponent* pPC = PhysicsManager::Instance()->m_PCs[i].getObject<PhysicsComponent>();
		for (unsigned int j = 0; j < PhysicsManager::Instance()->m_PCs.m_size; j++) {
			if (pPC->m_isStatic || i == j) continue;
			PhysicsComponent* pPC_other = PhysicsManager::Instance()->m_PCs[j].getObject<PhysicsComponent>();
			pPC->OnCollision(pPC_other);
		}
	}


}

}
