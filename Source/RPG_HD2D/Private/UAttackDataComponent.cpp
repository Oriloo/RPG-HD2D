// Fill out your copyright notice in the Description page of Project Settings.


#include "UAttackDataComponent.h"
#include "StatusEffectComponent.h"



// Sets default values for this component's properties
UUAttackDataComponent::UUAttackDataComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Constructor with parameters for attack data
UUAttackDataComponent::UUAttackDataComponent(FString AttackName, int32 AttackCost, TArray<UActionComponent*> AttackActions)
	: AttackName(AttackName), AttackCost(AttackCost), AttackActions(AttackActions)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}


// Called when the game starts
void UUAttackDataComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UUAttackDataComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UUAttackDataComponent::addAction(const FString& AttackTyp, int baseDamage, int duration)
{  
    UActionComponent* action = NewObject<UActionComponent>(this, UActionComponent::StaticClass());
    if (action)
    {
        action->setType(AttackTyp);
		action->setBaseDamage(baseDamage);
		action->setDuration(duration);
        AttackActions.Add(action); 

    }

	// Log to see if the action is added correctly
	UE_LOG(LogTemp, Warning, TEXT("Action added: Type=%s, BaseDamage=%d, Duration=%d"), *AttackTyp, baseDamage, duration);
	GetAttackActions();
}


TArray<UActionComponent*> UUAttackDataComponent::GetAttackActions() const
{
	// Log to see the number of actions in the array
	UE_LOG(LogTemp, Warning, TEXT("Number of actions in AttackActions: %d"), AttackActions.Num());
	// Return the array of attack actions
	return AttackActions;
}