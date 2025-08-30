#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusEffectComponent.generated.h"

UENUM(BlueprintType)
enum class EEffectType : uint8
{
    Buff    UMETA(DisplayName = "Buff"),
    Debuff  UMETA(DisplayName = "Debuff")
};

UENUM(BlueprintType)
enum class EEffectCode : uint8
{
    None = 0,
    AttackUp = 4,    // Buff: Attack stat enhancement
    DefenseUp = 5,   // Buff: Defense stat enhancement
    Heal = 6,        // Buff: HP restoration
    Stun = 7,        // Debuff: skip turns
    Poison = 8,      // Debuff: damage over time
    Intimidate = 9   // Debuff: reduce attack/defense
};


USTRUCT(BlueprintType)
struct FStatusEffect
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category="Status")
    FString Name;

    UPROPERTY(BlueprintReadWrite, Category="Status")
    EEffectType EffectType;

    UPROPERTY(BlueprintReadWrite, Category="Status")
    EEffectCode Code;

    UPROPERTY(BlueprintReadWrite, Category="Status")
    int32 Power; // Dégâts ou valeur du buff

    UPROPERTY(BlueprintReadWrite, Category="Status")
    int32 RemainingTurns;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPG_HD2D_API UStatusEffectComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStatusEffectComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Status Effects")
    TArray<FStatusEffect> ActiveEffects;

    /** Ajoute ou remplace un effet (buff ou debuff) */
    UFUNCTION(BlueprintCallable, Category="Status Effects")
    void AddStatusEffect(const FString& Name, EEffectType Type, EEffectCode Code, int32 Power, int32 Duration);

    /** Applique les effets au début du tour et réduit leur durée */
    UFUNCTION(BlueprintCallable, Category="Status Effects")
    void ApplyStatusEffects();

    /** Récupère le buff actif */
    UFUNCTION(BlueprintCallable, Category="Status Effects")
    FStatusEffect GetBuff();

    /** Récupère le debuff actif */
    UFUNCTION(BlueprintCallable, Category="Status Effects")
    FStatusEffect GetDebuff();
};
