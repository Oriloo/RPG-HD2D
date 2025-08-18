#include "GameplayPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AGameplayPlayerController::AGameplayPlayerController()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsInMainMenu = true;
	bIsGamePaused = false;
}

void AGameplayPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Setup Enhanced Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	// Start in main menu mode
	ShowMainMenu();
}

void AGameplayPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Cast to Enhanced Input Component
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Bind pause action
		if (PauseAction)
		{
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this, &AGameplayPlayerController::HandlePauseInput);
		}
	}
}

void AGameplayPlayerController::HandlePauseInput(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		TogglePause();
	}
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
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}
	
	// Change input mode to game only
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	
	// The game is now ready to play in the same level
}

void AGameplayPlayerController::ShowPauseMenu()
{
	if (!bIsInMainMenu && PauseMenuWidgetClass)
	{
		if (!PauseMenuWidget)
		{
			PauseMenuWidget = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
		}
		
		if (PauseMenuWidget)
		{
			PauseMenuWidget->AddToViewport(100); // High Z-order to appear on top
			
			// Set input mode to UI only
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
			SetInputMode(InputMode);
			bShowMouseCursor = true;
			
			// Pause the game
			SetPause(true);
			bIsGamePaused = true;
		}
	}
}

void AGameplayPlayerController::HidePauseMenu()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		
		// Return to game input mode
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		
		// Unpause the game
		SetPause(false);
		bIsGamePaused = false;
	}
}

void AGameplayPlayerController::TogglePause()
{
	if (bIsInMainMenu)
		return;
		
	if (bIsGamePaused)
	{
		HidePauseMenu();
	}
	else
	{
		ShowPauseMenu();
	}
}

void AGameplayPlayerController::ResumeGame()
{
	HidePauseMenu();
}

void AGameplayPlayerController::RestartCurrentLevel()
{
	HidePauseMenu();
	
	// Restart the current level
	UWorld* World = GetWorld();
	if (World)
	{
		FString CurrentLevelName = World->GetMapName();
		CurrentLevelName = CurrentLevelName.Replace(TEXT("UEDPIE_0_"), TEXT("")); // Remove PIE prefix if in editor
		GetWorld()->ServerTravel(CurrentLevelName);
	}
}

void AGameplayPlayerController::ReturnToMainMenu()
{
	HidePauseMenu();
	
	// Show the main menu again
	ShowMainMenu();
}