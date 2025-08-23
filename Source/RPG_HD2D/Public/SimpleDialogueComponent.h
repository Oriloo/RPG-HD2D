#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimpleDialogueComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPG_HD2D_API USimpleDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USimpleDialogueComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FString DialogueText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bCanInteract = true;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndDialogue();
};