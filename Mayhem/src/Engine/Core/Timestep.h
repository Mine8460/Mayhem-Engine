#pragma once

namespace Mayhem
{
	class Timestep
	{
	public:
		Timestep(float _time = 0.0f) : m_Time(_time)
		{

		}

		float GetSeconds() const { return m_Time; }
		float GetMilliseconds() const { return m_Time * 1000.f; }
	private:
		float m_Time;
	};
}
