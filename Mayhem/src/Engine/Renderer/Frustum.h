#pragma once

#include <glm/glm.hpp>
#include <cmath>

namespace Mayhem
{
	// Boîte englobante alignée sur les axes
	struct AABB
	{
		glm::vec3 Min = glm::vec3(0.0f);
		glm::vec3 Max = glm::vec3(0.0f);

		glm::vec3 GetCenter() const { return (Min + Max) * 0.5f; }
		glm::vec3 GetExtents() const { return (Max - Min) * 0.5f; }
	};

	// Boîte en espace monde, sous forme centre + demi-tailles (pratique pour le test de frustum)
	struct WorldBounds
	{
		glm::vec3 Center = glm::vec3(0.0f);
		glm::vec3 Extents = glm::vec3(0.0f);

		// Boîte locale -> boîte monde englobante (méthode d'Arvo : exacte et sans transformer les 8 coins)
		static WorldBounds FromLocal(const AABB& _local, const glm::mat4& _transform)
		{
			WorldBounds result;
			result.Center = glm::vec3(_transform * glm::vec4(_local.GetCenter(), 1.0f));

			const glm::vec3 extents = _local.GetExtents();
			for (int i = 0; i < 3; i++)
			{
				result.Extents[i] =
					std::abs(_transform[0][i]) * extents.x +
					std::abs(_transform[1][i]) * extents.y +
					std::abs(_transform[2][i]) * extents.z;
			}
			return result;
		}
	};

	// Volume de vue (caméra ou cascade d'ombre) : 6 plans extraits d'une matrice view-projection
	class Frustum
	{
	public:
		Frustum() = default;

		explicit Frustum(const glm::mat4& _viewProjection)
		{
			// Méthode de Gribb-Hartmann (clip space OpenGL : x, y, z dans [-1, 1])
			auto row = [&](int _i)
			{
				return glm::vec4(_viewProjection[0][_i], _viewProjection[1][_i], _viewProjection[2][_i], _viewProjection[3][_i]);
			};

			const glm::vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);
			m_Planes[0] = r3 + r0; // gauche
			m_Planes[1] = r3 - r0; // droite
			m_Planes[2] = r3 + r1; // bas
			m_Planes[3] = r3 - r1; // haut
			m_Planes[4] = r3 + r2; // proche
			m_Planes[5] = r3 - r2; // lointain
		}

		// true si la boîte est (au moins en partie) dans le volume
		bool Intersects(const WorldBounds& _bounds) const
		{
			for (const glm::vec4& plane : m_Planes)
			{
				const glm::vec3 normal = glm::vec3(plane);
				// "Rayon" de la boîte projeté sur la normale du plan
				const float radius = glm::dot(_bounds.Extents, glm::abs(normal));
				const float distance = glm::dot(normal, _bounds.Center) + plane.w;

				if (distance + radius < 0.0f) // entièrement du mauvais côté de ce plan
					return false;
			}
			return true;
		}

	private:
		glm::vec4 m_Planes[6];
	};
}
