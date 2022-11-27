//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"
#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) 
	: m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_AspectRatio = (float)m_Width / (float)m_Height;

	//Initialize Texture
	m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;
	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Solutions
	//Solution_W1();
	Solution_W2();
	
	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::Solution_W1()
{
	std::vector<Vertex> vertices_world
	{
		//triangle1
		{{0.0f, 2.0f, 0.0f},   {1, 0, 0}},
		{{1.5f, -1.0f, 0.0f},  {1, 0, 0}},
		{{-1.5f, -1.0f, 0.0f}, {1, 0, 0}},

		////triangle 2
		{{0.0f, 4.0f, 2.0f},   {1, 0, 0}},
		{{3.0f, -2.0f, 2.0f},  {0, 1, 0}},
		{{-3.0f, -2.0f, 2.0f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_screenspace{};
	
	// World Space Coordinates --> Screen Space Coordinates
	VertexTransformationFunction(vertices_world, vertices_screenspace);

	for (size_t i = 0; i < vertices_screenspace.size(); i += 3)
	{
		std::vector<Vertex> triangle
		{
			vertices_screenspace[i + 0],
			vertices_screenspace[i + 1],
			vertices_screenspace[i + 2]
		};
		
		RenderTriangle(triangle);
	}
}

bool Renderer::IsPointInTriangle(const Vector3& weights) const
{
	// All are negative
	if ((weights.x < 0) && (weights.y < 0) && (weights.z < 0))
	{
		return true;
	}

	// All are positive
	if ((weights.x > 0) && (weights.y > 0) && (weights.z > 0))
	{
		return true;
	}

	return false;
}

void Renderer::VertexTransformationFunction(std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out)
{
	for (auto& vertex : vertices_in)
	{
		// World Space -> View Space
		vertex.position = m_Camera.invViewMatrix.TransformPoint(vertex.position);

		// View Space -> NDC
		vertex.position.x = vertex.position.x / (vertex.position.z * m_Camera.fov * m_AspectRatio);
		vertex.position.y = vertex.position.y / (vertex.position.z * m_Camera.fov);

		// NDC -> Screen Space
		vertex.position.x = ((1 + vertex.position.x) / 2) * m_Width;
		vertex.position.y = ((1 - vertex.position.y) / 2) * m_Height;

		vertices_out.push_back(vertex);
	}
}


void Renderer::Solution_W2()
{
	std::vector<Mesh> meshWorld
	{
		Mesh
		{
			{
				Vertex(-3, 3,-2, {   0,   0}),
				Vertex( 0, 3,-2, {0.5f,   0}),
				Vertex( 3, 3,-2, {   1,   0}),
				Vertex(-3, 0,-2, {   0,0.5f}),
				Vertex( 0, 0,-2, {0.5f,0.5f}),
				Vertex( 3, 0,-2, {   1,0.5f}),
				Vertex(-3,-3,-2, {   0,   1}),
				Vertex( 0,-3,-2, {0.5f,   1}),
				Vertex( 3,-3,-2, {   1,   1})
			}
			,
			{
				3,0,4,
				1,5,2,
				2,6,6,
				3,7,4,
				8,5
			}
			,
			PrimitiveTopology::TriangleStrip
		}

		,

		Mesh
		{
			{
				Vertex(-3, 3,-2, {   0,   0}),
				Vertex( 0, 3,-2, {0.5f,   0}),
				Vertex( 3, 3,-2, {   1,   0}),
				Vertex(-3, 0,-2, {   0,0.5f}),
				Vertex( 0, 0,-2, {0.5f,0.5f}),
				Vertex( 3, 0,-2, {   1,0.5f}),
				Vertex(-3,-3,-2, {   0,   1}),
				Vertex( 0,-3,-2, {0.5f,   1}),
				Vertex( 3,-3,-2, {   1,   1})
			}
			,
			{
				3,0,1,
				1,4,3,
				4,1,2,
				2,5,4,
				6,3,4,
				4,7,6,
				7,4,5,
				5,8,7
			}
			,
			PrimitiveTopology::TriangleList
		}
	};



	// Render

	for (Mesh& mesh : meshWorld)
	{
		//if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		//{
		//	std::vector<Vertex> verticesSSC{};

		//	VertexTransformationFunction(mesh.vertices, verticesSSC);

		//	for (uint32_t idx = 0; idx < mesh.indices.size() - 2; idx++)
		//	{
		//		std::vector<Vertex> triangle{};

		//		triangle.push_back(verticesSSC[mesh.indices[idx + 0]]);
		//		if (idx % 2 == 1)
		//		{
		//			triangle.push_back(verticesSSC[mesh.indices[idx + 2]]);
		//			triangle.push_back(verticesSSC[mesh.indices[idx + 1]]);
		//		}
		//		else
		//		{
		//			triangle.push_back(verticesSSC[mesh.indices[idx + 1]]);
		//			triangle.push_back(verticesSSC[mesh.indices[idx + 2]]);
		//		}
		//		RenderTriangle(triangle);
		//	}
		//}

		if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
		{
			std::vector<Vertex> verticesSSC{};

			VertexTransformationFunction(mesh.vertices, verticesSSC);

			for (uint32_t idx = 0; idx < mesh.indices.size(); idx += 3)
			{
				std::vector<Vertex> triangle{};

				triangle.push_back(verticesSSC[mesh.indices[idx + 0]]);
				triangle.push_back(verticesSSC[mesh.indices[idx + 1]]);
				triangle.push_back(verticesSSC[mesh.indices[idx + 2]]);

				RenderTriangle(triangle);
			}
		}
	}
}

void Renderer::SwapLastTwoIndicesOfUnevenTriangles(std::vector<uint32_t>& indices) const
{
	for (size_t idx = 0; idx < indices.size() - 2; ++idx)
	{
		if (idx % 2 == 1)
		{
			std::swap(indices[idx + 1], indices[idx + 2]);
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

float Renderer::CalculateWeights(const Vector2& vertex1, const Vector2& vertex2, const Vector2& pixel, float area) const
{
	return Vector2::Cross(vertex2 - vertex1, pixel - vertex1) / area;
}

void Renderer::RenderTriangle(const std::vector<Vertex>& triangleScreenSpace) const
{
	const Vector2 v0{ triangleScreenSpace[0].position.x, triangleScreenSpace[0].position.y };
	const Vector2 v1{ triangleScreenSpace[1].position.x, triangleScreenSpace[1].position.y };
	const Vector2 v2{ triangleScreenSpace[2].position.x, triangleScreenSpace[2].position.y };

	Vector2 topLeft{std::min(std::min(triangleScreenSpace[0].position.x,triangleScreenSpace[1].position.x), triangleScreenSpace[2].position.x),
					std::min(std::min(triangleScreenSpace[0].position.y,triangleScreenSpace[1].position.y), triangleScreenSpace[2].position.y) };

	Vector2 bottomRight{std::max(std::max(triangleScreenSpace[0].position.x,triangleScreenSpace[1].position.x), triangleScreenSpace[2].position.x),
						std::max(std::max(triangleScreenSpace[0].position.y,triangleScreenSpace[1].position.y), triangleScreenSpace[2].position.y) };

	topLeft.x = Clamp(topLeft.x, 0.f, float(m_Width - 1));
	topLeft.y = Clamp(topLeft.y, 0.f, float(m_Height - 1));
	bottomRight.x = Clamp(bottomRight.x, 0.f, float(m_Width - 1));
	bottomRight.y = Clamp(bottomRight.y, 0.f, float(m_Height - 1));

	for (int px{ int(topLeft.x) }; px < int(bottomRight.x); ++px)
	{
		for (int py{ int(topLeft.y) }; py < int(bottomRight.y); ++py)
		{
			Vector3 weights{};
			ColorRGB finalColor{};
			const Vector2 pixel_ssc{ float(px) + 0.5f , float(py) + 0.5f };

			const float area{ Vector2::Cross(Vector2(v1, v2), Vector2(v1, v0)) };

			weights.x = CalculateWeights(v1, v2, pixel_ssc, area);
			weights.y = CalculateWeights(v2, v0, pixel_ssc, area);
			weights.z = CalculateWeights(v0, v1, pixel_ssc, area);

			//const Vector3 point = triangleScreenSpace[0].position * weights.x + triangleScreenSpace[1].position * weights.y + triangleScreenSpace[2].position * weights.z; // previous pixelDepth version
			const float pixelDepth{ 1 / (weights.x * (1 / triangleScreenSpace[0].position.z) + weights.y * (1 / triangleScreenSpace[1].position.z) + weights.z * (1 / triangleScreenSpace[2].position.z)) };
			if (IsPointInTriangle(weights))
			{
				if (pixelDepth < m_pDepthBufferPixels[px + py * m_Width])
				{
					m_pDepthBufferPixels[py * m_Width + px] = pixelDepth;

					//finalColor = triangleScreenSpace[0].color * weights.x + triangleScreenSpace[1].color * weights.y + triangleScreenSpace[2].color * weights.z;
					//finalColor.MaxToOne();

					Vector2 uvInterpolated{ pixelDepth * (	weights.x * (triangleScreenSpace[0].uv / triangleScreenSpace[0].position.z) + 
															weights.y * (triangleScreenSpace[1].uv / triangleScreenSpace[1].position.z) + 
															weights.z * (triangleScreenSpace[2].uv / triangleScreenSpace[2].position.z))};
					finalColor = m_pTexture->Sample(uvInterpolated);
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}
}