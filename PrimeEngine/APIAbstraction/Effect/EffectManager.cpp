#define NOMINMAX

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "../../Lua/LuaEnvironment.h"
#include "../../Lua/EventGlue/EventDataCreators.h"

#include "PrimeEngine/Render/ShaderActions/SetPerObjectGroupConstantsShaderAction.h"
#include "PrimeEngine/Render/ShaderActions/SetPerFrameConstantsShaderAction.h"
#include "PrimeEngine/Render/ShaderActions/SA_Bind_Resource.h"
#include "PrimeEngine/Render/ShaderActions/SetInstanceControlConstantsShaderAction.h"
#include "PrimeEngine/Render/ShaderActions/SA_SetAndBind_ConstResource_PerInstanceData.h"
#include "PrimeEngine/Render/ShaderActions/SA_SetAndBind_ConstResource_SingleObjectAnimationPalette.h"
#include "PrimeEngine/Render/ShaderActions/SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes.h"

#include "PrimeEngine/APIAbstraction/GPUBuffers/AnimSetBufferGPU.h"
#include "PrimeEngine/APIAbstraction/Texture/GPUTextureManager.h"
#include "PrimeEngine/Scene/DrawList.h"

// Sibling/Children includes
#include "EffectManager.h"
#include "Effect.h"
#include <vector>


