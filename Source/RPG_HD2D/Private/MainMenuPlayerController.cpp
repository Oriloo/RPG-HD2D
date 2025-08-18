#include "MainMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

AMainMenuPlayerController::AMainMenuPlayerController()
{
	PrimaryActorTick.bCanEverTick = false;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Set input mode to UI only
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	
	ShowMainMenu();
}

void AMainMenuPlayerController::ShowMainMenu()
{
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

void AMainMenuPlayerController::HideMainMenu()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AMainMenuPlayerController::StartNewGame()
{
	// Hide the main menu before transitioning
	HideMainMenu();
	
	// Remove the widget completely from the viewport to avoid persistence
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromViewport();
		MainMenuWidget = nullptr;
	}
	
	// Change input mode back to game only
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	
	// Load the game level
	UGameplayStatics::OpenLevel(this, FName("Basic"));
}