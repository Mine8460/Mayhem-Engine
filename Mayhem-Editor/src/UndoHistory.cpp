#include "UndoHistory.h"

#include <Engine/Scene/SceneSerializer.h>

namespace Mayhem
{
	UndoHistory::Snapshot UndoHistory::Take(const Ref<Scene>& _scene)
	{
		SceneSerializer serializer(_scene);
		return { Scene::Copy(_scene), serializer.SerializeToString() };
	}

	void UndoHistory::PushUndo(Snapshot&& _snapshot)
	{
		m_UndoStack.push_back(std::move(_snapshot));
		if (m_UndoStack.size() > MaxSteps)
			m_UndoStack.erase(m_UndoStack.begin());

		// Une nouvelle modification rend l'historique "rétablir" obsolète
		m_RedoStack.clear();
		m_Version++;
	}

	void UndoHistory::BeginInteraction(const Ref<Scene>& _scene)
	{
		if (m_HasPending)
			return;

		m_Pending = Take(_scene);
		m_HasPending = true;
	}

	void UndoHistory::Track(const Ref<Scene>& _scene, bool _interacting)
	{
		// Début d'une interaction : on mémorise l'état "avant"
		if (_interacting && !m_WasInteracting)
			BeginInteraction(_scene);

		// Fin de l'interaction : on ne garde l'étape que si la scène a réellement changé
		// (un simple clic sur un nœud de la hiérarchie ou un déplacement de fenêtre ne compte pas)
		if (!_interacting && m_WasInteracting && m_HasPending)
		{
			SceneSerializer serializer(_scene);
			if (serializer.SerializeToString() != m_Pending.Text)
				PushUndo(std::move(m_Pending));

			CancelInteraction();
		}

		m_WasInteracting = _interacting;
	}

	void UndoHistory::Record(const Ref<Scene>& _scene)
	{
		PushUndo(Take(_scene));
	}

	Ref<Scene> UndoHistory::Undo(const Ref<Scene>& _current)
	{
		if (m_UndoStack.empty())
			return nullptr;

		m_RedoStack.push_back(Take(_current));

		Ref<Scene> previous = m_UndoStack.back().SceneCopy;
		m_UndoStack.pop_back();
		m_Version++;
		return previous;
	}

	Ref<Scene> UndoHistory::Redo(const Ref<Scene>& _current)
	{
		if (m_RedoStack.empty())
			return nullptr;

		m_UndoStack.push_back(Take(_current));

		Ref<Scene> next = m_RedoStack.back().SceneCopy;
		m_RedoStack.pop_back();
		m_Version++;
		return next;
	}

	void UndoHistory::CancelInteraction()
	{
		m_Pending = Snapshot();
		m_HasPending = false;
	}

	void UndoHistory::Clear()
	{
		m_UndoStack.clear();
		m_RedoStack.clear();
		CancelInteraction();
		m_WasInteracting = false;
		m_Version++;
	}
}