namespace PE {

using namespace Components;

Handle EffectManager::s_myHandle;
	
EffectManager::EffectManager(PE::GameContext &context, PE::MemoryArena arena)
	: m_map(context, arena, 128)
	, m_pixelShaderSubstitutes(context, arena)
	, m_pCurRenderTarget(NULL)
	, m_pixelShaders(context, arena, 256)
	, m_vertexShaders(context, arena, 256)
#if APIABSTRACTION_D3D11
		, m_cbuffers(context, arena, 64)
	#endif
	, m_doMotionBlur(false)
	, m_glowSeparatedTextureGPU(context, arena)
	, m_2ndGlowTargetTextureGPU(context, arena)
	, m_shadowMapDepthTexture(context, arena)
	, m_frameBufferCopyTexture(context, arena)
{
	m_arena = arena; m_pContext = &context;
}
void EffectManager::setupConstantBuffersAndShaderResources()
{
#if PE_PLAT_IS_PSVITA
	
#elif APIABSTRACTION_D3D9

#	elif APIABSTRACTION_D3D11

		D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(m_pContext->getGPUScreen());
		ID3D11Device *pDevice = pD3D11Renderer->m_pD3DDevice;
		ID3D11DeviceContext *pDeviceContext = pD3D11Renderer->m_pD3DContext;

		ID3D11Buffer *pBuf;
		{
			//cbuffers
			HRESULT hr;
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC; // can only write to it, can't read
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // make sure we can access it with cpu for writing only
			cbDesc.MiscFlags = 0;
		
			cbDesc.ByteWidth = sizeof( SetPerFrameConstantsShaderAction::Data );

			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf );
			assert(SUCCEEDED(hr));
			SetPerFrameConstantsShaderAction::s_pBuffer = pBuf;
		
			m_cbuffers.add("cbPerFrame", pBuf);

			cbDesc.ByteWidth = sizeof( SetPerObjectGroupConstantsShaderAction::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
		
			SetPerObjectGroupConstantsShaderAction::s_pBuffer = pBuf;
			m_cbuffers.add("cbPerObjectGroup", pBuf);

			cbDesc.ByteWidth = sizeof( SetPerObjectConstantsShaderAction::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SetPerObjectConstantsShaderAction::s_pBuffer = pBuf;
			m_cbuffers.add("cbPerObject", pBuf);

			cbDesc.ByteWidth = sizeof( SetPerMaterialConstantsShaderAction::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SetPerMaterialConstantsShaderAction::s_pBuffer = pBuf;
			m_cbuffers.add("cbPerMaterial", pBuf);

			cbDesc.ByteWidth = sizeof( SetInstanceControlConstantsShaderAction::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SetInstanceControlConstantsShaderAction::s_pBuffer = pBuf;
			m_cbuffers.add("cbInstanceControlConstants", pBuf);
		}

		{
			// cbDesc.Usage = D3D11_USAGE_DEFAULT can't have cpu access flag set

			// if need cpu access for writing (using map) usage must be DYNAMIC or STAGING. resource cant be set as output
			// if need cpu access for reading and writing (using map) usage must be STAGING. resource cant be set as output
			// note, map & DYNAMIC is used for resrouces that get updated at least once per frame
			// also note that map allows updating part of resource while gpu might be using other part
			// can use UpdateSubresource with DEFAULT or IMMUTABLE, but suggested to use only with DEFAULT
			
			// if usage is default (gpu reads and writes) then UpdateSubresource() is used to write to it from cpu
			
			
			//tbuffers
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC; // can only write to it, can't read
			cbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; //means will need shader resource view // D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // make sure we can access it with cpu for writing only
#if PE_DX11_USE_STRUCTURED_BUFFER_INSTEAD_OF_TBUFFER
			cbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			cbDesc.StructureByteStride = sizeof(Matrix4x4);
#else
			cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = sizeof(Vector4);
#endif
			cbDesc.ByteWidth = sizeof(SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::Data);
			HRESULT hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::s_pBuffer = pBuf;
			

			cbDesc.ByteWidth = sizeof(SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::Data);
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::s_pBufferSinglePalette = pBuf;


			cbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			cbDesc.StructureByteStride = sizeof(SA_SetAndBind_ConstResource_PerInstanceData::PerObjectInstanceData);
			cbDesc.ByteWidth = sizeof( SA_SetAndBind_ConstResource_PerInstanceData::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SA_SetAndBind_ConstResource_PerInstanceData::s_pBuffer = pBuf;


			D3D11_SHADER_RESOURCE_VIEW_DESC vdesc;
#if PE_DX11_USE_STRUCTURED_BUFFER_INSTEAD_OF_TBUFFER
			vdesc.Format = DXGI_FORMAT_UNKNOWN;
			vdesc.Buffer.NumElements = sizeof(SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::Data) / (4*4*4);
#else
			vdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			vdesc.Buffer.NumElements = sizeof(SetPerObjectAnimationConstantsShaderAction::Data) / (4*4);
#endif
			vdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			vdesc.Buffer.ElementOffset = 0;
			
			hr = pDevice->CreateShaderResourceView(SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::s_pBuffer, &vdesc, &SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::s_pShaderResourceView);
			assert(SUCCEEDED(hr));
			
#if PE_DX11_USE_STRUCTURED_BUFFER_INSTEAD_OF_TBUFFER
			vdesc.Buffer.NumElements = sizeof(SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::Data) / (4*4*4);
#else
			vdesc.Buffer.NumElements = sizeof(SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::Data) / (4*4);
#endif
			hr = pDevice->CreateShaderResourceView(SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::s_pBufferSinglePalette, &vdesc, &SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::s_pShaderResourceViewSinglePalette);
			assert(SUCCEEDED(hr));


			vdesc.Buffer.NumElements = sizeof(SA_SetAndBind_ConstResource_PerInstanceData::Data) / sizeof(SA_SetAndBind_ConstResource_PerInstanceData::PerObjectInstanceData);
			hr = pDevice->CreateShaderResourceView(SA_SetAndBind_ConstResource_PerInstanceData::s_pBuffer, &vdesc, &SA_SetAndBind_ConstResource_PerInstanceData::s_pShaderResourceView);
			assert(SUCCEEDED(hr));
		}

		AnimSetBufferGPU::createGPUBufferForAnimationCSResult(*m_pContext);
#	endif

}

void EffectManager::createSetShadowMapShaderValue(PE::Components::DrawList *pDrawList)
{
	Handle &h = pDrawList->nextGlobalShaderValue();
	h = Handle("SA_Bind_Resource", sizeof(SA_Bind_Resource));
	SA_Bind_Resource *p = new(h) SA_Bind_Resource(*m_pContext, m_arena);
	p->set(SHADOWMAP_TEXTURE_2D_SAMPLER_SLOT, m_shadowMapDepthTexture.m_samplerState, API_CHOOSE_DX11_DX9_OGL_PSVITA(m_shadowMapDepthTexture.m_pDepthShaderResourceView, m_shadowMapDepthTexture.m_pTexture, m_shadowMapDepthTexture.m_texture, m_shadowMapDepthTexture.m_texture));
}

void EffectManager::buildFullScreenBoard()
{
	//todo: use createBillboard() functionality of cpu buffers
	PositionBufferCPU vbcpu(*m_pContext, m_arena);
	float fw = (float)(m_pContext->getGPUScreen()->getWidth());
	float fh = (float)(m_pContext->getGPUScreen()->getHeight());
	float fx = -1.0f / fw / 2.0f;
	float fy = 1.0f / fh / 2.0f;

	vbcpu.createNormalizeBillboardCPUBufferXYWithPtOffsets(fx, fy);
	
	ColorBufferCPU tcbcpu(*m_pContext, m_arena);
	tcbcpu.m_values.reset(3 * 4);
	#if APIABSTRACTION_OGL
		// flip up vs down in ogl
		tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f);
	#else
		tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f);
	#endif
	
	IndexBufferCPU ibcpu(*m_pContext, m_arena);
	ibcpu.createBillboardCPUBuffer();

	m_hVertexBufferGPU = Handle("VERTEX_BUFFER_GPU", sizeof(VertexBufferGPU));
	VertexBufferGPU *pvbgpu = new(m_hVertexBufferGPU) VertexBufferGPU(*m_pContext, m_arena);
	pvbgpu->createGPUBufferFromSource_ColoredMinimalMesh(vbcpu, tcbcpu);

	m_hIndexBufferGPU = Handle("INDEX_BUFFER_GPU", sizeof(IndexBufferGPU));
	IndexBufferGPU *pibgpu = new(m_hIndexBufferGPU) IndexBufferGPU(*m_pContext, m_arena);
	pibgpu->createGPUBuffer(ibcpu);

	m_hFirstGlowPassEffect = getEffectHandle("firstglowpass.fx");
	m_hSecondGlowPassEffect = getEffectHandle("verticalblur.fx");
	m_hGlowSeparationEffect = getEffectHandle("glowseparationpass.fx");
	m_hMotionBlurEffect = getEffectHandle("motionblur.fx");
	m_hColoredMinimalMeshTech = getEffectHandle("ColoredMinimalMesh_Tech");
}


void EffectManager::setFrameBufferCopyRenderTarget()
{
	#if APIABSTRACTION_D3D9
		m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_frameBufferCopyTexture, true);
	#elif APIABSTRACTION_OGL
		assert(!"not implemented");
	#elif APIABSTRACTION_D3D11

		m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_frameBufferCopyTexture, true);
	#endif
}

void EffectManager::setShadowMapRenderTarget()
{
	if (m_pCurRenderTarget)
	{
		assert(!"There should be no active render target when we set shadow map as render target!");
	}

#if APIABSTRACTION_D3D9
	m_pContext->getGPUScreen()->setDepthStencilOnlyRenderTargetAndViewport(&m_shadowMapDepthTexture, true);
	m_pCurRenderTarget = &m_shadowMapDepthTexture;
#elif APIABSTRACTION_D3D11
	m_pContext->getGPUScreen()->setDepthStencilOnlyRenderTargetAndViewport(&m_shadowMapDepthTexture, true);
	m_pCurRenderTarget = &m_shadowMapDepthTexture;
#elif APIABSTRACTION_OGL
	m_pContext->getGPUScreen()->setDepthStencilOnlyRenderTargetAndViewport(&m_shadowMapDepthTexture, true);
	m_pCurRenderTarget = &m_shadowMapDepthTexture;
#endif


}

void EffectManager::endCurrentRenderTarget()
{
	m_pContext->getGPUScreen()->endRenderTarget(m_pCurRenderTarget);
	m_pCurRenderTarget = NULL;
}

void EffectManager::setTextureAndDepthTextureRenderTargetForGlow()
{
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(m_hGlowTargetTextureGPU.getObject<TextureGPU>(), m_hGlowTargetTextureGPU.getObject<TextureGPU>(), true, true);
	m_pCurRenderTarget = m_hGlowTargetTextureGPU.getObject<TextureGPU>();
}

// ms 1
void EffectManager::setTextureAndDepthTextureRenderTargetForDeferredGlow()
{
	m_pContext->getGPUScreen()->setMultipleRenderTargetsAndViewportWithDepth(m_hGlowMultipleTargetTextureGPU.getObject<TextureGPU>(), m_hGlowMultipleTargetTextureGPU.getObject<TextureGPU>(), true, true);
	m_pCurRenderTarget = m_hGlowTargetTextureGPU.getObject<TextureGPU>();
}

void EffectManager::setTextureAndDepthTextureRenderTargetForDefaultRendering()
{
	// use device back buffer and depth
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
}


void EffectManager::set2ndGlowRenderTarget()
{
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_2ndGlowTargetTextureGPU, true);
	m_pCurRenderTarget = &m_2ndGlowTargetTextureGPU;
}


