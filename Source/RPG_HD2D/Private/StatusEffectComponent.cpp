#include "StatusEffectComponent.h"
#include "GameFramework/Actor.h"
#include "UStatsComponent.h"
#include "UTurnCombatComponent.h"

UStatusEffectComponent::UStatusEffectComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStatusEffectComponent::AddStatusEffect(const FString& Name, EEffectType Type, EEffectCode Code, int32 Power, int32 Duration)
{
    for (FStatusEffect& Effect : ActiveEffects)
    {
        if (Effect.EffectType == Type)
        {
            // Remplace l'effet existant
            Effect.Name = Name;
            Effect.Code = Code;
            Effect.Power = Power;
            Effect.RemainingTurns = Duration;

            UE_LOG(LogTemp, Log, TEXT("%s remplace son %s par %s"),
                *GetOwner()->GetName(), Type == EEffectType::Buff ? TEXT("Buff") : TEXT("Debuff"), *Name);
            return;
        }
    }

    // Sinon ajoute un nouvel effet
    FStatusEffect NewEffect;
    NewEffect.Name = Name;
    NewEffect.EffectType = Type;
    NewEffect.Code = Code;
    NewEffect.Power = Power;
    NewEffect.RemainingTurns = Duration;
    ActiveEffects.Add(NewEffect);

    UE_LOG(LogTemp, Log, TEXT("%s reçoit %s (%s) pour %d tours"),
        *GetOwner()->GetName(), *Name, Type == EEffectType::Buff ? TEXT("Buff") : TEXT("Debuff"), Duration);
}

void UStatusEffectComponent::ApplyStatusEffects()
{
    UUStatsComponent* StatsComp = GetOwner()->FindComponentByClass<UUStatsComponent>();
    if (!StatsComp)
    {
        UE_LOG(LogTemp, Error, TEXT("UUStatsComponent not found on actor: %s"), *GetOwner()->GetName());
        return;
	}
    UUTurnCombatComponent* TurnComp = GetOwner()->FindComponentByClass<UUTurnCombatComponent>();


    for (int32 i = ActiveEffects.Num() - 1; i >= 0; --i)
    {
        FStatusEffect& Effect = ActiveEffects[i];

        switch (Effect.Code)
        {
        case EEffectCode::Heal:
            // Heal l'acteur
            UE_LOG(LogTemp, Log, TEXT("%s récupère %d HP grâce à %s"), *GetOwner()->GetName(), Effect.Power, *Effect.Name);
			StatsComp->HealthPoints += Effect.Power;
            break;

        case EEffectCode::Poison:
            // Inflige des dégâts sur le temps
            UE_LOG(LogTemp, Log, TEXT("%s subit %d dégâts de %s"), *GetOwner()->GetName(), Effect.Power, *Effect.Name);
			StatsComp->RecieveDamage(Effect.Power);
            break;

        case EEffectCode::Stun:
            // Peut être utilisé pour sauter le tour
            UE_LOG(LogTemp, Log, TEXT("%s est étourdi par %s et doit sauter son tour"), *GetOwner()->GetName(), *Effect.Name);
			//get the turn combat component of the owner
            if (TurnComp)
            {
                TurnComp->EndTurn();
            }
            break;

        case EEffectCode::AttackUp:
            // Effets de stat, appliquer selon votre système de combat
            UE_LOG(LogTemp, Log, TEXT("%s est affecté par %s"), *GetOwner()->GetName(), *Effect.Name);
			StatsComp->PhysicalAttack += Effect.Power;
			StatsComp->ElementalAttack += Effect.Power;
			StatsComp->SpiritualAttack += Effect.Power;
			break;

        case EEffectCode::DefenseUp:
            // Effets de stat, appliquer selon votre système de combat
            UE_LOG(LogTemp, Log, TEXT("%s est affecté par %s"), *GetOwner()->GetName(), *Effect.Name);
			StatsComp->PhysicalDefense += Effect.Power;
			StatsComp->ElementalDefense += Effect.Power;
			StatsComp->SpiritualDefense += Effect.Power;
			break;

        case EEffectCode::Intimidate:
            // Effets de stat, appliquer selon votre système de combat
            UE_LOG(LogTemp, Log, TEXT("%s est affecté par %s"), *GetOwner()->GetName(), *Effect.Name);
			StatsComp->PhysicalAttack -= Effect.Power;
			StatsComp->ElementalAttack -= Effect.Power;
			StatsComp->SpiritualAttack -= Effect.Power;
			StatsComp->PhysicalDefense -= Effect.Power;
			StatsComp->ElementalDefense -= Effect.Power;
			StatsComp->SpiritualDefense -= Effect.Power;
            break;

        default:
            break;
        }

        Effect.RemainingTurns--;

        if (Effect.RemainingTurns <= 0)
        {
            UE_LOG(LogTemp, Log, TEXT("%s n'est plus affecté par %s"), *GetOwner()->GetName(), *Effect.Name);
            if (Effect.Code == EEffectCode::AttackUp || Effect.Code == EEffectCode::DefenseUp || Effect.Code == EEffectCode::Intimidate)
            {
                if (Effect.Code == EEffectCode::AttackUp)
                {
                    UE_LOG(LogTemp, Log, TEXT("%s voit son attaque revenir à la normale après %s"), *GetOwner()->GetName(), *Effect.Name);
					StatsComp->PhysicalAttack -= Effect.Power;
					StatsComp->ElementalAttack -= Effect.Power;
					StatsComp->SpiritualAttack -= Effect.Power;
                }
                else if (Effect.Code == EEffectCode::DefenseUp)
                {
                    UE_LOG(LogTemp, Log, TEXT("%s voit sa défense revenir à la normale après %s"), *GetOwner()->GetName(), *Effect.Name);
					StatsComp->PhysicalDefense -= Effect.Power;
					StatsComp->ElementalDefense -= Effect.Power;
					StatsComp->SpiritualDefense -= Effect.Power;
                    
                }
                else if (Effect.Code == EEffectCode::Intimidate)
                {
                    UE_LOG(LogTemp, Log, TEXT("%s voit son attaque diminuer après %s"), *GetOwner()->GetName(), *Effect.Name);
					StatsComp->PhysicalAttack += Effect.Power;
					StatsComp->ElementalAttack += Effect.Power;
					StatsComp->SpiritualAttack += Effect.Power;
					StatsComp->PhysicalDefense += Effect.Power;
					StatsComp->ElementalDefense += Effect.Power;
					StatsComp->SpiritualDefense += Effect.Power;
				}
			}
            ActiveEffects.RemoveAt(i);
        }
    }
}

FStatusEffect UStatusEffectComponent::GetBuff()
{
    for (FStatusEffect& Effect : ActiveEffects)
    {
        if (Effect.EffectType == EEffectType::Buff)
            return Effect;

    }
    return FStatusEffect(); // Retourne une valeur nulle par défaut
}

FStatusEffect UStatusEffectComponent::GetDebuff()
{
    for (FStatusEffect& Effect : ActiveEffects)
    {
        if (Effect.EffectType == EEffectType::Debuff)
            return Effect;
    }
    return FStatusEffect(); // Retourne une valeur nulle par défaut
}
