#include "MainMenuGameMode.h"
#include "MainMenuPlayerController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	// Set default player controller class to our main menu controller
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
}