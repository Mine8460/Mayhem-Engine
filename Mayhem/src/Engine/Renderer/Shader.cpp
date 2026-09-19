#include "Enpch.h"
#include "Shader.h"

#include <memory>
#include <Engine/Renderer/RendererAPI.h>
#include <Platform/OpenGL/OpenGLShader.h>

namespace Mayhem
{
	Ref<Shader> Shader::Create(const std::string& _path)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::OpenGL: return std::make_shared<OpenGLShader>(_path);
		case RendererAPI::API::Vulkan: return nullptr;
		case RendererAPI::API::Direct3D: return nullptr;
		}
		return nullptr;

		return nullptr;
	}
	Ref<Shader> Shader::Create(const std::string& _name, const std::string& _vertex, const std::string& _fragment)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: return nullptr;
		case RendererAPI::API::OpenGL: return std::make_shared<OpenGLShader>(_name, _vertex, _fragment);
		case RendererAPI::API::Vulkan: return nullptr;
		case RendererAPI::API::Direct3D: return nullptr;
		}
		return nullptr;
	}
	void Shader::SetUniformMat4(const std::string& _name, const glm::mat4& _value)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: break;
		case RendererAPI::API::OpenGL: dynamic_cast<OpenGLShader*>(this)->UploadUniformMat4(_name, _value); break;
		case RendererAPI::API::Vulkan: break;
		case RendererAPI::API::Direct3D: break;
		}
	}
	void Shader::SetUniformMat3(const std::string& _name, const glm::mat3& _value)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: break;
		case RendererAPI::API::OpenGL: dynamic_cast<OpenGLShader*>(this)->UploadUniformMat3(_name, _value); break;
		case RendererAPI::API::Vulkan: break;
		case RendererAPI::API::Direct3D: break;
		}
	}
	void Shader::SetUniformFloat4(const std::string& _name, const glm::vec4& _value)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: break;
		case RendererAPI::API::OpenGL: dynamic_cast<OpenGLShader*>(this)->UploadUniformFloat4(_name, _value); break;
		case RendererAPI::API::Vulkan: break;
		case RendererAPI::API::Direct3D: break;
		}
	}
	void Shader::SetUniformFloat3(const std::string& _name, const glm::vec3& _value)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: break;
		case RendererAPI::API::OpenGL: dynamic_cast<OpenGLShader*>(this)->UploadUniformFloat3(_name, _value); break;
		case RendererAPI::API::Vulkan: break;
		case RendererAPI::API::Direct3D: break;
		}
	}
	void Shader::SetUniformFloat2(const std::string& _name, const glm::vec2& _value)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: break;
		case RendererAPI::API::OpenGL: dynamic_cast<OpenGLShader*>(this)->UploadUniformFloat2(_name, _value); break;
		case RendererAPI::API::Vulkan: break;
		case RendererAPI::API::Direct3D: break;
		}
	}
	void Shader::SetUniformFloat(const std::string& _name, float _value)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: break;
		case RendererAPI::API::OpenGL: dynamic_cast<OpenGLShader*>(this)->UploadUniformFloat(_name, _value); break;
		case RendererAPI::API::Vulkan: break;
		case RendererAPI::API::Direct3D: break;
		}
	}
	void Shader::SetUniformIntArray(const std::string& _name, int* _value, uint32_t _count)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: break;
		case RendererAPI::API::OpenGL: dynamic_cast<OpenGLShader*>(this)->UploadUniformIntArray(_name, _value, _count); break;
		case RendererAPI::API::Vulkan: break;
		case RendererAPI::API::Direct3D: break;
		}
	}
	void Shader::SetUniformInt(const std::string& _name, int _value)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None: break;
		case RendererAPI::API::OpenGL: dynamic_cast<OpenGLShader*>(this)->UploadUniformInt(_name, _value); break;
		case RendererAPI::API::Vulkan: break;
		case RendererAPI::API::Direct3D: break;
		}
	}
	void ShaderLibrary::Add(const Ref<Shader>& _s)
	{
		m_Shaders[_s->GetName()] = _s;
	}
	void ShaderLibrary::Add(const std::string& _name, const Ref<Shader>& _s)
	{
		m_Shaders[_name] = _s;
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& _filepath)
	{
		auto shader = Shader::Create(_filepath);
		Add(shader);
		return shader;
	}
	Ref<Shader> ShaderLibrary::Load(const std::string& _name, const std::string& _filepath)
	{
		auto shader = Shader::Create(_filepath);
		Add(_name, shader);
		return shader;
	}
	Ref<Shader> ShaderLibrary::Load(const std::string& _name, const std::string& _vertex, const std::string& _fragment)
	{
		auto shader = Shader::Create(_name, _vertex, _fragment);
		Add(_name, shader);
		return shader;
	}
	Ref<Shader> ShaderLibrary::Get(const std::string& _name)
	{
		return m_Shaders[_name];
	}
	bool ShaderLibrary::Exists(const std::string& _name)
	{
		return m_Shaders.find(_name) != m_Shaders.end();
	}
}
