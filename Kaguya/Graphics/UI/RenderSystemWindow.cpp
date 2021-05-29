#include "pch.h"
#include "RenderSystemWindow.h"

#include <Core/RenderSystem.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/PathIntegrator.h>

void RenderSystemWindow::RenderGui()
{
	ImGui::Begin("Render System");

	UIWindow::Update();

	const auto& AdapterDesc = RenderDevice::Instance().GetAdapterDesc();
	auto localVideoMemoryInfo = RenderDevice::Instance().QueryLocalVideoMemoryInfo();
	auto usageInMiB = ToMiB(localVideoMemoryInfo.CurrentUsage);
	ImGui::Text("GPU: %ls", AdapterDesc.Description);
	ImGui::Text("VRAM Usage: %d Mib", usageInMiB);

	ImGui::Text("");
	ImGui::Text("Total Frame Count: %d", RenderSystem::Statistics::TotalFrameCount);
	ImGui::Text("FPS: %f", RenderSystem::Statistics::FPS);
	ImGui::Text("FPMS: %f", RenderSystem::Statistics::FPMS);

	ImGui::Text("");

	if (ImGui::Button("Request Capture"))
	{
		m_pRenderSystem->OnRequestCapture();
	}

	if (ImGui::TreeNode("Settings"))
	{
		if (ImGui::Button("Restore Defaults"))
		{
			RenderSystem::Settings::RestoreDefaults();
		}
		ImGui::Checkbox("V-Sync", &RenderSystem::Settings::VSync);

		PathIntegrator::Settings::RenderGui();

		ImGui::TreePop();
	}
	ImGui::End();
}
