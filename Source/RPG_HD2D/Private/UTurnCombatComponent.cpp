// Fill out your copyright notice in the Description page of Project Settings.


#include "UTurnCombatComponent.h"
#include "ATurnGameMode.h"
#include "UStatsComponent.h"
#include "TurnGameState.h"
#include <StatusEffectComponent.h>




// Sets default values for this component's properties
UUTurnCombatComponent::UUTurnCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UUTurnCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = GetOwner();

	if (OwningCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("Composant attaché à : %s"), *OwningCharacter->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Le propriétaire n'est pas un APaperCharacter !"));
	}

	TurnGameMode = Cast<ATurnGameMode>(GetWorld()->GetAuthGameMode());
	if (TurnGameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMode trouve : %s"), *TurnGameMode->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode non trouve ou pas de type ATurnGameMode"));
	}


	UE_LOG(LogTemp, Warning, TEXT("lunching UturnCombatComponat"));


}


// Called every frame
void UUTurnCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


}


void UUTurnCombatComponent::StartTurn()
{
	bIsMyTurn = true;
	OwningCharacter = GetOwner();
	//UE_LOG(LogTemp, Warning, TEXT("Début du tour pour l'acteur : %s"), *OwningCharacter->GetName());
	if (OwningCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("C'est mon tour ! Actor: %s"), *OwningCharacter->GetName());

		TArray<UStatusEffectComponent*> StatusEffects;
		TArray<UActorComponent*> Components = OwningCharacter->GetComponents().Array();
		for (UActorComponent* Comp : Components)
		{
			if (Comp && Comp->IsA(UStatusEffectComponent::StaticClass()))
			{
				StatusEffects.Add(Cast<UStatusEffectComponent>(Comp));
			}
		}
		for (UActorComponent* Comp : StatusEffects)
		{
			UStatusEffectComponent* StatusEffect = Cast<UStatusEffectComponent>(Comp);
			if (StatusEffect)
			{
				StatusEffect->ApplyStatusEffects();
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("C'est mon tour ! Actor: inconnu"));
	}

	//afficher la valeru de BIsMyTurn
	UE_LOG(LogTemp, Warning, TEXT("bIsMyTurn: %s"), bIsMyTurn ? TEXT("true") : TEXT("false"));
}

void UUTurnCombatComponent::EndTurn()
{

	bIsMyTurn = false;
	// Notify game state to end turn
	OwningCharacter = GetOwner();
	if (OwningCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("C'est la fin de mon tour ! Actor: %s"), *OwningCharacter->GetName());
		//ajoute 1 point d'action à l'acteur
		UUStatsComponent* StatsComp = OwningCharacter->FindComponentByClass<UUStatsComponent>();
		if (StatsComp)
		{
			StatsComp->ActionPoints += 1;

			if (StatsComp->ActionPoints > StatsComp->MaxActionPoints)
			{
				StatsComp->ActionPoints = StatsComp->MaxActionPoints; // Ne pas dépasser le maximum
			}
			UE_LOG(LogTemp, Warning, TEXT("Action Points of %s reset to %d"), *OwningCharacter->GetName(), StatsComp->ActionPoints);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UUStatsComponent not found on actor: %s"), *OwningCharacter->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("C'est la fin de mon tour ! Actor: inconnu"));
	}

	//afficher la valeru de BIsMyTurn
	UE_LOG(LogTemp, Warning, TEXT("bIsMyTurn: %s"), bIsMyTurn ? TEXT("true") : TEXT("false"));
	TurnGameMode->OnTurnEnded();

}

void UUTurnCombatComponent::testFunction()
{
	TurnGameMode = Cast<ATurnGameMode>(GetWorld()->GetAuthGameMode());
	if (TurnGameMode)
	{
		TurnGameMode->testFunction();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMode is not set or not of type ATurnGameState"));
	}
}

