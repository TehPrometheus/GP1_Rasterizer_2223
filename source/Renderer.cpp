//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) 
	: m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_AspectRatio = (float)m_Width / (float)m_Height;
	//m_Vertices_ndc.push_back( Vertex(   0.f,  0.5f, 1.f ) );
	//m_Vertices_ndc.push_back( Vertex(  0.5f, -0.5f, 1.f ) );
	//m_Vertices_ndc.push_back( Vertex( -0.5f, -0.5f, 1.f ) );

	m_Vertices_world.push_back({ 0.f, 2.f, 0.f });
	m_Vertices_world.push_back({ 1.f, 0.f, 0.f });
	m_Vertices_world.push_back({-1.f, 0.f, 0.f });

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			// World Space Coordinates --> View Space
			m_Vertices_viewspace.clear();
			for (const auto& vertex : m_Vertices_world)
			{
				m_Vertices_viewspace.push_back(m_Camera.invViewMatrix.TransformPoint(vertex.position));
			}
			// perspective divide
			for (auto& vertex : m_Vertices_viewspace)
			{
				vertex.x = vertex.x / (vertex.z * m_Camera.fov * m_AspectRatio);
				vertex.y = vertex.y / (vertex.z * m_Camera.fov);
			}

			// Normalized Device Coordinates --> Screen Space Coordinates
			VertexTransformationFunction(m_Vertices_viewspace, m_Vertices_ssc);

			const Vector2 pixel_ssc{ float(px) + 0.5f , float(py) + 0.5f };
			ColorRGB finalColor{};
			ColorRGB weights{};

			// Is pixel in triangle?
			if (IsPixelInTriangle(pixel_ssc, m_Vertices_ssc,weights))
			{
				finalColor.r = weights.r;
				finalColor.g = weights.g;
				finalColor.b = weights.b;
			}



			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vector3>& vertices_in, std::vector<Vector3>& vertices_out) const
{
	vertices_out.clear();
	Vector3 screenSpaceVertex{0,0,1};

	for (const auto& vertex : vertices_in)
	{
		screenSpaceVertex.x = ((1 + vertex.x) / 2) * m_Width;
		screenSpaceVertex.y = ((1 - vertex.y) / 2) * m_Height;
		vertices_out.emplace_back(screenSpaceVertex);
	}
}


bool Renderer::IsPixelInTriangle(Vector2 pixel_ssc, std::vector<Vector3>& triangleVertices, ColorRGB& weights) const
{
	Vector2 edge{	triangleVertices[1].x - triangleVertices[0].x,
					triangleVertices[1].y - triangleVertices[0].y };

	Vector2 pointToSide{	pixel_ssc.x - triangleVertices[0].x ,
							pixel_ssc.y - triangleVertices[0].y };

	const float TotalAreaParallelogram{ Vector2::Cross(edge, Vector2{triangleVertices[2].x - triangleVertices[0].x,triangleVertices[2].y - triangleVertices[0].y}) };

	weights.r = Vector2::Cross(edge, pointToSide);
	if (weights.r < 0.f)
		return false;

	edge.x = triangleVertices[2].x - triangleVertices[1].x;
	edge.y = triangleVertices[2].y - triangleVertices[1].y;

	pointToSide.x = pixel_ssc.x - triangleVertices[1].x;
	pointToSide.y = pixel_ssc.y - triangleVertices[1].y;

	weights.g = Vector2::Cross(edge, pointToSide);
	if (weights.g < 0.f)
		return false;

	edge.x = triangleVertices[0].x - triangleVertices[2].x;
	edge.y = triangleVertices[0].y - triangleVertices[2].y;

	pointToSide.x = pixel_ssc.x - triangleVertices[2].x;
	pointToSide.y = pixel_ssc.y - triangleVertices[2].y;

	weights.b = Vector2::Cross(edge, pointToSide);
	if (weights.b < 0.f)
		return false;

	weights.r /= TotalAreaParallelogram;
	weights.g /= TotalAreaParallelogram;
	weights.b /= TotalAreaParallelogram;

	return true;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}


