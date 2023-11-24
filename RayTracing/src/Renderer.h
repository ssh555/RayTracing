#pragma once

#include "Walnut\Image.h"
#include "memory"
#include "glm\ext\vector_float2.hpp"
#include "glm\ext\vector_float4.hpp"
#include "Camera.h"
#include "Ray.h"
#include "Scene.h"



class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	void Render(const Scene& scene, const Camera& camera);

	void OnResize(uint32_t width, uint32_t height);

	inline std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

private:
	glm::vec4 TraceRay(const Scene& scene, const Ray& ray);



private:
	std::shared_ptr<Walnut::Image> m_FinalImage;
	uint32_t* m_ImageData = nullptr;
};

