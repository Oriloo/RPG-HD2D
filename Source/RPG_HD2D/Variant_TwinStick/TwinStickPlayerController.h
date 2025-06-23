// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TwinStickPlayerController.generated.h"

class UInputMappingContext;
class ATwinStickCharacter;

/**
 *  Simple Player Controller for a Twin Stick Shooter game
 *  Manages input mapping contexts
 *  Respawns the pawn if it is destroyed
 */
UCLASS(abstract)
class ATwinStickPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input mapping context for this player */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* InputMappingContext;

	/** Character class to respawn when the possessed pawn is destroyed */
	UPROPERTY(EditAnywhere, Category="Respawn")
	TSubclassOf<ATwinStickCharacter> CharacterClass;

protected:

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	/** Pawn initialization */
	virtual void OnPossess(APawn* InPawn) override;

	/** Called if the possessed pawn is destroyed */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);
};
