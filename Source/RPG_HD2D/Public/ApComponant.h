// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ApComponant.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RPG_HD2D_API UApComponant : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UApComponant();

	//Varible fort the nomber of actions Point 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Points")
	int32 ActionPoints;
	//Varible for the maximum number of action points
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MaxAction Points")
	int32 MaxActionPoints;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
