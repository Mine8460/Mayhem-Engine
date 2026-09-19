#include "Enpch.h"
#include "OpenGLShader.h"
#include <fstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Mayhem
{
	static GLenum ShaderTypeFromString(const std::string& _type)
	{
		if (_type == "vertex")
			return GL_VERTEX_SHADER;
		if (_type == "fragment" || _type == "pixel")
			return GL_FRAGMENT_SHADER;

		return GL_NONE;
	}

	OpenGLShader::OpenGLShader(const std::string& _path)
	{
		std::string shaderSource = ReadFile(_path);
		auto shaderSources = PreProcess(shaderSource);
		Compile(shaderSources);
		
		auto lastSlash = _path.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = _path.rfind('.');
		auto count = lastDot == std::string::npos ? _path.size() - lastSlash : lastDot - lastSlash;
		m_Name = _path.substr(lastSlash, count);
	}

	OpenGLShader::OpenGLShader(const std::string& _name, const std::string& _vertex, const std::string& _fragment)
	{
		m_Name = _name;
		std::unordered_map<GLenum, std::string> sources;
		sources[GL_VERTEX_SHADER] = _vertex;
		sources[GL_FRAGMENT_SHADER] = _fragment;

		Compile(sources);
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{
		glUseProgram(0);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& _name, const glm::mat3& _mat)
	{
		GLint location = glGetUniformLocation(m_RendererID, _name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(_mat));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& _name, const glm::mat4& _mat)
	{
		GLint location = glGetUniformLocation(m_RendererID, _name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(_mat));
	}

	void OpenGLShader::UploadUniformInt(const std::string& _name, int _values)
	{
		GLint location = glGetUniformLocation(m_RendererID, _name.c_str());
		glUniform1i(location, _values);
	}

	void OpenGLShader::UploadUniformIntArray(const std::string& _name, int* _values, uint32_t _count)
	{
		GLint location = glGetUniformLocation(m_RendererID, _name.c_str());
		glUniform1iv(location, _count, _values);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& _name, float _values)
	{
		GLint location = glGetUniformLocation(m_RendererID, _name.c_str());
		glUniform1f(location, _values);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& _name, const glm::vec2& _values)
	{
		GLint location = glGetUniformLocation(m_RendererID, _name.c_str());
		glUniform2f(location, (GLfloat)_values.x, (GLfloat)_values.y);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& _name, const glm::vec3& _values)
	{
		GLint location = glGetUniformLocation(m_RendererID, _name.c_str());
		glUniform3f(location, (GLfloat)_values.x, (GLfloat)_values.y, (GLfloat)_values.z);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& _name, const glm::vec4& _values)
	{
		GLint location = glGetUniformLocation(m_RendererID, _name.c_str());
		glUniform4f(location, (GLfloat)_values.x, (GLfloat)_values.y, (GLfloat)_values.z, (GLfloat)_values.w);
	}

	std::string OpenGLShader::ReadFile(const std::string& _path)
	{
		std::string result;
		std::ifstream in(_path, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		}
		else
		{
			MAYHEM_CORE_ERROR("Could not open shader file at path : ");
			MAYHEM_CORE_ERROR(_path.c_str());
		}

		return result;
	}

	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& _source)
	{
		std::unordered_map<GLenum, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLenght = strlen(typeToken);
		size_t pos = _source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = _source.find_first_of("\r\n", pos);
			size_t begin = pos + typeTokenLenght + 1;
			std::string type = _source.substr(begin, eol - begin);

			size_t nextLinePos = _source.find_first_not_of("\r\n", eol);
			pos = _source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] = _source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? _source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void OpenGLShader::Compile(std::unordered_map<GLenum, std::string> _shaderSources)
	{
		m_RendererID = glCreateProgram();
		std::vector<GLenum> glShaderIDs;
		for (auto& kv : _shaderSources)
		{
			GLenum type = kv.first;
			const std::string& source = kv.second;

			GLuint shader = glCreateShader(type);

			const GLchar* sourceCStr = (const GLchar*)source.c_str();
			glShaderSource(shader, 1, &sourceCStr, 0);

			// Compile the vertex FlatColorShader
			glCompileShader(shader);

			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

				// The maxLength includes the NULL character
				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

				// We don't need the FlatColorShader anymore.
				glDeleteShader(shader);

				MAYHEM_CORE_ERROR("Shader compilation failure !");
				MAYHEM_CORE_ERROR(infoLog.data());
				break;
			}
			glAttachShader(m_RendererID, shader);
			glShaderIDs.push_back(shader);
		}

		// Link our program
		glLinkProgram(m_RendererID);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(m_RendererID, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(m_RendererID, maxLength, &maxLength, &infoLog[0]);

			// We don't need the program anymore.
			glDeleteProgram(m_RendererID);
			// Don't leak shaders either.
			for (auto shader : glShaderIDs)
				glDeleteShader(shader);

			MAYHEM_CORE_ERROR("Shaders link failure !");
			MAYHEM_CORE_ERROR(infoLog.data());

			return;
		}

		// Always detach shaders after a successful link.
		for (auto shader : glShaderIDs)
			glDetachShader(m_RendererID, shader);

		glShaderIDs.clear();
	}

}
