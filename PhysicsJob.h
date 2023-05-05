#ifndef __Physics_Job_H__
#define __Physics_Job_H__

#include "PrimeEngine/PrimeEngineIncludes.h"

namespace PE {
	void physicsThreadFunctionJob(void* params);

	void runPhysicsThreadSingleFrame();

	void runPhysicsThreadSingleFrameThreaded();
}

#endif
