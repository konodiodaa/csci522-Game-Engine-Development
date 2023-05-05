#define NOMINMAX

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Immediate include
#include "PositionBufferCPU.h"

// Inter-Engine includes
#include "PrimeEngine/FileSystem/FileReader.h"
#include "PrimeEngine/Utils/StringOps.h"
#include "PrimeEngine/MainFunction/MainFunctionArgs.h"
#include <vector>
#include <PrimeEngine/Math/Vector3.h>

// Sibling/Children includes

// Reads the specified buffer from file
void PositionBufferCPU::ReadPositionBuffer(const char *filename, const char *package)
{
    PEString::generatePathname(*m_pContext, filename, package, "PositionBuffers", PEString::s_buf, PEString::BUF_SIZE);

	// Path is now a full path to the file with the filename itself
	FileReader f(PEString::s_buf);

	char line[256];
	f.nextNonEmptyLine(line, 255);
	// TODO : make sure it is "POSITION_BUFFER"
	int version = 0;
	if (0 == StringOps::strcmp(line, "POSITION_BUFFER_V1"))
	{
		version = 1;
	}

	PrimitiveTypes::Int32 n;
	f.nextInt32(n);
	m_values.reset(n * 3); // 3 Float32 per vertex

	float factor = version == 0 ? (1.0f / 100.0f) : 1.0f;

	// Read all values
	PrimitiveTypes::Float32 val;
	float pMin[3] = { std::numeric_limits<float>::max(),std::numeric_limits<float>::max() ,std::numeric_limits<float>::max() };
	float pMax[3] = { std::numeric_limits<float>::min(),std::numeric_limits<float>::min() ,std::numeric_limits<float>::min() };
	for (int i = 0; i < n * 3; i++)
	{
		f.nextFloat32(val);
		m_values.add(val * factor);
		
		pMin[i%3] = std::min(pMin[i % 3], val * factor);
		pMax[i%3] = std::max(pMax[i % 3], val * factor);
	}

	m_pMin[0] = pMin[0]; m_pMin[1] = pMin[1]; m_pMin[2] = pMin[2];
	m_pMax[0] = pMax[0]; m_pMax[1] = pMax[1]; m_pMax[2] = pMax[2];
}

void PositionBufferCPU::createEmptyCPUBuffer()
{
	m_values.reset(0);
}

void PositionBufferCPU::createBillboardCPUBuffer(PrimitiveTypes::Float32 w, PrimitiveTypes::Float32 h)
{
	m_values.reset(3 * 4);
	add3Floats(-w/2, 0.0f, -h/2);
	add3Floats(-w/2, 0.0f, h/2); 
	add3Floats(w/2, 0.0f, h/2);
	add3Floats(w/2, 0.0f, -h/2);
}
void PositionBufferCPU::createNormalizeBillboardCPUBufferXYWithPtOffsets(PrimitiveTypes::Float32 dx, PrimitiveTypes::Float32 dy)
{
	m_values.reset(3 * 4);
	m_values.add(-1.0f+dx); m_values.add(-1.0f+dy); m_values.add(0.0f);
	m_values.add(1.0f+dx); m_values.add(-1.0f+dy); m_values.add(0.0f);
	m_values.add(1.0f+dx); m_values.add(1.0f+dy); m_values.add(0.0f);
	m_values.add(-1.0f+dx); m_values.add(1.0f+dy); m_values.add(0.0f);
	
}

void PositionBufferCPU::createSphereCPUBufferWithRadiusCenter(float radius)
{
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
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			positions.push_back(Vector3(xPos, yPos, zPos));
			uv.push_back(Vector2(xSegment, ySegment));
			normals.push_back(Vector3(xPos, yPos, zPos));
		}
	}

	std::vector<float> data;
	for (unsigned int i = 0; i < positions.size(); ++i)
	{
		data.push_back(positions[i].m_x);
		data.push_back(positions[i].m_y);
		data.push_back(positions[i].m_z);
		if (normals.size() > 0)
		{
			data.push_back(normals[i].m_x);
			data.push_back(normals[i].m_y);
			data.push_back(normals[i].m_z);
		}
		if (uv.size() > 0)
		{
			data.push_back(uv[i].m_x);
			data.push_back(uv[i].m_y);
		}
	}
}