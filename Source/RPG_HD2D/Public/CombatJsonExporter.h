#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dom/JsonObject.h"
#include "Templates/SharedPointer.h"
#include "TurnGameState.h"
#include "CombatJsonExporter.generated.h"


UCLASS(Blueprintable)
class RPG_HD2D_API UCombatJsonExporter : public UObject
{
    GENERATED_BODY()

public:
    /** Construit un JSON complet de l'etat du combat */
    static TSharedPtr<FJsonObject> MakeCombatStateToJson(const ATurnGameState* GameState);

private:
    // Fonctions utilitaires privees
    static TSharedPtr<FJsonObject> BuildPLayerLastActionsToJson(const ATurnGameState* GameState);
    static TSharedPtr<FJsonObject> AIActiveEffectToJson(const ATurnGameState* GameState);
    static TSharedPtr<FJsonObject> PlayerActiveEffectToJson(const ATurnGameState* GameState);
    static TSharedPtr<FJsonObject> CombatantStatsToJson(const ATurnGameState* GameState);
    static TSharedPtr<FJsonObject> CombatStateToJson(const ATurnGameState* GameState);
    static TSharedPtr<FJsonObject> BuildDisponibleActionToJson(const ATurnGameState* GameState);
    static TSharedPtr<FJsonObject> BuildAILastActionsToJson(const ATurnGameState* GameState);
};
