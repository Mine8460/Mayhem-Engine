#pragma once

#include <Engine/Renderer/Shader.h>
#include <glm/glm.hpp>
#include <glad/glad.h>

namespace Mayhem
{
	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& _path);
		OpenGLShader(const std::string& _name, const std::string& _vertex, const std::string& _fragment);
		~OpenGLShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual const std::string& GetName() const override { return m_Name; }

		void UploadUniformMat3(const std::string& _name, const glm::mat3& _mat);
		void UploadUniformMat4(const std::string& _name, const glm::mat4& _mat);
		void UploadUniformInt(const std::string& _name, int _values);
		void UploadUniformIntArray(const std::string& _name, int* _values, uint32_t _count);
		void UploadUniformFloat(const std::string& _name, float _values);
		void UploadUniformFloat2(const std::string& _name, const glm::vec2& _values);
		void UploadUniformFloat3(const std::string& _name, const glm::vec3& _values);
		void UploadUniformFloat4(const std::string& _name, const glm::vec4& _values);
	private:
		std::string ReadFile(const std::string& _path);
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& _source);
		void Compile(std::unordered_map<GLenum, std::string> _shaderSources);
	private:
		uint32_t m_RendererID = 0;
		std::string m_Name;
	};
}
