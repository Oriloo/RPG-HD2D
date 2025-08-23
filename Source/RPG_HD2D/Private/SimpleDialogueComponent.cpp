#include "SimpleDialogueComponent.h"

USimpleDialogueComponent::USimpleDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USimpleDialogueComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USimpleDialogueComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USimpleDialogueComponent::StartDialogue()
{
	// Implementation for starting dialogue
}

void USimpleDialogueComponent::EndDialogue()
{
	// Implementation for ending dialogue
}