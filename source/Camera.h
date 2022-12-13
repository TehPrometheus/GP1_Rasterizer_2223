#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <iostream>
#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(float _fovAngle, const Vector3& _origin ):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}

		const float movementSpeed{ 20.f };
		const float rotationSpeed{ 150.f };

		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		const float near{ 0.1f };
		const float far{ 100.f };
		float aspectRatio{};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix ProjectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f},float _aspectRatio = 1)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			aspectRatio = _aspectRatio;
		}

		void CalculateViewMatrix()
		{
			Matrix rotationMatrix = Matrix::CreateRotationX(totalPitch * TO_RADIANS) * Matrix::CreateRotationY(totalYaw * TO_RADIANS);

			forward = rotationMatrix.GetAxisZ();
			right = rotationMatrix.GetAxisX();
			up = rotationMatrix.GetAxisY();

			Matrix ONB
			{
				Vector4{ right, 0.f},
				Vector4{ up, 0.f},
				Vector4{forward, 0.f},
				Vector4{ origin ,1.f}
			};

			viewMatrix = ONB;
			invViewMatrix = Matrix::Inverse(viewMatrix);

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2

			ProjectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, near, far);
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += movementSpeed * deltaTime * forward;
			}
			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin += movementSpeed * deltaTime * -right;
			}
			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin += movementSpeed * deltaTime * -forward;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += movementSpeed * deltaTime * right;
			}

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
			if (mouseState & SDL_BUTTON(1))
			{

				if (mouseState & SDL_BUTTON(3))
				{
					if (mouseY > 0)
					{
						origin += movementSpeed * deltaTime * -up;
					}
					else if (mouseY < 0)
					{
						origin += movementSpeed * deltaTime * up;
					}
				}
				else
				{
					if (mouseY > 0)
					{
						origin += movementSpeed * deltaTime * -forward;
					}
					else if (mouseY < 0)
					{
						origin += movementSpeed * deltaTime * forward;
					}
					if (mouseX > 0)
					{
						totalYaw += rotationSpeed * deltaTime;
					}
					else if (mouseX < 0)
					{
						totalYaw -= rotationSpeed * deltaTime;
					}
				}


			}
			else if (mouseState & SDL_BUTTON(3))
			{

				if (mouseX < 0)
				{
					totalYaw -= rotationSpeed * deltaTime;
				}
				else if (mouseX > 0)
				{
					totalYaw += rotationSpeed * deltaTime;
				}
				if (mouseY > 0)
				{
					totalPitch -= rotationSpeed * deltaTime;
				}
				else if (mouseY < 0)
				{
					totalPitch += rotationSpeed * deltaTime;
				}
			}
			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
