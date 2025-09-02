#include "TurnGameState.h"

ATurnGameState::ATurnGameState()
{
	CurrentTurnIndex = 0;
	CurrentPhase = "Init";
}

void ATurnGameState::BeginPlay()
{
	Super::BeginPlay();
	// Tu peux ajouter ici une logique de debug ou d'initialisation
}

AActor* ATurnGameState::GetCurrentActor() const
{
	int32 NextIndex = CurrentTurnIndex;
	if (TurnOrder.IsValidIndex(NextIndex))
	{
		return TurnOrder[NextIndex];
	}
	return nullptr;
}

void ATurnGameState::RegisterParticipant(AActor* NewParticipant)
{
	if (NewParticipant && !TurnOrder.Contains(NewParticipant))
	{
		TurnOrder.Add(NewParticipant);
	}
}
