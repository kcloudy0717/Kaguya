#pragma once
#include <cstdint>

//----------------------------------------------------------------------------------------------------
class Time;
struct Scene;

//----------------------------------------------------------------------------------------------------
class RenderSystem
{
public:
	struct Statistics
	{
		inline static uint64_t TotalFrameCount = 0;
		inline static uint64_t FrameCount = 0;
		inline static double TimeElapsed = 0.0;
		inline static double FPS = 0.0;
		inline static double FPMS = 0.0;
	};

	struct Settings
	{
		inline static bool VSync;

		static void RestoreDefaults()
		{
			VSync = true;
		}
	};

	RenderSystem(uint32_t Width, uint32_t Height);
	virtual ~RenderSystem();

	void OnInitialize();
	void OnRender(const Time& Time, Scene& Scene);
	void OnResize(uint32_t Width, uint32_t Height);
	void OnDestroy();
	void OnRequestCapture();
protected:
	virtual void Initialize() = 0;
	virtual void Render(const Time& Time, Scene& Scene) = 0;
	virtual void Resize(uint32_t Width, uint32_t Height) = 0;
	virtual void Destroy() = 0;
	virtual void RequestCapture() {}
protected:
	uint32_t m_Width, m_Height;
	float m_AspectRatio;
};