void EffectManager::drawFullScreenQuad()
{
	Effect &curEffect = *m_hColoredMinimalMeshTech.getObject<Effect>();

	if (!curEffect.m_isReady)
		return;

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	TextureGPU *pTex = PE::GPUTextureManager::Instance()->getRandomTexture().getObject<TextureGPU>();

 	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, pTex->m_samplerState, API_CHOOSE_DX11_DX9_OGL_PSVITA(pTex->m_pShaderResourceView, pTex->m_pTexture, pTex->m_texture, pTex->m_texture));
 	setTextureAction.bindToPipeline(&curEffect);

	PE::SetPerFrameConstantsShaderAction sa(*m_pContext, m_arena);
	sa.m_data.gGameTime = 1.0f;

	sa.bindToPipeline(&curEffect);

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();

	static float x = 0;
	//objSa.m_data.gW.importScale(0.5f, 1.0f, 1.0f);
	objSa.m_data.gW.setPos(Vector3(x, 0, 1.0f));
	//x+=0.01;
	if (x > 1)
		x = 0;
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);

	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	sa.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
}

// this function has to be called right after rendering scene into render target
// the reason why mipmaps have to be generated is because we separate glow
// into a smaller texture, so we need to generate mipmaps so that when we sample glow, it is not aliased
// if mipmaps are not generated, but do exist glow might not work at all since a lower mip will be sampled
void EffectManager::drawGlowSeparationPass()
{
	Effect &curEffect = *m_hGlowSeparationEffect.getObject<Effect>(); // firstglowpass 

	if (!curEffect.m_isReady)
		return;
//todo: generate at least one mipmap on all platforms so that glow downsampling is smoother
// in case it doesnt look smooth enough. as long as we are nto donwsampling to ridiculous amount it should be ok without mipmaps
/*

		m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pTexture->GenerateMipSubLevels();

#if APIABSTRACTION_D3D11

			D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(m_pContext->getGPUScreen());
			ID3D11Device *pDevice = pD3D11Renderer->m_pD3DDevice;
			ID3D11DeviceContext *pDeviceContext = pD3D11Renderer->m_pD3DContext;

			pDeviceContext->GenerateMips(
			m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView);
#endif		
*/
	
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_glowSeparatedTextureGPU, true);

	m_pCurRenderTarget = &m_glowSeparatedTextureGPU;

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);
	
	TextureGPU *t = m_hGlowTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture,t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);
	
	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
}

