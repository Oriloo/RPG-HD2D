// Fill out your copyright notice in the Description page of Project Settings.


#include "UStatsComponent.h"

// Sets default values for this component's properties
UUStatsComponent::UUStatsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	// ...
}


// Called when the game starts
void UUStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UUStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UUStatsComponent::RecieveDamage(int32 damage) 
{
	HealthPoints -= damage;
	//log to see if the damage is applied correctly
	/*UE_LOG(LogTemp, Warning, TEXT("Damage received: %d, Current Health: %d"), damage, HealthPoints);*/

	if (HealthPoints < 0)
	{
		HealthPoints = 0;
	}
	else if (HealthPoints > MaxHealthPoints)
	{
		HealthPoints = MaxHealthPoints;
	}

	if (HealthPoints == 0)
	{
		// Handle player death logic here
		// For example, you might want to trigger a death animation or respawn the player
	}

}

void UUStatsComponent::Heal(int32 healAmount)
{
	HealthPoints += healAmount;
	if (HealthPoints > MaxHealthPoints)
	{
		HealthPoints = MaxHealthPoints;
	}
}