void UUTurnCombatComponent::excuteAction(AActor* attackingActor, AActor* attackedActor, UUAttackDataComponent* attack)
{
	if (!attackingActor || !attackedActor || !attack)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid parameters for excuteAction"));
		if (!attackingActor) UE_LOG(LogTemp, Error, TEXT("attackingActor is null"));
		if (!attackedActor) UE_LOG(LogTemp, Error, TEXT("attackedActor is null"));
		if (!attack) UE_LOG(LogTemp, Error, TEXT("attack is null"));
		return;
	}

	// Log the action execution
	UE_LOG(LogTemp, Warning, TEXT("Executing action: %s from %s to %s"), *attack->AttackName, *attackingActor->GetName(), *attackedActor->GetName());

	//get the GameState to check if it's the right turn
	ATurnGameState* gameState = Cast<ATurnGameState>(GetWorld()->GetGameState());
	if (!gameState)
	{
		UE_LOG(LogTemp, Error, TEXT("GameState is not set or not of type ATurnGameState"));
		return;
	}

	//get stats component of the attacking actor
	UUStatsComponent* attackingStats = attackingActor->FindComponentByClass<UUStatsComponent>();
	if (!attackingStats)
	{
		UE_LOG(LogTemp, Error, TEXT("Attacking actor does not have a UUStatsComponent"));
		return;
	}

	UUStatsComponent* attackedStats = attackedActor->FindComponentByClass<UUStatsComponent>();
	if (!attackedStats)
	{
		UE_LOG(LogTemp, Error, TEXT("Attacked actor does not have a UUStatsComponent"));
		return;
	}
	bool specialAttack = false;
	// Calculate damage based on the attack actions
	int totalDamage = 0;

	int attackCost = attack->AttackCost;

	if (attackingStats->ActionPoints < attackCost)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s does not have enough Action Points to execute %s"), *attackingActor->GetName(), *attack->AttackName);
		EndTurn();
		return; // Not enough action points to execute the attack
	}

	attackingStats->ActionPoints -= attackCost; // Deduct action points for the attack
	UE_LOG(LogTemp, Warning, TEXT("%s uses %s and loses %d Action Points"), *attackingActor->GetName(), *attack->AttackName, attackCost);
	UE_LOG(LogTemp, Error, TEXT("%s has %d Action Points left"), *attackingActor->GetName(), attackingStats->ActionPoints);

	for (UActionComponent* action : attack->GetAttackActions())
	{
		if (action)
		{
			int damage = action->getBaseDamage();
			const FString& type = action->getType();
			int duration = action->getDuration();
			int attakiningAttack = 0;
			int attackedDefense = 0;


			if (duration > 0)
			{
				specialAttack = true;

				if (type == "Stun" || type == "Poison" || type == "Intimidate")
				{
					//get status effect component of the attacked actor
					UStatusEffectComponent* statusEffect = attackedActor->FindComponentByClass<UStatusEffectComponent>();
					if (!statusEffect)
					{
						UE_LOG(LogTemp, Error, TEXT("Attacked actor does not have a UStatusEffectComponent"));
						return;
					}
					//get EEffectType form the status effect component
					// 
					// Log the status effect application
					UE_LOG(LogTemp, Warning, TEXT("Applying status effect: %s with duration %d"), *type, duration);
					if (statusEffect)
					{

						if (type == "Stun")
						{
							// Apply stun effect
							statusEffect->AddStatusEffect("Stun", EEffectType::Debuff, EEffectCode::Stun, 0, duration);
							UE_LOG(LogTemp, Warning, TEXT("%s is stunned for %d turns"), *attackedActor->GetName(), duration);

						}
						else if (type == "Poison")
						{
							// Apply poison effect
							statusEffect->AddStatusEffect("Poison", EEffectType::Debuff, EEffectCode::Poison, damage, duration);
							UE_LOG(LogTemp, Warning, TEXT("%s is poisoned for %d turns"), *attackedActor->GetName(), duration);
						}
						else if (type == "Intimidate")
						{
							// Apply intimidate effect
							statusEffect->AddStatusEffect("Intimidate", EEffectType::Debuff, EEffectCode::Intimidate, damage, duration);
							UE_LOG(LogTemp, Warning, TEXT("%s is intimidated for %d turns"), *attackedActor->GetName(), duration);
						}
						else
						{
							// Type inconnu
						}

					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("Attacked actor does not have a UStatusEffectComponent"));
					}
				}
				else if (type == "Heal" || type == "AttackUp" || type == "DefenseUp")
				{
					//get status effect component of the attacking actor
					UStatusEffectComponent* statusEffect = attackingActor->FindComponentByClass<UStatusEffectComponent>();
					if (statusEffect)
					{
						if (type == "Heal")
						{
							// Apply heal effect
							statusEffect->AddStatusEffect("Heal", EEffectType::Buff, EEffectCode::Heal, damage, duration);
						}
						else if (type == "AttackUp")
						{
							// Apply attack up effect
							statusEffect->AddStatusEffect("AttackUp", EEffectType::Buff, EEffectCode::AttackUp, damage, duration);
							UE_LOG(LogTemp, Warning, TEXT("%s increases attack by %d"), *attackingActor->GetName(), damage);
						}
						else if (type == "DefenseUp")
						{
							// Apply defense up effect
							statusEffect->AddStatusEffect("DefenseUp", EEffectType::Buff, EEffectCode::DefenseUp, damage, duration);
							UE_LOG(LogTemp, Warning, TEXT("%s increases defense by %d"), *attackingActor->GetName(), damage);
						}
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("Attacking actor does not have a UStatusEffectComponent"));
					}
				}
				else
				{

				}



			}

			//en fonction du type d'attaque, on applique la defense correspondante
			else if (type == "phy")
			{
				attakiningAttack = attackingStats->PhysicalAttack;
				attackedDefense = attackedStats->PhysicalDefense;
			}
			else if (type == "elm")
			{
				attakiningAttack = attackingStats->ElementalAttack;
				attackedDefense = attackedStats->ElementalDefense;
			}
			else if (type == "spr")
			{
				attakiningAttack = attackingStats->SpiritualAttack;
				attackedDefense = attackedStats->SpiritualDefense;
			}
			else if (type == "heal")
			{
				// Apply heal effect
				attackingStats->RecieveDamage(-damage); // Negative damage heals
				UE_LOG(LogTemp, Warning, TEXT("%s heals for %d"), *attackedActor->GetName(), damage);
				specialAttack = true;
				continue; // Skip further processing for heal actions
			}

			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Unknown action type: %s"), *type);
				continue; // Skip unknown action types
			}

			// Calculate effective damage
			totalDamage = damage + (1 + attakiningAttack / 100) * (1 - attackedDefense / 100);
			//UE_LOG(LogTemp, Warning, TEXT("Action: %s, Base Damage: %d, Effective Damage: %d"), *action->getType(), damage, totalDamage);

			// Apply damage to the attacked actor
			attackedStats->RecieveDamage(totalDamage);

			// log the health of the attacked actor
			UE_LOG(LogTemp, Warning, TEXT("Health of %s after attack: %d"), *attackedActor->GetName(), attackedStats->HealthPoints);

		}


	}
	// ajouter une action dans le GameState
	gameState->numberOfActions++;
	addActionToHistory(attackingActor, attack);

	//afficher le nombre d'actions
	UE_LOG(LogTemp, Warning, TEXT("Number of actions: %d"), gameState->numberOfActions);

	if (gameState->numberOfActions >= 3)
	{
		gameState->numberOfActions = 0;

		// Notify the combat component to end the turn
		EndTurn();
		return;
	}

	if (specialAttack)
	{
		// Log the special attack
		UE_LOG(LogTemp, Warning, TEXT("Special attack executed by %s on %s"), *attackingActor->GetName(), *attackedActor->GetName());
		EndTurn();
		return;
	}


	return;

}


void UUTurnCombatComponent::addActionToHistory(AActor* actor, UUAttackDataComponent* attack)
{
	if (!actor || !attack)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid parameters for addActionToHistory"));
		return;
	}
	ATurnGameState* gameState = Cast<ATurnGameState>(GetWorld()->GetGameState());
	if (!gameState)
	{
		UE_LOG(LogTemp, Error, TEXT("GameState is not set or not of type ATurnGameState"));
		return;
	}
	// Check if the actor is a player or an enemy
    bool isPlayer = actor->GetName().Equals(TEXT("BP_Main_Character"));
	if (isPlayer)
	{
		gameState->LastPLayerAction.Add(attack);
		if (gameState->LastPLayerAction.Num() > 3)
		{
			gameState->LastPLayerAction.RemoveAt(0); // Keep only the last 3 actions
		}
	}
	else
	{
		gameState->LastEnemyAction.Add(attack);
		if (gameState->LastEnemyAction.Num() > 2)
		{
			gameState->LastEnemyAction.RemoveAt(0); // Keep only the last 2 actions
		}
	}
}


