#include "GameplayGameMode.h"
#include "GameplayPlayerController.h"

AGameplayGameMode::AGameplayGameMode()
{
	// Set default player controller class to our gameplay controller
	PlayerControllerClass = AGameplayPlayerController::StaticClass();
}