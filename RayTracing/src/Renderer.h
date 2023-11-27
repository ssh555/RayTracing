#pragma once

#include "Walnut\Image.h"
#include "memory"
#include "glm\ext\vector_float2.hpp"
#include "glm\ext\vector_float4.hpp"
#include "Camera.h"
#include "Ray.h"
#include "Scene.h"
#include <vector>



class Renderer
{
public:
	struct Settings
	{
		bool Accumulate = true;
		bool SlowRandom = true;
	};

public:
	Renderer() = default;
	~Renderer();

	void Render(const Scene& scene, const Camera& camera);

	void OnResize(uint32_t width, uint32_t height);

	inline std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

	inline void ResetFrameIndex() { m_FrameIndex = 1; }

	inline Settings& GetSettings() { return m_Settings; }

private:
	struct HitPayLoad
	{
		float HitDistance;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;

		int ObjectIndex;
	};

	// RayGen
	glm::vec4 PerPixel(uint32_t x, uint32_t y);

	HitPayLoad TraceRay(const Ray& ray);
	HitPayLoad ClosestRay(const Ray& ray, float hitDistance, int objectIndex);
	HitPayLoad Miss(const Ray& ray);



private:
	Settings m_Settings;

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;


	std::vector<uint32_t> m_ImageHorizontalIter, m_ImageVerticalIter;
	std::shared_ptr<Walnut::Image> m_FinalImage;
	uint32_t* m_ImageData = nullptr;
	glm::vec4* m_AccumulationData = nullptr;

	uint32_t m_FrameIndex = 1;
};
