#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ATurnGameMode.generated.h"

class ATurnGameState;
class UUTurnCombatComponent;

UCLASS()
class RPG_HD2D_API ATurnGameMode : public AGameModeBase
{
	GENERATED_BODY()


public:
	ATurnGameMode();

	/** Lance le combat en initialisant l'ordre des tours */
	UFUNCTION(BlueprintCallable)
	void StartCombat();

	/** Appelle le tour du prochain participant */
	UFUNCTION(BlueprintCallable)
	void StartNextTurn();

	UFUNCTION(BlueprintCallable)
	void OnTurnEnded();

	//test function
	UFUNCTION(BlueprintCallable, Category = "TurnBased")
	void testFunction();
protected:
	virtual void BeginPlay() override;

	ATurnGameState* GetTurnGameState() const;

private:

	UPROPERTY()
	ATurnGameState* TurnGameState;
	UPROPERTY()
	TArray<AActor*> Participants;
	UPROPERTY()
	int32 CurrentTurnIndex = 0;
	UPROPERTY()
	TArray<AActor*> TurnOrder;

};
