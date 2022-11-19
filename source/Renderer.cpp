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
	Solution_W1();
	
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

		const Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
		const Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
		const Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

		Vector2 topLeft{  std::min(std::min(triangle[0].position.x,triangle[1].position.x), triangle[2].position.x),
								std::min(std::min(triangle[0].position.y,triangle[1].position.y), triangle[2].position.y) };
		Vector2 bottomRight{  std::max(std::max(triangle[0].position.x,triangle[1].position.x), triangle[2].position.x),
									std::max(std::max(triangle[0].position.y,triangle[1].position.y), triangle[2].position.y) };
		
		topLeft.x = Clamp(topLeft.x, 0.f, float(m_Width - 1));
		topLeft.y = Clamp(topLeft.y, 0.f, float(m_Height - 1));
		bottomRight.x = Clamp(bottomRight.x, 0.f, float(m_Width - 1));
		bottomRight.y = Clamp(bottomRight.y, 0.f, float(m_Height - 1));

		for (int px{int(topLeft.x)}; px < int(bottomRight.x); ++px)
		{
			for (int py{int(topLeft.y)}; py < int(bottomRight.y); ++py)
			{
				Vector3 weights{};
				ColorRGB finalColor{};
				const Vector2 pixel_ssc{ float(px) + 0.5f , float(py) + 0.5f };

				weights.x = Vector2::Cross(Vector2(v1, v2), Vector2(v1, pixel_ssc));
				weights.y = Vector2::Cross(Vector2(v2, v0), Vector2(v2, pixel_ssc));
				weights.z = Vector2::Cross(Vector2(v0, v1), Vector2(v0, pixel_ssc));

				const Vector3 point = triangle[0].position * weights.x + triangle[1].position * weights.y + triangle[2].position * weights.z;

				if (IsPointInTriangle(triangle,point,weights))
				{
					if (point.z <  m_pDepthBufferPixels[py * m_Width + px])
					{
						m_pDepthBufferPixels[py * m_Width + px] = point.z;
						finalColor = triangle[0].color * weights.x + triangle[1].color * weights.y + triangle[2].color * weights.z;

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
}

bool Renderer::IsPointInTriangle(const std::vector<Vertex>& triangle, const Vector3& point, const Vector3& weights) const
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
	std::vector<Mesh> mesh_world
	{
		Mesh{
				{
					Vertex(-3, 3,-2),
					Vertex( 0, 3,-2),
					Vertex( 3, 3,-2),
					Vertex(-3, 0,-2),
					Vertex( 0, 0,-2),
					Vertex( 3, 0,-2),
					Vertex(-3,-3,-2),
					Vertex( 0,-3,-2),
					Vertex(-3,-3,-2)
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


	};
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}


