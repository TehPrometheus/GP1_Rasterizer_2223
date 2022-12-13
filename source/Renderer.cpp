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
	: m_pWindow(pWindow),
	m_IsRotating{true},
	m_IsNormalMapEnabled{true}
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_AspectRatio = (float)m_Width / (float)m_Height;

	//Initialize Textures
	m_pTextureUVGrid		= Texture::LoadFromFile("Resources/uv_grid_2.png");
	m_pTextureTukTuk		= Texture::LoadFromFile("Resources/tuktuk.png");
	m_pTextureVehicle		= Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pNormalMapVehicle		= Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pGlossMapVehicle		= Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pSpecularMapVehicle	= Texture::LoadFromFile("Resources/vehicle_specular.png");

	m_VehicleYaw = PI_DIV_2;

	//Create Buffers
	m_pFrontBuffer			= SDL_GetWindowSurface(pWindow);
	m_pBackBuffer			= SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels		= (uint32_t*)m_pBackBuffer->pixels;
	m_pDepthBufferPixels	= new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,0.0f,-45.f }, m_AspectRatio);

	//Initialize Mesh
	m_pVehicleMesh = new Mesh();
	m_pVehicleMesh->primitiveTopology = PrimitiveTopology::TriangleList;
	Utils::ParseOBJ("Resources/vehicle.obj", m_pVehicleMesh->vertices, m_pVehicleMesh->indices);
	m_pVehicleMesh->vertices_out.resize(m_pVehicleMesh->vertices.size());

}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTextureUVGrid;
	delete m_pTextureTukTuk;
	delete m_pTextureVehicle;
	delete m_pVehicleMesh;
	delete m_pNormalMapVehicle;
	delete m_pGlossMapVehicle;
	delete m_pSpecularMapVehicle;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	m_TuktukYaw  = PI_DIV_2 * pTimer->GetTotal();
	if (m_IsRotating)
	{
		m_VehicleYaw = PI_DIV_2 * pTimer->GetTotal();
	}
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
	//Solution_W2_W3();
	//Solution_W4();
	Solution_W5();
	

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::Solution_W5()
{
	//Projection Stage
	m_pVehicleMesh->worldMatrix = Matrix::CreateRotationY(m_VehicleYaw);
	VertexTransformationMatrix(m_pVehicleMesh);


	for (size_t idx = 0; idx < m_pVehicleMesh->indices.size(); idx += 3)
	{
		std::vector<Vertex_Out> triangle{};

		triangle.push_back(m_pVehicleMesh->vertices_out[m_pVehicleMesh->indices[idx + 0]]);
		triangle.push_back(m_pVehicleMesh->vertices_out[m_pVehicleMesh->indices[idx + 1]]);
		triangle.push_back(m_pVehicleMesh->vertices_out[m_pVehicleMesh->indices[idx + 2]]);

		// Optimisation Stage
		if (IsFrustumCullingRequired(triangle))
		{
			continue;
		}

		// NDC -> Screen Space Coordinates
		for (Vertex_Out& vertex : triangle)
		{
			vertex.position.x = ((1 + vertex.position.x) / 2) * m_Width;
			vertex.position.y = ((1 - vertex.position.y) / 2) * m_Height;
		}

		// Rasterization Stage
		RenderTriangle_W5(triangle);
	}
}

void Renderer::VertexTransformationMatrix(Mesh* Mesh)
{
	/*
	Optimizations
	* Declare and define vertex_out outside the loop. Right now the destructor is being called at the end of each loop
	* Don't pushback the values. Instead reserve and resize the vertices_out vector and fill in the values at idx
	*/

	Matrix WorldViewProjectionMatrix
	{
		Mesh->worldMatrix * m_Camera.invViewMatrix * m_Camera.ProjectionMatrix
	};

	for (size_t idx = 0; idx < Mesh->vertices.size(); ++idx)
	{
		Vertex_Out vertex_out{ Vertex_Out(
							   {Mesh->vertices[idx].position.x,
								Mesh->vertices[idx].position.y,
								Mesh->vertices[idx].position.z,
								1}
								,
								Mesh->vertices[idx].uv
								,
								Mesh->vertices[idx].color,
								Mesh->vertices[idx].normal,
								Mesh->vertices[idx].tangent) };

		// Position Transformation To NDC
		vertex_out.position = WorldViewProjectionMatrix.TransformPoint(vertex_out.position);

		// Perspective Divide
		vertex_out.position.x /= vertex_out.position.w;
		vertex_out.position.y /= vertex_out.position.w;
		vertex_out.position.z /= vertex_out.position.w;

		// Normal & Tangent Transformation To World Space
		vertex_out.normal = Mesh->worldMatrix.TransformVector(vertex_out.normal);
		vertex_out.tangent = Mesh->worldMatrix.TransformVector(vertex_out.tangent);
		vertex_out.normal.Normalize();
		vertex_out.tangent.Normalize();

		// Create ViewDirection
		vertex_out.viewDirection = Mesh->worldMatrix.TransformVector(Mesh->vertices[idx].position) - m_Camera.origin;

		// NDC Output
		Mesh->vertices_out[idx] = vertex_out;
	}

}


