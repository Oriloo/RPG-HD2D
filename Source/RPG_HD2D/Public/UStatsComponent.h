// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UStatsComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RPG_HD2D_API UUStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUStatsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	//point de vie du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 HealthPoints;

	//point de vie max du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MaxHealthPoints;

	//val vitesse du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Speed;

	//val PA du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 ActionPoints;

	//val PA max du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MaxActionPoints;

	//val attaque physique du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 PhysicalAttack;

	//val attaque elmentaire du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 ElementalAttack;

	//val attaque spirituelle du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 SpiritualAttack;

	//val defense physique du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 PhysicalDefense;

	//val defense elmentaire du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 ElementalDefense;

	//val defense spirituelle du joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 SpiritualDefense;

	
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecieveDamage(int32 damage);

	
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void Heal(int32 healAmount);

	//UFUNCTION(BlueprintCallable, Category = "Stats")
	void ResetStats();

		
};
