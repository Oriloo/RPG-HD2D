// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ElmAtkComponant.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RPG_HD2D_API UElmAtkComponant : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UElmAtkComponant();

	// The elemental attack power of the character
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ElementalAtk")
	int elementalAtk;



protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
