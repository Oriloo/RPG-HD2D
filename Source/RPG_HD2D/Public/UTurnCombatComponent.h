#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UAttackDataComponent.h"
#include "UTurnCombatComponent.generated.h"


class ATurnGameMode;


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RPG_HD2D_API UUTurnCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUTurnCombatComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsMyTurn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsPlayerTurn = false;

	UFUNCTION(BlueprintCallable)
	void StartTurn();

	UFUNCTION(BlueprintCallable)
	void EndTurn();


	UPROPERTY(BlueprintReadOnly)
	ATurnGameMode* TurnGameMode;

	UPROPERTY()
	AActor* OwningCharacter;


	UFUNCTION(BlueprintCallable, Category = "Combat")
	void testFunction();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void excuteAction(AActor* attackingActor, AActor* attackedActor, UUAttackDataComponent* attack);


private:

	void addActionToHistory(AActor* actor, UUAttackDataComponent* attack);
	bool IsPlayerActor(AActor* Actor);

};
