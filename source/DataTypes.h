#pragma once
#include "Math.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vertex(Vector3 posInput = Vector3{0,0,0}, Vector2 uvInput = Vector2{0,0}, ColorRGB colorInput = ColorRGB{1,1,1})
		{
			position = posInput;
			color = colorInput;
			uv = uvInput;
		}
		Vector3 position{};
		ColorRGB color{colors::White};
		Vector2 uv{}; //W3
		Vector3 normal{}; //W4
		Vector3 tangent{}; //W4
		Vector3 viewDirection{}; //W4
		
	};

	struct Vertex_Out
	{
		Vertex_Out(	Vector4 posInput = Vector4{ 0,0,0,0 }, 
					Vector2 uvInput = Vector2{ 0,0 }, 
					ColorRGB colorInput = ColorRGB{ 1,1,1 },
					Vector3 normalInput = Vector3{1,0,0},
					Vector3 tangentInput = Vector3{1,0,0})
		{
			position = posInput;
			color = colorInput;
			uv = uvInput;
			normal = normalInput;
			tangent = tangentInput;
		}
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
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

	struct Light
	{
		Vector3 origin{};
		Vector3 direction{};
		ColorRGB color{};
		float intensity{};
		float shininess{};
		ColorRGB ambient{};
	};

}