void EffectManager::drawFirstGlowPass()
{
	Effect &curEffect = *m_hFirstGlowPassEffect.getObject<Effect>();

	if (!curEffect.m_isReady)
		return;
	
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_2ndGlowTargetTextureGPU, true); // bind frambuffer to textureGPU, viewport setting

	m_pCurRenderTarget = &m_2ndGlowTargetTextureGPU;

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent(); // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, XXX);
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect); //  glBindVertexArray(XXX) glBindBuffer(GL_ARRAY_BUFFER,XXX)

	curEffect.setCurrent(pvbGPU); // glUseProgram(shader of curEffect)

	//PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_glowSeparatedTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_glowSeparatedTextureGPU.m_pShaderResourceView, m_glowSeparatedTextureGPU.m_pTexture, m_glowSeparatedTextureGPU.m_texture));
	//setTextureAction.bindToPipeline(&curEffect); //   glBindTexture here.

	TextureGPU* t = m_hGlowMultipleTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);
	PE::SA_Bind_Resource setPositionTextureAction(*m_pContext, m_arena, POSITION_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_deferredTexture_pos));
	setPositionTextureAction.bindToPipeline(&curEffect);
	PE::SA_Bind_Resource setNormalTextureAction(*m_pContext, m_arena, NORMAL_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_deferredTexture_normal));
	setNormalTextureAction.bindToPipeline(&curEffect);

	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	SetPerObjectGroupConstantsShaderAction::hlsl_Light light0 =
	{
		{ 0.0f, 0.0f, 0.0f }, 0.0f, // pos, pad0
		{ 1.0f, 0.0f, 0.0f }, 0.0f, // dir, pad1
		{ 0.2f, 0.2f, 0.2f, 1.0f }, // ambient
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // diffuse
		{ 0.0f, 0.0f, 0.0f , 1.0f }, // spec
		{ 0.0f, 0.0f, 0.0f }, 1.0f, // att, spotPower
		100.0f, 1.0, 0.0f, 0.0f // range, type, pad2, pad3
	};
	cb.m_data.gLights[0] = light0;
	cb.bindToPipeline(&curEffect);

	PE::SetPerObjectConstantsShaderAction objSa; // config data
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect); // set Matrix config as well

	pibGPU->draw(1, 0); // glDrawElements

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	cb.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
}

