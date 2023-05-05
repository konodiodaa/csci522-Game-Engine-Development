#include "CameraSceneNode.h"
#include "../Lua/LuaEnvironment.h"
#include "PrimeEngine/Events/StandardEvents.h"

#define Z_ONLY_CAM_BIAS 0.0f
namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(CameraSceneNode, SceneNode);

CameraSceneNode::CameraSceneNode(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself) : SceneNode(context, arena, hMyself)
{
	m_near = 0.05f;
	m_far = 2000.0f;
}
void CameraSceneNode::addDefaultComponents()
{
	Component::addDefaultComponents();
	PE_REGISTER_EVENT_HANDLER(Events::Event_CALCULATE_TRANSFORMATIONS, CameraSceneNode::do_CALCULATE_TRANSFORMATIONS);
}

void CameraSceneNode::do_CALCULATE_TRANSFORMATIONS(Events::Event *pEvt)
{
	Handle hParentSN = getFirstParentByType<SceneNode>();
	if (hParentSN.isValid())
	{
		Matrix4x4 parentTransform = hParentSN.getObject<PE::Components::SceneNode>()->m_worldTransform;
		m_worldTransform = parentTransform * m_base;
	}
	
	Matrix4x4 &mref_worldTransform = m_worldTransform;

	Vector3 pos = Vector3(mref_worldTransform.m[0][3], mref_worldTransform.m[1][3], mref_worldTransform.m[2][3]);
	Vector3 n = Vector3(mref_worldTransform.m[0][2], mref_worldTransform.m[1][2], mref_worldTransform.m[2][2]);
	Vector3 target = pos + n;
	Vector3 up = Vector3(mref_worldTransform.m[0][1], mref_worldTransform.m[1][1], mref_worldTransform.m[2][1]);

	m_worldToViewTransform = CameraOps::CreateViewMatrix(pos, target, up);

	m_worldTransform2 = mref_worldTransform;

	m_worldTransform2.moveForward(Z_ONLY_CAM_BIAS);

	Vector3 pos2 = Vector3(m_worldTransform2.m[0][3], m_worldTransform2.m[1][3], m_worldTransform2.m[2][3]);
	Vector3 n2 = Vector3(m_worldTransform2.m[0][2], m_worldTransform2.m[1][2], m_worldTransform2.m[2][2]);
	Vector3 target2 = pos2 + n2;
	Vector3 up2 = Vector3(m_worldTransform2.m[0][1], m_worldTransform2.m[1][1], m_worldTransform2.m[2][1]);

	m_worldToViewTransform2 = CameraOps::CreateViewMatrix(pos2, target2, up2);
    
    PrimitiveTypes::Float32 aspect = (PrimitiveTypes::Float32)(m_pContext->getGPUScreen()->getWidth()) / (PrimitiveTypes::Float32)(m_pContext->getGPUScreen()->getHeight());
    
    PrimitiveTypes::Float32 verticalFov = 0.33f * PrimitiveTypes::Constants::c_Pi_F32;
    if (aspect < 1.0f)
    {
        //ios portrait view
        static PrimitiveTypes::Float32 factor = 0.5f;
        verticalFov *= factor;
    }

	m_viewToProjectedTransform = CameraOps::CreateProjectionMatrix(verticalFov, 
		aspect,
		m_near, m_far);
	
	// create view frustum.
	Vector3 camera_Front = pos - target;
	camera_Front.normalize();
	Vector3 camera_Up = up;
	camera_Up.normalize();
	Vector3 camera_Right = camera_Front.crossProduct(camera_Up);
	camera_Right.normalize();

	const float halfVSide = m_far * tanf(verticalFov * 0.5f);
	const float halfHSide = halfVSide * aspect;
	const Vector3 frontMultFar = m_far * camera_Front;

	frustum.nearFace = { pos + m_near * camera_Front, camera_Front };
	frustum.farFace = { pos - frontMultFar, -camera_Front };
	frustum.rightFace = { pos, (frontMultFar - camera_Right * halfHSide).crossProduct(camera_Up) };
	frustum.leftFace = { pos, camera_Up.crossProduct(frontMultFar + camera_Right * halfHSide) };
	frustum.topFace = { pos,camera_Right.crossProduct(frontMultFar - camera_Up * halfVSide) };
	frustum.bottomFace = { pos,(frontMultFar + camera_Up * halfVSide).crossProduct(camera_Right)};

	SceneNode::do_CALCULATE_TRANSFORMATIONS(pEvt);
}

bool CameraSceneNode::FrustumCulling(Vector3 pMin, Vector3 pMax)
{

	return 	isOnFrustum(frustum.nearFace, pMin, pMax) &&
		isOnFrustum(frustum.farFace, pMin, pMax) &&
		isOnFrustum(frustum.topFace, pMin, pMax) &&
		isOnFrustum(frustum.bottomFace, pMin, pMax) &&
		isOnFrustum(frustum.rightFace, pMin, pMax) &&
		isOnFrustum(frustum.leftFace, pMin, pMax);

}

bool CameraSceneNode::isOnFrustum(Plane plane, Vector3 pMin, Vector3 pMax)
{
	Vector3 list[8] = {
		Vector3(pMin.m_x,pMin.m_y,pMin.m_z),
		Vector3(pMax.m_x,pMin.m_y,pMin.m_z),
		Vector3(pMin.m_x,pMin.m_y,pMax.m_z),
		Vector3(pMax.m_x,pMin.m_y,pMax.m_z),
		Vector3(pMin.m_x,pMax.m_y,pMin.m_z),
		Vector3(pMax.m_x,pMax.m_y,pMin.m_z),
		Vector3(pMin.m_x,pMax.m_y,pMax.m_z),
		Vector3(pMax.m_x,pMax.m_y,pMax.m_z),
	};

	for (int i = 0; i < 8; i++) {
		plane.normal.normalize();
		Vector3 tmp = list[i] - plane.p;
		tmp.normalize();
		float c = plane.normal.dotProduct(list[i] - plane.p);
		if (c > 0)
		{
			return false;
		}
	}

	return true;
}

}; // namespace Components
}; // namespace PE
