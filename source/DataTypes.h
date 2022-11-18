#pragma once
#include "Math.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		Vertex()
		{
			position.x = 0.f;
			position.y = 0.f;
			position.z = 0.f;
		}
		Vertex(float x, float y, float z)
		{
			position.x = x;
			position.y = y;
			position.z = z;
		}
		ColorRGB color{colors::White};
		//Made this operator, not sure if I want to use it...
		//Vertex operator-(const Vertex& other)
		//{
		//	Vertex temp;
		//	temp.position = this->position - other.position;
		//	return temp;
		//}
		//Vector2 uv{}; //W3
		//Vector3 normal{}; //W4
		//Vector3 tangent{}; //W4
		//Vector3 viewDirection{}; //W4
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		//Vector2 uv{};
		//Vector3 normal{};
		//Vector3 tangent{};
		//Vector3 viewDirection{};
	};

	enum class PrimitiveTopology
	{
		TriangeList,
		TriangleStrip
	};

	struct Mesh
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

		std::vector<Vertex_Out> vertices_out{};
		Matrix worldMatrix{};
	};
}