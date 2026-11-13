#pragma once

#include <Engine.h>
#include <Engine/Scene/Scene.h>

#include <vector>
#include <string>

namespace Mayhem
{
	// Annuler / Rétablir par instantanés de la scène.
	// Chaque étape garde une copie complète de la scène (Scene::Copy) + sa version texte
	// (pour savoir si une interaction a vraiment modifié quelque chose).
	class UndoHistory
	{
	public:
		// À appeler une fois par frame, APRÈS tout le code ImGui de l'éditeur.
		// _interacting = l'utilisateur est en train de cliquer / glisser / taper quelque chose.
		void Track(const Ref<Scene>& _scene, bool _interacting);

		// Prend l'instantané "avant" tout de suite (ex : juste avant que le gizmo ne bouge l'objet)
		void BeginInteraction(const Ref<Scene>& _scene);

		// Enregistre une étape immédiatement (actions au clavier : Ctrl+D...)
		void Record(const Ref<Scene>& _scene);

		// Renvoient la scène à afficher, ou nullptr s'il n'y a rien à annuler / rétablir
		Ref<Scene> Undo(const Ref<Scene>& _current);
		Ref<Scene> Redo(const Ref<Scene>& _current);

		bool CanUndo() const { return !m_UndoStack.empty(); }
		bool CanRedo() const { return !m_RedoStack.empty(); }

		// Change à chaque modification, annulation ou rétablissement :
		// permet de ne revérifier "scène modifiée ?" que quand c'est nécessaire
		uint64_t GetVersion() const { return m_Version; }

		void CancelInteraction();
		void Clear();

	private:
		struct Snapshot
		{
			Ref<Scene> SceneCopy;
			std::string Text;
		};

		static Snapshot Take(const Ref<Scene>& _scene);
		void PushUndo(Snapshot&& _snapshot);

	private:
		static constexpr size_t MaxSteps = 100;

		std::vector<Snapshot> m_UndoStack;
		std::vector<Snapshot> m_RedoStack;

		Snapshot m_Pending;
		bool m_HasPending = false;
		bool m_WasInteracting = false;

		uint64_t m_Version = 0;
	};
}
