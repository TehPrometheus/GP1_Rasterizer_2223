#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <iostream>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		SDL_Surface* pSurface{ IMG_Load( path.c_str() ) };
		Texture* pTexture{ nullptr };

		if (pSurface == NULL)
		{
			std::cout << "Error: unable to load image from file in function " << __func__ << "\n";
			return nullptr;
		}
		
		pTexture = new Texture(pSurface);
		
		return pTexture;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		const float uvX{ Saturate(uv.x) };
		const float uvY{ Saturate(uv.y) };

		ColorRGB color{};

		Uint8 r{};
		Uint8 g{};
		Uint8 b{};

		float width { static_cast<float>(m_pSurface->w) };
		float height{ static_cast<float>(m_pSurface->h) };

		uint32_t px{ static_cast<uint32_t>( width * uvX) },
				 py{ static_cast<uint32_t>(height * uvY) };

		SDL_GetRGB(m_pSurfacePixels[px + (py * m_pSurface->w)], m_pSurface->format, &r, &g, &b);

		color.r = static_cast<float>(r);
		color.g = static_cast<float>(g);
		color.b = static_cast<float>(b);

		color /= 255.f;

		return color;
	}
}