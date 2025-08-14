#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "GameplayPlayerController.generated.h"

UCLASS()
class RPG_HD2D_API AGameplayPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGameplayPlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UUserWidget* MainMenuWidget;

	UPROPERTY(BlueprintReadWrite, Category = "Game State")
	bool bIsInMainMenu;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartNewGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void ReturnToMainMenu();
};