void EffectManager::drawSecondGlowPass() 
{
	Effect &curEffect = *m_hSecondGlowPassEffect.getObject<Effect>(); // verticalblur.fx "ColoredMinimalMesh_VS", "main", "ColoredMinimalMesh_VertBlurCombine_PS"
	if (!curEffect.m_isReady)
		return;
	
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>(), true);
	
	m_pCurRenderTarget = m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>();
	
	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	//PE::SA_Bind_Resource setBlurTextureAction(*m_pContext, m_arena, DIFFUSE_BLUR_TEXTURE_2D_SAMPLER_SLOT, m_2ndGlowTargetTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_2ndGlowTargetTextureGPU.m_pShaderResourceView, m_2ndGlowTargetTextureGPU.m_pTexture, m_2ndGlowTargetTextureGPU.m_texture));
	//setBlurTextureAction.bindToPipeline(&curEffect);

	//TextureGPU *t = m_hGlowTargetTextureGPU.getObject<TextureGPU>();

	// ms1 read from multiple rendering texture
	TextureGPU* t = m_hGlowMultipleTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);
	PE::SA_Bind_Resource setPositionTextureAction(*m_pContext, m_arena, POSITION_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_deferredTexture_pos));
	setPositionTextureAction.bindToPipeline(&curEffect);
	PE::SA_Bind_Resource setNormalTextureAction(*m_pContext, m_arena, NORMAL_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_deferredTexture_normal));
	setNormalTextureAction.bindToPipeline(&curEffect);

	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	SetPerObjectGroupConstantsShaderAction::hlsl_Light light0 =
	{
		{ 0.0f, 0.0f, 0.0f }, 0.0f, // pos, pad0
		{ 1.0f, 0.0f, 0.0f }, 0.0f, // dir, pad1
		{ 0.2f, 0.2f, 0.2f, 1.0f }, // ambient
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // diffuse
		{ 0.0f, 0.0f, 0.0f , 1.0f }, // spec
		{ 0.0f, 0.0f, 0.0f }, 1.0f, // att, spotPower
		100.0f, 1.0, 0.0f, 0.0f // range, type, pad2, pad3
	};
	SetPerObjectGroupConstantsShaderAction::hlsl_Light light1 =
	{
		{ -10.0f, 2.0f , 21.0f}, 0.0f, // pos, pad0
		{ 1.0f, 0.0f, 0.0f }, 0.0f, // dir, pad1
		{ 0.0f, 0.0f, 0.0f, 1.0f }, // ambient
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // diffuse
		{ 0.0, 1.0f, 0.0f , 1.0f }, // spec
		{ 0.05, 0.05, 0.05 }, 1.0f, // att, spotPower
		200.0f, 2.0, 0.0f, 0.0f // range, type, pad2, pad3
	};
	SetPerObjectGroupConstantsShaderAction::hlsl_Light light2 =
	{
		{ -20.0f, 2.0f , 21.0f}, 0.0f, // pos, pad0
		{ 1.0f, 0.0f, 0.0f }, 0.0f, // dir, pad1
		{ 0.0f, 0.0f, 0.0f, 1.0f }, // ambient
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // diffuse
		{ 1.0, 0.0f, 0.0f , 1.0f }, // spec
		{ 0.05, 0.05, 0.05 }, 1.0f, // att, spotPower
		200.0f, 2.0, 0.0f, 0.0f // range, type, pad2, pad3
	};
	SetPerObjectGroupConstantsShaderAction::hlsl_Light light3 =
	{
		{ -30.0f, 2.0f , 21.0f}, 0.0f, // pos, pad0
		{ 1.0f, 0.0f, 0.0f }, 0.0f, // dir, pad1
		{ 0.0f, 0.0f, 0.0f, 1.0f }, // ambient
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // diffuse
		{ 0.0, 0.0f, 1.0f , 1.0f }, // spec
		{ 0.05, 0.05, 0.05 }, 1.0f, // att, spotPower
		200.0f, 2.0, 0.0f, 0.0f // range, type, pad2, pad3
	};

	cb.m_data.gLights[0] = light1;
	cb.m_data.gLights[1] = light2;
	cb.m_data.gLights[2] = light3;
	//cb.m_data.gLights[3] = light0;
	cb.bindToPipeline(&curEffect);

	Matrix4x4 WVP = Matrix4x4();
	Matrix4x4 model = Matrix4x4();
	model.loadIdentity();
	model.setPos(Vector3(light1.pos.m_x, light1.pos.m_y, light1.pos.m_z));
	WVP = m_currentViewProjMatrix * model;

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = WVP;

	objSa.bindToPipeline(&curEffect);
	drawSphere(2.0f);

	model.loadIdentity();
	model.setPos(Vector3(light2.pos.m_x, light2.pos.m_y, light2.pos.m_z));
	WVP = m_currentViewProjMatrix * model;

	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = WVP;

	objSa.bindToPipeline(&curEffect);
	drawSphere(2.0f);

	model.loadIdentity();
	model.setPos(Vector3(light3.pos.m_x, light3.pos.m_y, light3.pos.m_z));
	WVP = m_currentViewProjMatrix * model;

	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = WVP;

	objSa.bindToPipeline(&curEffect);
	drawSphere(2.0f);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	//setBlurTextureAction.unbindFromPipeline(&curEffect);
	setTextureAction.unbindFromPipeline(&curEffect);
	cb.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);

}

