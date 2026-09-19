#pragma once

#include <Engine.h>
#include <chrono>

class EditorLayer : public Mayhem::Layer
{
public:
	EditorLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Mayhem::Timestep _timestep) override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(Mayhem::Event& _e) override;
private:
	Mayhem::Ref<Mayhem::OrthographicCameraController> m_CameraController;

	Mayhem::Ref<Mayhem::Texture2D> m_Texture;
	Mayhem::Ref<Mayhem::Texture2D> m_AlphaTexture;
	Mayhem::Ref<Mayhem::Texture2D> m_SpriteSheet;
	std::vector<Mayhem::Ref<Mayhem::SubTexture2D>> m_Sprites;

	int spriteToUse = 1;

	glm::vec4 m_FlatColor = { 0.2f, 0.3f, 1.0f, 1.0f };

	struct ProfileResult
	{
		const char* Name;
		float Time;
	};

	std::vector<ProfileResult> m_ProfileResults;
};