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
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UUserWidget* MainMenuWidget;

	UPROPERTY(BlueprintReadWrite, Category = "Game State")
	bool bIsInMainMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UUserWidget* PauseMenuWidget;

	UPROPERTY(BlueprintReadWrite, Category = "Game State")
	bool bIsGamePaused;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HidePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void TogglePause();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void ResumeGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void RestartCurrentLevel();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartNewGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void ReturnToMainMenu();
};