#include "GameplayPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"

AGameplayPlayerController::AGameplayPlayerController()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsInMainMenu = true;
}

void AGameplayPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Start in main menu mode
	ShowMainMenu();
}

void AGameplayPlayerController::ShowMainMenu()
{
	bIsInMainMenu = true;
	
	// Set input mode to UI only and show cursor
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	// Create and show main menu widget
	if (MainMenuWidgetClass && !MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();
		}
	}
	else if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AGameplayPlayerController::HideMainMenu()
{
	bIsInMainMenu = false;
	
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AGameplayPlayerController::StartNewGame()
{
	// Hide the main menu
	HideMainMenu();
	
	// Remove the widget completely from the viewport
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromViewport();
		MainMenuWidget = nullptr;
	}
	
	// Change input mode to game only
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	
	// The game is now ready to play in the same level
}

void AGameplayPlayerController::ReturnToMainMenu()
{
	// Show the main menu again
	ShowMainMenu();
}