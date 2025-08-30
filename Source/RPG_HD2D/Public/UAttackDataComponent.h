// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UActionComponent.h"
#include "UAttackDataComponent.generated.h"




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RPG_HD2D_API UUAttackDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUAttackDataComponent();
	// Constructor with parameters for attack data
	UUAttackDataComponent(FString AttackName, int32 AttackCost, TArray<UActionComponent*> AttackActions);

	// Name of the attack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Name")
	FString AttackName;
	//cost of the attack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Cost")
	int32 AttackCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Actions List")
	TArray<UActionComponent*> AttackActions;


    UFUNCTION(BlueprintCallable, Category = "AddAction")
    void addAction(const FString& AttackTyp, int32 baseDamage, int32 duration);

	UFUNCTION(BlueprintCallable, Category = "Attack Actions")
	TArray<UActionComponent*> GetAttackActions() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


		
};
