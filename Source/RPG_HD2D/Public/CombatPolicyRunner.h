// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "CombatPolicyRunner.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPG_HD2D_API UCombatPolicyRunner : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Constructor
	UCombatPolicyRunner();

	/** Encode JSON en 92 features normalisees. */
	UFUNCTION(BlueprintCallable, Category="CombatPolicy")
	TArray<float> EncodeJsonToFeatures92(const FString& JsonStr);

	/** Charge le modele .pt, lance l'inference et affiche les resultats. */
	UFUNCTION(BlueprintCallable, Category="CombatPolicy")
	bool RunModelAndPrint(const TArray<float>& Features, const FString& ModelPath);

	/** Execute une action en utilisant l'IA basee sur l'etat actuel du combat */
	UFUNCTION(BlueprintCallable, Category="CombatPolicy")
	void executeAction();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// ---------- JSON helpers ----------
	TSharedPtr<FJsonObject> ParseJson(const FString& JsonStr) const;
	float JNum(const TSharedPtr<FJsonObject>& O, const TCHAR* K, float D=0.f) const;
	TArray<TSharedPtr<FJsonValue>> JArr(const TSharedPtr<FJsonObject>& O, const TCHAR* K) const;
	// Specialisation concrete (plus de template) pour eviter problemes de deduction
	void PadTo(TArray<TSharedPtr<FJsonObject>>& A, int32 N) const;

	// ---------- Normalization bounds ----------
	struct FBounds {
		float LevelMin=1, LevelMax=5;
		float HpMaxMin=80, HpMaxMax=120;
		float HpMin=0,   HpMax=120;
		float SpeedMin=0, SpeedMax=0;
		float ApMaxMin=4, ApMaxMax=4;
		float ApMin=0,   ApMax=4;
		float StatMin=0, StatMax=0;
		float CostMin=0, CostMax=4;
		float CodeMin=0, CodeMax=10;
		float MultMin=0, MultMax=125;
		float DurMin=0,  DurMax=4;
		float RoundMin=0, RoundMax=50;
		float ActsMin=0, ActsMax=3;
	};
	float Clamp01(float x) const;
	float N01(float v, float lo, float hi) const;
	FBounds MakeBounds(int Level) const;

	// ---------- Normalized data structs ----------
	struct FNormEffect{ float Code=0, Mult=0, Dur=0; };
	struct FNormAction{ float Cost=0; FNormEffect E1, E2; };
	struct FNormStats6{ float PhyAtk=0,PhyDef=0,SpiAtk=0,SpiDef=0,EleAtk=0,EleDef=0; };
	struct FNormAIStats{
		float Level=0, HpMax=0, Hp=0, Speed=0, ApMax=0, Ap=0;
		FNormStats6 Stats;
	};
	struct FNormEnemyStats{ float Level=0, HpMax=0, Hp=0; };
	struct FNormalizedState {
		FNormAction EnemyRecent[3];
		FNormEffect AiActifs[2];
		FNormEffect EnemyActifs[2];
		FNormAIStats AI;
		FNormEnemyStats Enemy;
		float RoundCount=0, ActionLeft=0;
		FNormAction AIAvail[4];
		FNormAction AIHist[2];
	};

	// ---------- Normalization pipeline ----------
	FNormEffect NEff(const TSharedPtr<FJsonObject>& E, const FBounds& B) const;
	FNormAction NAct(const TSharedPtr<FJsonObject>& A, const FBounds& B) const;
	FNormalizedState Normalize(const TSharedPtr<FJsonObject>& Root) const;
	TArray<float> EncodeLikeStateEncoder(const FNormalizedState& N) const;

	void PrintOutputs(const TArray<float>& Logits, const TArray<float>& Probs) const;
};