void EffectManager::drawMotionBlur()
{
	Effect &curEffect = *m_hMotionBlurEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;
	
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(0, true);

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	TextureGPU *t = m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);
	PE::SA_Bind_Resource setAmbientTextureAction(*m_pContext, m_arena, ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_2ndGlowTargetTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_2ndGlowTargetTextureGPU.m_pShaderResourceView, m_2ndGlowTargetTextureGPU.m_pTexture, m_2ndGlowTargetTextureGPU.m_texture));
	setAmbientTextureAction.bindToPipeline(&curEffect);

	//todo: enable this to get motion blur working. need to make shader use depth map, not shadow map
	//PE::SA_Bind_Resource setDepthTextureAction(DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT, API_CHOOSE_DX11_DX9_OGL(m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pTexture, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_texture));
	//setDepthTextureAction.bindToPipeline(&curEffect);

	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gPreviousViewProjMatrix = m_previousViewProjMatrix;
	cb.m_data.gViewProjInverseMatrix = m_currentViewProjMatrix.inverse();
	cb.m_data.gDoMotionBlur = m_doMotionBlur;

	cb.bindToPipeline();

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);


	pibGPU->draw(1, 0);
	m_previousViewProjMatrix = m_currentViewProjMatrix;

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	setAmbientTextureAction.unbindFromPipeline(&curEffect);
	cb.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
	//todo: enable this to get motion blur working
	//setDepthTextureAction.unbindFromPipeline(&curEffect);
}

