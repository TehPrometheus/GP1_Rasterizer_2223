
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

		enum class ShadingMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined
		};


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

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		bool m_IsRotating{};
		bool m_IsNormalMapEnabled{};
		ShadingMode m_ShadingMode{ ShadingMode::Combined };


		int m_Width{};
		int m_Height{};

		float m_AspectRatio{};
		float m_TuktukYaw{};
		float m_VehicleYaw{};

		Texture* m_pTextureUVGrid{};
		Texture* m_pTextureTukTuk{};
		Texture* m_pTextureVehicle{};
		Texture* m_pNormalMapVehicle{};
		Texture* m_pGlossMapVehicle{};
		Texture* m_pSpecularMapVehicle{};

		Mesh* m_pVehicleMesh{};

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out); //W1 Version
		void VertexTransformationMatrix(Mesh* Mesh);
		bool IsPointInTriangle(const Vector3& weights) const;
		void Solution_W1();
		void Solution_W2_W3();
		void Solution_W4();
		void Solution_W5();

		float CalculateWeights(const Vector2& vertex1, const Vector2& vertex2, const Vector2& pixel, float area) const;
		void RenderTriangle_W3(const std::vector<Vertex>& triangleScreenSpace) const;
		void RenderTriangle_W4(std::vector<Vertex_Out>& triangle) const;
		void RenderTriangle_W5(std::vector<Vertex_Out>& triangle) const;
		bool IsFrustumCullingRequired(std::vector<Vertex_Out>& triangle) const;
		ColorRGB PixelShading(const Vertex_Out& v) const ;
		void FindBoundingBoxCorners(Vector2& topLeft, Vector2& botRight, const std::vector<Vertex_Out>& triangle) const ;
	};

}
