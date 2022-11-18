#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;
	class Texture;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		//float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};
		float m_AspectRatio{};
		//TODO: start here: convert these containers to vertex containers
		std::vector<Vertex> m_Vertices_world{};
		std::vector<Vector3> m_Vertices_viewspace{};
		std::vector<Vector3> m_Vertices_ssc{};
		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(const std::vector<Vector3>& vertices_in, std::vector<Vector3>& vertices_out) const; //W1 Version
		bool IsPixelInTriangle(Vector2 pixel_ssc, std::vector<Vector3>& triangleVertices) const;
	};
}