void EffectManager::drawFrameBufferCopy()
{
	Effect &curEffect = *m_hMotionBlurEffect.getObject<Effect>();

	if (!curEffect.m_isReady)
		return;

	setFrameBufferCopyRenderTarget();

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	TextureGPU *t = m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gPreviousViewProjMatrix = m_previousViewProjMatrix;
	cb.m_data.gViewProjInverseMatrix = m_currentViewProjMatrix.inverse();
	cb.m_data.gDoMotionBlur = m_doMotionBlur;

	cb.bindToPipeline();

	pibGPU->draw(1, 0);
	m_previousViewProjMatrix = m_currentViewProjMatrix;

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	}

Effect *EffectManager::operator[] (const char *pEffectFilename)
{
	return m_map.findHandle(pEffectFilename).getObject<Effect>();
}

void EffectManager::debugDrawRenderTarget(bool drawGlowRenderTarget, bool drawSeparatedGlow, bool drawGlow1stPass, bool drawGlow2ndPass, bool drawShadowRenderTarget)
{
	// use motion blur for now since it doesnt do anything but draws the surface
	Effect &curEffect = *m_hMotionBlurEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;
	
#	if APIABSTRACTION_D3D9
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
	// this is called in function above: IRenderer::Instance()->getDevice()->BeginScene();
#elif APIABSTRACTION_OGL
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
#	elif APIABSTRACTION_D3D11
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(0, true);
#	endif

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena);
	
	if (drawGlowRenderTarget)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_samplerState,  API_CHOOSE_DX11_DX9_OGL(m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pTexture, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_texture));
	if (drawSeparatedGlow)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_glowSeparatedTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_glowSeparatedTextureGPU.m_pShaderResourceView, m_glowSeparatedTextureGPU.m_pTexture, m_glowSeparatedTextureGPU.m_texture));
	if (drawGlow1stPass)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_2ndGlowTargetTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_2ndGlowTargetTextureGPU.m_pShaderResourceView, m_2ndGlowTargetTextureGPU.m_pTexture, m_2ndGlowTargetTextureGPU.m_texture));
	if (drawGlow2ndPass)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>()->m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>()->m_pTexture, m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>()->m_texture));
	if (drawShadowRenderTarget)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_shadowMapDepthTexture.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_shadowMapDepthTexture.m_pShaderResourceView, m_shadowMapDepthTexture.m_pTexture, m_shadowMapDepthTexture.m_texture));
	
	setTextureAction.bindToPipeline(&curEffect);
	
	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gPreviousViewProjMatrix = m_previousViewProjMatrix;
	cb.m_data.gViewProjInverseMatrix = m_currentViewProjMatrix.inverse();
	cb.m_data.gDoMotionBlur = m_doMotionBlur;

	cb.bindToPipeline();

	pibGPU->draw(1, 0);
	m_previousViewProjMatrix = m_currentViewProjMatrix;

	setTextureAction.unbindFromPipeline(&curEffect);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);
}

unsigned int sphereVAO;
unsigned int indexCount;
void EffectManager::drawSphere(float radius = 1.0f)
{
	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);

		unsigned int vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		std::vector<Vector3> positions;
		std::vector<Vector2> uv;
		std::vector<Vector3> normals;
		std::vector<unsigned int> indices;

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;

		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI) * radius;
				float yPos = std::cos(ySegment * PI) * radius;
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI) * radius;

				positions.push_back(Vector3(xPos, yPos, zPos));
				uv.push_back(Vector2(xSegment, ySegment));
				normals.push_back(Vector3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		indexCount = static_cast<unsigned int>(indices.size());

		std::vector<float> data;
		for (unsigned int i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].getX());
			data.push_back(positions[i].getY());
			data.push_back(positions[i].getZ());
			if (normals.size() > 0)
			{
				data.push_back(normals[i].getX());
				data.push_back(normals[i].getY());
				data.push_back(normals[i].getZ());
			}
			if (uv.size() > 0)
			{
				data.push_back(uv[i].m_x);
				data.push_back(uv[i].m_y);
			}
		}
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		unsigned int stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}


}; // namespace PE


