#include "ATurnGameMode.h"
#include "TurnGameState.h"
#include "Kismet/GameplayStatics.h"
#include "UTurnCombatComponent.h"
#include "Components/ActorComponent.h"  
#include "EngineUtils.h"  
#include "UStatsComponent.h"
#include "StatusEffectComponent.h"
#include <CombatJsonExporter.h>

ATurnGameMode::ATurnGameMode()
{
	// Utilise notre GameState personnaliséS
	GameStateClass = ATurnGameState::StaticClass();
}

void ATurnGameMode::BeginPlay()
{
	Super::BeginPlay();


}

ATurnGameState* ATurnGameMode::GetTurnGameState() const
{
    if (!GameState) return nullptr;
    // Assurez-vous que GameState est du type ATurnGameState
    if (!GameState->IsA(ATurnGameState::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("GameState is not of type ATurnGameState"));
        return nullptr;
    }
	// Cast et retourner le GameState
	return Cast<ATurnGameState>(GameState);
}



void ATurnGameMode::StartCombat()
{
    ATurnGameState* TurnState = GetTurnGameState();
    if (!TurnState) return;

    Participants.Empty();
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor->FindComponentByClass<UUTurnCombatComponent>())
        {
            Participants.Add(Actor);
        }
    }

    struct FParticipantInfo
    {
        AActor* Actor;
        float Speed;
    };

    TArray<FParticipantInfo> SortedParticipants;
    for (AActor* Actor : Participants)
    {
        UUStatsComponent* StatsComp = Actor->FindComponentByClass<UUStatsComponent>();
        float Speed = StatsComp ? StatsComp->Speed : 0.0f;
        SortedParticipants.Add({ Actor, Speed });
    }

    // Trie les participants par vitesse décroissante (plus rapide en premier)
    SortedParticipants.Sort([](const FParticipantInfo& A, const FParticipantInfo& B)
    {
        return A.Speed > B.Speed;
    });

    TurnState->TurnOrder.Empty();
    for (const FParticipantInfo& Info : SortedParticipants)
    {
        TurnState->RegisterParticipant(Info.Actor);
        UE_LOG(LogTemp, Log, TEXT("Participant: %s, Speed: %f"), *Info.Actor->GetName(), Info.Speed);
    }

    StartNextTurn();
}

void ATurnGameMode::StartNextTurn()
{
    ATurnGameState* TurnState = GetTurnGameState();
    // Vérifie si le GameState est valide   
    if (!TurnState || TurnState->TurnOrder.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TurnState is not valid or has no participants"));
        return;
    }

    // Récupère l’acteur courant à l’index actuel
    AActor* CurrentActor = TurnState->TurnOrder[TurnState->CurrentTurnIndex];
    if (!CurrentActor) return;

    //get le buff et debudf du personnage courant si c est une IA
    UStatusEffectComponent* StatusEffect = CurrentActor->FindComponentByClass<UStatusEffectComponent>();
    if (!CurrentActor->GetName().Equals(TEXT("BP_Main_Character")))
    {
        // Pour IA : stocker l'effet actif dans une variable locale ou traiter ici
        if (StatusEffect)
        {  
            TurnState->AiActiveEffect = StatusEffect;
			/*TSharedPtr<FJsonObject> jsonIA = UCombatJsonExporter::MakeCombatStateToJson(TurnState);*/

        }
    }
    else
    {
        if (StatusEffect)
        {
            TurnState->AiActiveEffect = StatusEffect;

        }
    }

    if (UUTurnCombatComponent* CombatComp = CurrentActor->FindComponentByClass<UUTurnCombatComponent>())
    {
        CombatComp->StartTurn();
        UE_LOG(LogTemp, Log, TEXT("Starting turn for actor: %s"), *CurrentActor->GetName());
    }
    if (TurnState->CurrentTurnIndex == 0)
    {
        TurnState->CurrentRound++;
        //UE_LOG(LogTemp, Log, TEXT("Starting round: %d"), TurnState->CurrentRound);
    }

    // Prépare l’index pour le prochain tour
    TurnState->CurrentTurnIndex = (TurnState->CurrentTurnIndex + 1) % TurnState->TurnOrder.Num();
}


void ATurnGameMode::OnTurnEnded()
{
	StartNextTurn();
}


void ATurnGameMode::testFunction()
{
    // Just a placeholder for testing purposes
    UE_LOG(LogTemp, Warning, TEXT("Test function called in ATurnGameMode"));
}
