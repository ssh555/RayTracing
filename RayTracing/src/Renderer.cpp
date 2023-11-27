#include "Renderer.h"
#include "Walnut/Random.h"
#include <algorithm>
#include <thread>
#include <execution>

namespace Utils
{
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		return ((uint8_t)(color.a*255.0f) << 24) | ((uint8_t)(color.b * 255.0f) << 16) | ((uint8_t)(color.g * 255.0f) << 8) | ((uint8_t)(color.r * 255.0f));
	}

	static uint32_t PCG_Hash(uint32_t input)
	{
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	static float RandomFloat(uint32_t& seed)
	{
		seed = PCG_Hash(seed);
		return (float)seed / (float)std::numeric_limits<uint32_t>::max();
	}

	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(RandomFloat(seed) * 2.0f - 1.0f, RandomFloat(seed) * 2.0f - 1.0f, RandomFloat(seed) * 2.0f - 1.0f));
	}
}

Renderer::~Renderer()
{
	delete[] m_ImageData;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	if (m_FrameIndex == 1)
	{
		memset(m_AccumulationData, 0, (size_t)m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));
	}

	// render every pixel
#define MT 1
#if MT
	// 当前设备可以创建的线程数(core)
	//std::thread::hardware_concurrency();
	// 从 C++17 开始可以单线程执行，也可以多线程执行
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y) 
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
				[this, y](uint32_t x)
				{
					uint32_t index = x + y * m_FinalImage->GetWidth();

					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[index] += color;

					glm::vec4 accumulateColor = m_AccumulationData[index];
					accumulateColor /= (float)m_FrameIndex;

					accumulateColor = glm::clamp(accumulateColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[index] = Utils::ConvertToRGBA(accumulateColor);
				});
		});
#else
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); ++y)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); ++x)
		{
			uint32_t index = x + y * m_FinalImage->GetWidth();

			glm::vec4 color = PerPixel(x, y);
			m_AccumulationData[index] += color;

			glm::vec4 accumulateColor = m_AccumulationData[index];
			accumulateColor /= (float)m_FrameIndex;

			accumulateColor = glm::clamp(accumulateColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[index] = Utils::ConvertToRGBA(accumulateColor);
		}
	}
#endif

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
	{
		++m_FrameIndex;
	}
	else
	{
		m_FrameIndex = 1;
	}
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		// No resize necessary
		if (m_FinalImage->GetHeight() == height && m_FinalImage->GetWidth() == width)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * (size_t)height];
	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * (size_t)height];

	m_ImageHorizontalIter.resize(width);
	for (uint32_t i = 0; i < width; ++i)
		m_ImageHorizontalIter[i] = i;
	m_ImageVerticalIter.resize(height);
	for (uint32_t i = 0; i < height; ++i)
		m_ImageVerticalIter[i] = i;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * (size_t)m_FinalImage->GetWidth()];


	glm::vec3 light(0.0f);
	glm::vec3 throughput(1.0f);

	uint32_t seed = (x + y * (size_t)m_FinalImage->GetWidth()) * m_FrameIndex;

	int bounces = 5;
	for (int i = 0; i < bounces; ++i)
	{
		seed += i;

		Renderer::HitPayLoad payLoad = TraceRay(ray);

		// 天空光
		if (payLoad.HitDistance < 0.0f)
		{
			glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
			light += skyColor * throughput;
			break;
		}


		const Sphere& sphere = m_ActiveScene->Spheres[payLoad.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

		// 自发光
		//light += material.GetEmission();
		light += material.GetEmission() * throughput;

		// 可通过量过滤
		//throughput *= material.Albedo;
		throughput *= glm::mix(material.Albedo, glm::vec3(1.0f), material.Metallic);

		// 其余光源
		// maybe : 遍历所有光源 -> 处于影响范围内 -> 在该点处的光 -> 法线计算 -> 光的影响(light += lightColor * lightPower * throughput)(即:类似于天空光,计算时需过滤可通过量)

		ray.Origin = payLoad.WorldPosition + payLoad.WorldNormal * 0.0001f;
		// static thread_local -> 为每个线程创建一个实例
		if (m_Settings.SlowRandom)
		{
			ray.Direction = glm::normalize(payLoad.WorldNormal + Walnut::Random::InUnitSphere());
		}
		else
		{
			ray.Direction = glm::normalize(payLoad.WorldNormal + Utils::InUnitSphere(seed));
		}
	}


	return glm::vec4(light, 1.0f);
}

Renderer::HitPayLoad Renderer::TraceRay(const Ray& ray)
{
	// b^2 * t^2 + 2(a * b) * t + a^2 - r^2 = 0
	// sphere is in (0, 0, 0)
	// a = ray origin
	// b = ray direction
	// r = radius
	// t = hit distance

	int closestSphere = -1;
	//float hitDistance = FLT_MAX;
	float hitDistance = std::numeric_limits<float>::max();
	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); ++i)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];

		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		// Quadratic formula discriminant:
		// b^2 - 4ac
		float discriminant = b * b - 4.0f * a * c;

		// (-b +|- sqrt(discriminant)) / 2a

		if (discriminant < 0.0f)
		{
			continue;
		}

		//float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a);
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = (int)i;
		}
	}
	
	if (closestSphere < 0)
	{
		return Miss(ray);
	}

	return ClosestRay(ray, hitDistance, closestSphere);
}

Renderer::HitPayLoad Renderer::ClosestRay(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayLoad payLoad;
	payLoad.HitDistance = hitDistance;
	payLoad.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payLoad.WorldPosition = origin + ray.Direction * hitDistance;
	payLoad.WorldNormal = glm::normalize(payLoad.WorldPosition);
	payLoad.WorldPosition += closestSphere.Position;
	return payLoad;
}

Renderer::HitPayLoad Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayLoad payLoad;
	payLoad.HitDistance = -1.0f;
	return payLoad;
}

