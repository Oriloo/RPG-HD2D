#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "UAttackDataComponent.h"
#include "StatusEffectComponent.h"
#include "TurnGameState.generated.h"



/**
 * Contient les données de combat accessibles à tout le monde :
 * ordre des tours, entité active, phase actuelle, etc.
 */
UCLASS()
class RPG_HD2D_API ATurnGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATurnGameState();

	/** Ordre des entités dans le combat (joueur, IA, etc.) */
	UPROPERTY(BlueprintReadOnly, Category = "TurnBased")
	TArray<AActor*> TurnOrder;

	/** Index du personnage actuellement en train de jouer */
	UPROPERTY(BlueprintReadOnly, Category = "TurnBased")
	int32 CurrentTurnIndex;

	UPROPERTY(BlueprintReadOnly, Category = "TurnBased")
	int32 CurrentRound = 0;

	/** Phase actuelle du combat (début, action, fin, victoire, etc.) */
	UPROPERTY(BlueprintReadOnly, Category = "TurnBased")
	FName CurrentPhase;

	UPROPERTY(BlueprintReadOnly , Category = "TurnBased")
	int numberOfActions = 0;

	// 3 last action for player
	UPROPERTY(BlueprintReadOnly, Category = "TurnBased")
	TArray<UUAttackDataComponent*> LastPLayerAction;

	// 2 last action for enemy
	UPROPERTY(BlueprintReadOnly, Category = "TurnBased")
	TArray<UUAttackDataComponent*> LastEnemyAction;

	// Status effect aplicate on the player
	UPROPERTY(BlueprintReadWrite, Category = "TurnBased")
	UStatusEffectComponent* PlayerActiveEffect;

	// Status effect aplicate on the IA
	UPROPERTY(BlueprintReadWrite, Category = "TurnBased")
	UStatusEffectComponent* AiActiveEffect;

	/** Récupère l'entité en train de jouer */
	UFUNCTION(BlueprintCallable, Category = "TurnBased")
	AActor* GetCurrentActor() const;

	/** Ajoute un participant dans l'ordre des tours */
	UFUNCTION(BlueprintCallable, Category = "TurnBased")
	void RegisterParticipant(AActor* NewParticipant);

protected:
	virtual void BeginPlay() override;
};