void Renderer::FindBoundingBoxCorners(Vector2& topLeft, Vector2& botRight,const std::vector<Vertex_Out>& triangle) const
{
	topLeft.x = std::min(std::min(triangle[0].position.x, triangle[1].position.x), triangle[2].position.x);
	topLeft.x = Clamp(topLeft.x, 0.f, float(m_Width - 1));
	topLeft.y = std::min(std::min(triangle[0].position.y, triangle[1].position.y), triangle[2].position.y);
	topLeft.y = Clamp(topLeft.y, 0.f, float(m_Height - 1));

	botRight.x = std::max(std::max(triangle[0].position.x, triangle[1].position.x), triangle[2].position.x);
	botRight.x = Clamp(botRight.x, 0.f, float(m_Width - 1));
	botRight.x = std::ceil(botRight.x);

	botRight.y = std::max(std::max(triangle[0].position.y, triangle[1].position.y), triangle[2].position.y);
	botRight.y = Clamp(botRight.y, 0.f, float(m_Height - 1));
	botRight.y = std::ceil(botRight.y);


}

void Renderer::RenderTriangle_W5(std::vector<Vertex_Out>& triangle) const
{
	const Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
	const Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
	const Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

	Vector2 topLeft{};
	Vector2 botRight{};
	FindBoundingBoxCorners(topLeft, botRight, triangle);


	for (int py{ int(topLeft.y) }; py < int(botRight.y); ++py)
	{
		for (int px{ int(topLeft.x) }; px < int(botRight.x); ++px)
		{
			Vector3 weights{};
			ColorRGB finalColor{};
			const Vector2 pixel_ssc{ float(px) + 0.5f , float(py) + 0.5f };

			const float area{ Vector2::Cross(Vector2(v1, v2), Vector2(v1, v0)) };

			weights.x = CalculateWeights(v1, v2, pixel_ssc, area);
			weights.y = CalculateWeights(v2, v0, pixel_ssc, area);
			weights.z = CalculateWeights(v0, v1, pixel_ssc, area);

			if (IsPointInTriangle(weights))
			{
				float zBufferValue{ 1 / ((weights.x / triangle[0].position.z) + (weights.y / triangle[1].position.z) + (weights.z / triangle[2].position.z)) };

				if (zBufferValue < m_pDepthBufferPixels[px + py * m_Width])
				{
					m_pDepthBufferPixels[px + (py * m_Width)] = zBufferValue;

					const float wInterpolated{ 1 / ((weights.x / triangle[0].position.w) + (weights.y / triangle[1].position.w) + (weights.z / triangle[2].position.w)) };

					Vector2 uvInterpolated{ wInterpolated   *  (weights.x * (triangle[0].uv / triangle[0].position.w) +
																weights.y * (triangle[1].uv / triangle[1].position.w) +
																weights.z * (triangle[2].uv / triangle[2].position.w)) };

					Vertex_Out interpolatedData{};
					interpolatedData.uv = uvInterpolated;
					interpolatedData.color = m_pTextureVehicle->Sample(uvInterpolated);
					interpolatedData.normal = ((triangle[0].normal / triangle[0].position.w) * weights.x +
											   (triangle[1].normal / triangle[1].position.w) * weights.y +
											   (triangle[2].normal / triangle[2].position.w) * weights.z) * wInterpolated;
					interpolatedData.normal.Normalize();

					interpolatedData.tangent =  ((triangle[0].tangent / triangle[0].position.w) * weights.x +
												 (triangle[1].tangent / triangle[1].position.w) * weights.y +
												 (triangle[2].tangent / triangle[2].position.w) * weights.z) * wInterpolated;
					interpolatedData.tangent.Normalize();

					interpolatedData.viewDirection =   ((triangle[0].viewDirection / triangle[0].position.w) * weights.x +
														(triangle[1].viewDirection / triangle[1].position.w) * weights.y +
														(triangle[2].viewDirection / triangle[2].position.w) * weights.z) * wInterpolated;
					interpolatedData.viewDirection.Normalize();

					finalColor = PixelShading(interpolatedData);



					//finalColor = m_pTextureVehicle->Sample(uvInterpolated);
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

ColorRGB Renderer::PixelShading(const Vertex_Out& v) const 
{
	ColorRGB shading{};
	Vector3 lightDirection{ 0.577f, -0.577f ,0.577f };

	//Construct correct normal
	Vector3 binormal{ Vector3::Cross(v.normal,v.tangent) };
	Matrix tangentSpaceAxis = Matrix{ v.tangent,binormal,v.normal,Vector3{} };
	ColorRGB normalMapSample{ m_pNormalMapVehicle->Sample(v.uv)};
	Vector3 sampledNormal{ normalMapSample.r, normalMapSample.g, normalMapSample.b };
	sampledNormal.x = (2.f * sampledNormal.x) - 1.f;
	sampledNormal.y = (2.f * sampledNormal.y) - 1.f;
	sampledNormal.z = (2.f * sampledNormal.z) - 1.f;
	sampledNormal /= 255.f;
	sampledNormal = tangentSpaceAxis.TransformVector(sampledNormal);

	
	//LambertCosine
	float lambertCosine{ Vector3::Dot(sampledNormal.Normalized(),-lightDirection)};
	if (lambertCosine > 0.00001f)
	{
		shading += ColorRGB{ lambertCosine ,lambertCosine ,lambertCosine };
	}

	//Lambert Diffuse
	ColorRGB lambertDiffuse{};
	const float kd{ 7.f }; //diffuseReflectionCoefficient
	lambertDiffuse = m_pTextureVehicle->Sample(v.uv) * kd / static_cast<float>(M_PI);
	
	
	//Phong
	const Vector3 reflect{ lightDirection - 2 * (Vector3::Dot(v.normal,lightDirection)) * lightDirection };
	const float cosine{ std::max(Vector3::Dot(reflect, -v.viewDirection),0.f) };
	const float phongExponent{ m_pGlossMapVehicle->Sample(v.uv).r };
	const float shininess{ 25.f };
	const float ks{ 1.f };
	ColorRGB specularColor{ m_pSpecularMapVehicle->Sample(v.uv) };
	const float phongSpecularReflection{ ks * powf(cosine,phongExponent * shininess) };
	const ColorRGB phong{ specularColor * phongSpecularReflection };

	shading *= (lambertDiffuse + phong);

	return shading;
}

bool Renderer::IsFrustumCullingRequired(std::vector<Vertex_Out>& triangle) const
{
	for (Vertex_Out& vertex : triangle)
	{
		if (vertex.position.x < -1.f || vertex.position.x > 1.f ||
			vertex.position.y < -1.f || vertex.position.y > 1.f ||
			vertex.position.z <  0.f || vertex.position.z > 1.f )
		{
			return true;
		}
	}

	return false;
}

bool Renderer::IsPointInTriangle(const Vector3& weights) const
{
	if ((weights.x > 0) && (weights.y > 0) && (weights.z > 0))
	{
		return true;
	}

	return false;
}

void Renderer::Solution_W1()
{
	std::vector<Vertex> vertices_world
	{
		//triangle1
		Vertex({ 0.0f,  2.0f, 0.0f},  {0,0}, {1, 0, 0}),
		Vertex({ 1.5f, -1.0f, 0.0f},  {0,0}, {1, 0, 0}),
		Vertex({-1.5f, -1.0f, 0.0f},  {0,0}, {1, 0, 0}),
										  
		///triangle 2					  
		Vertex({ 0.0f,  4.0f, 2.0f},  {0,0}, {1, 0, 0}),
		Vertex({ 3.0f, -2.0f, 2.0f},  {0,0}, {0, 1, 0}),
		Vertex({-3.0f, -2.0f, 2.0f},  {0,0}, {0, 0, 1})
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
		
		RenderTriangle_W3(triangle);
	}
}

void Renderer::Solution_W2_W3()
{
	std::vector<Mesh> meshWorld
	{
		Mesh
		{
			{
				Vertex({-3, 3,-2}, {   0,   0}),
				Vertex({ 0, 3,-2}, {0.5f,   0}),
				Vertex({ 3, 3,-2}, {   1,   0}),
				Vertex({-3, 0,-2}, {   0,0.5f}),
				Vertex({ 0, 0,-2}, {0.5f,0.5f}),
				Vertex({ 3, 0,-2}, {   1,0.5f}),
				Vertex({-3,-3,-2}, {   0,   1}),
				Vertex({ 0,-3,-2}, {0.5f,   1}),
				Vertex({ 3,-3,-2}, {   1,   1})
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
				Vertex({-3, 3,-2}, {   0,   0}),
				Vertex({ 0, 3,-2}, {0.5f,   0}),
				Vertex({ 3, 3,-2}, {   1,   0}),
				Vertex({-3, 0,-2}, {   0,0.5f}),
				Vertex({ 0, 0,-2}, {0.5f,0.5f}),
				Vertex({ 3, 0,-2}, {   1,0.5f}),
				Vertex({-3,-3,-2}, {   0,   1}),
				Vertex({ 0,-3,-2}, {0.5f,   1}),
				Vertex({ 3,-3,-2}, {   1,   1})
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

				RenderTriangle_W3(triangle);
			}
		}
	}
}

//void Renderer::Solution_W4()
//{
//	// Input
//	std::vector<Mesh> meshWorldStrip
//	{
//		Mesh
//		{
//			{
//				Vertex({-3, 3,-2}, {   0,   0}),
//				Vertex({ 0, 3,-2}, {0.5f,   0}),
//				Vertex({ 3, 3,-2}, {   1,   0}),
//				Vertex({-3, 0,-2}, {   0,0.5f}),
//				Vertex({ 0, 0,-2}, {0.5f,0.5f}),
//				Vertex({ 3, 0,-2}, {   1,0.5f}),
//				Vertex({-3,-3,-2}, {   0,   1}),
//				Vertex({ 0,-3,-2}, {0.5f,   1}),
//				Vertex({ 3,-3,-2}, {   1,   1})
//			}
//			,
//			{
//				3,0,4,
//				1,5,2,
//				2,6,6,
//				3,7,4,
//				8,5
//			}
//			,
//			PrimitiveTopology::TriangleStrip
//		}
//	};
//
//	std::vector<Mesh> meshWorldList
//	{
//
//		Mesh
//		{
//			{
//				Vertex({-3, 3,-2}, {   0,   0}),
//				Vertex({ 0, 3,-2}, {0.5f,   0}),
//				Vertex({ 3, 3,-2}, {   1,   0}),
//				Vertex({-3, 0,-2}, {   0,0.5f}),
//				Vertex({ 0, 0,-2}, {0.5f,0.5f}),
//				Vertex({ 3, 0,-2}, {   1,0.5f}),
//				Vertex({-3,-3,-2}, {   0,   1}),
//				Vertex({ 0,-3,-2}, {0.5f,   1}),
//				Vertex({ 3,-3,-2}, {   1,   1})
//			}
//			,
//			{
//				3,0,1,
//				1,4,3,
//				4,1,2,
//				2,5,4,
//				6,3,4,
//				4,7,6,
//				7,4,5,
//				5,8,7
//			}
//			,
//			PrimitiveTopology::TriangleList
//		}
//	};
//
//	Mesh tuktukMesh{};
//	Utils::ParseOBJ("Resources/tuktuk.obj", tuktukMesh.vertices, tuktukMesh.indices);
//	tuktukMesh.worldMatrix = Matrix::CreateRotationY(m_TuktukYaw);
//
//	// Projection Stage
//	VertexTransformationMatrix(tuktukMesh);
//
//	for (size_t idx = 0; idx < tuktukMesh.indices.size(); idx += 3)
//	{
//		std::vector<Vertex_Out> triangle{};
//
//		triangle.push_back(tuktukMesh.vertices_out[tuktukMesh.indices[idx + 0]]);
//		triangle.push_back(tuktukMesh.vertices_out[tuktukMesh.indices[idx + 1]]);
//		triangle.push_back(tuktukMesh.vertices_out[tuktukMesh.indices[idx + 2]]);
//
//		// Optimisation Stage
//		if (IsFrustumCullingRequired(triangle))
//		{
//			continue;
//		}
//
//		// Rasterization Stage
//		RenderTriangle_W4(triangle);
//	}
//}

void Renderer::RenderTriangle_W3(const std::vector<Vertex>& triangleScreenSpace) const
{	
	//// NDC -> Screen Space
	//triangle[0].position.x = ((1 + triangle[0].position.x) / 2) * m_Width;
	//triangle[0].position.y = ((1 - triangle[0].position.y) / 2) * m_Height;
	//triangle[1].position.x = ((1 + triangle[1].position.x) / 2) * m_Width;
	//triangle[1].position.y = ((1 - triangle[1].position.y) / 2) * m_Height;
	//triangle[2].position.x = ((1 + triangle[2].position.x) / 2) * m_Width;
	//triangle[2].position.y = ((1 - triangle[2].position.y) / 2) * m_Height;

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
					finalColor = m_pTextureUVGrid->Sample(uvInterpolated);
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

void Renderer::RenderTriangle_W4(std::vector<Vertex_Out>& triangle) const
{
	// NDC -> Screen Space Coordinates
	for (Vertex_Out& vertex : triangle)
	{
		vertex.position.x = ((1 + vertex.position.x) / 2) * m_Width;
		vertex.position.y = ((1 - vertex.position.y) / 2) * m_Height;
	}

	const Vector2 v0{ triangle[0].position.x, triangle[0].position.y };
	const Vector2 v1{ triangle[1].position.x, triangle[1].position.y };
	const Vector2 v2{ triangle[2].position.x, triangle[2].position.y };

	Vector2 topLeft{	std::min(std::min(triangle[0].position.x,triangle[1].position.x), triangle[2].position.x),
						std::min(std::min(triangle[0].position.y,triangle[1].position.y), triangle[2].position.y) };

	Vector2 bottomRight{	std::max(std::max(triangle[0].position.x,triangle[1].position.x), triangle[2].position.x),
							std::max(std::max(triangle[0].position.y,triangle[1].position.y), triangle[2].position.y) };

	topLeft.x = Clamp(topLeft.x, 0.f, float(m_Width - 1));
	topLeft.y = Clamp(topLeft.y, 0.f, float(m_Height - 1));
	bottomRight.x = Clamp(bottomRight.x, 0.f, float(m_Width - 1));
	bottomRight.y = Clamp(bottomRight.y, 0.f, float(m_Height - 1));

	for (int py{ int(topLeft.y) }; py < int(bottomRight.y); ++py)
	{
		for (int px{ int(topLeft.x) }; px < int(bottomRight.x); ++px)
		{
			Vector3 weights{};
			ColorRGB finalColor{};
			const Vector2 pixel_ssc{ float(px) + 0.5f , float(py) + 0.5f };

			const float area{ Vector2::Cross(Vector2(v1, v2), Vector2(v1, v0)) };

			weights.x = CalculateWeights(v1, v2, pixel_ssc, area);
			weights.y = CalculateWeights(v2, v0, pixel_ssc, area);
			weights.z = CalculateWeights(v0, v1, pixel_ssc, area);

			if (IsPointInTriangle(weights))
			{
				float zBufferValue{ 1 / ( (weights.x / triangle[0].position.z) + (weights.y / triangle[1].position.z) + (weights.z / triangle[2].position.z)) };

				if (zBufferValue < m_pDepthBufferPixels[px + py * m_Width])
				{
					m_pDepthBufferPixels[px + (py * m_Width)] = zBufferValue;

					const float wInterpolated{ 1 / ( (weights.x / triangle[0].position.w) + (weights.y / triangle[1].position.w) + (weights.z / triangle[2].position.w)) };

					Vector2 uvInterpolated{ wInterpolated * (weights.x * (triangle[0].uv / triangle[0].position.w) +
																weights.y * (triangle[1].uv / triangle[1].position.w) +
																weights.z * (triangle[2].uv / triangle[2].position.w) )};

					finalColor = m_pTextureTukTuk->Sample(uvInterpolated);
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

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

float Renderer::CalculateWeights(const Vector2& vertex1, const Vector2& vertex2, const Vector2& pixel, float area) const
{
	return Vector2::Cross(vertex2 - vertex1, pixel - vertex1) / area;
}

