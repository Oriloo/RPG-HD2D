
#include "CombatJsonExporter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UActionComponent.h"
#include "UStatsComponent.h"

// ---- Action JSON ----
TSharedPtr<FJsonObject> UCombatJsonExporter::BuildPLayerLastActionsToJson(const ATurnGameState* GameState)
{
	if (!GameState) return MakeShareable(new FJsonObject);

	TArray<UUAttackDataComponent*> LastActions = GameState->LastPLayerAction;
	TArray<TSharedPtr<FJsonValue>> ActionsJsonArray;

	for (int actionIdx = 0; actionIdx < 3; ++actionIdx)
	{
		UUAttackDataComponent* action = (actionIdx < LastActions.Num()) ? LastActions[actionIdx] : nullptr;
		TSharedPtr<FJsonObject> ActionJson = MakeShareable(new FJsonObject);

		ActionJson->SetNumberField(TEXT("cost"), action ? action->AttackCost : 0);

		TArray<TSharedPtr<FJsonValue>> EffectsArray;
		TArray<UActionComponent*> AttackActions = action ? action->GetAttackActions() : TArray<UActionComponent*>();

		for (int i = 0; i < 2; ++i)
		{
			UActionComponent* act = (i < AttackActions.Num()) ? AttackActions[i] : nullptr;
			TSharedPtr<FJsonObject> EffectJson = MakeShareable(new FJsonObject);
			EffectJson->SetNumberField(TEXT("code"), act ? act->getTypeCode() : 0);
			EffectJson->SetNumberField(TEXT("multiplier"), act ? act->getBaseDamage() : 0);
			EffectJson->SetNumberField(TEXT("duration"), act ? act->getDuration() : 0);
			EffectsArray.Add(MakeShareable(new FJsonValueObject(EffectJson)));
		}

		ActionJson->SetArrayField(TEXT("effects"), EffectsArray);
		ActionsJsonArray.Add(MakeShareable(new FJsonValueObject(ActionJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetArrayField(TEXT("ply_recent_actions"), ActionsJsonArray);

	return ResultJson;
}

TSharedPtr<FJsonObject> UCombatJsonExporter::AIActiveEffectToJson(const ATurnGameState* GameState)
{
	if (!GameState || !GameState->AiActiveEffect) return MakeShareable(new FJsonObject);

	UStatusEffectComponent* AIEffects = GameState->AiActiveEffect;
	TArray<TSharedPtr<FJsonValue>> EffectsArray;

	FStatusEffect Buff = AIEffects->GetBuff();
	FStatusEffect Debuff = AIEffects->GetDebuff();

	auto AddEffect = [&](const FStatusEffect& Eff)
		{
			TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
			Json->SetNumberField(TEXT("code"), static_cast<int32>(Eff.Code));
			Json->SetNumberField(TEXT("multiplier"), Eff.Power);
			Json->SetNumberField(TEXT("duration"), Eff.RemainingTurns);
			EffectsArray.Add(MakeShareable(new FJsonValueObject(Json)));
		};

	AddEffect(Buff);
	AddEffect(Debuff);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetArrayField(TEXT("ai_active_effects"), EffectsArray);

	return ResultJson;
}

TSharedPtr<FJsonObject> UCombatJsonExporter::PlayerActiveEffectToJson(const ATurnGameState* GameState)
{
	if (!GameState || !GameState->PlayerActiveEffect) return MakeShareable(new FJsonObject);

	UStatusEffectComponent* PlayerEffects = GameState->PlayerActiveEffect;
	TArray<TSharedPtr<FJsonValue>> EffectsArray;

	FStatusEffect Buff = PlayerEffects->GetBuff();
	FStatusEffect Debuff = PlayerEffects->GetDebuff();

	auto AddEffect = [&](const FStatusEffect& Eff)
		{
			TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
			Json->SetNumberField(TEXT("code"), static_cast<int32>(Eff.Code));
			Json->SetNumberField(TEXT("multiplier"), Eff.Power);
			Json->SetNumberField(TEXT("duration"), Eff.RemainingTurns);
			EffectsArray.Add(MakeShareable(new FJsonValueObject(Json)));
		};

	AddEffect(Buff);
	AddEffect(Debuff);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetArrayField(TEXT("ply_active_effects"), EffectsArray);

	return ResultJson;
}

TSharedPtr<FJsonObject> UCombatJsonExporter::CombatantStatsToJson(const ATurnGameState* GameState)
{
	if (!GameState) return MakeShareable(new FJsonObject);
	TArray<AActor*> TurnOrder = GameState->TurnOrder;

	UUStatsComponent* PlayerStatsComp = nullptr;
	UUStatsComponent* AiStatsComp = nullptr;

	for (AActor* Actor : TurnOrder)
	{
		if (Actor->GetName() == TEXT("BP_Main_Character"))
			PlayerStatsComp = Cast<UUStatsComponent>(Actor->GetComponentByClass(UUStatsComponent::StaticClass()));
		else
			AiStatsComp = Cast<UUStatsComponent>(Actor->GetComponentByClass(UUStatsComponent::StaticClass()));
	}

	TSharedPtr<FJsonObject> AiStatsJson = MakeShareable(new FJsonObject);
	TSharedPtr<FJsonObject> PlyStatsJson = MakeShareable(new FJsonObject);

	auto FillStats = [](UUStatsComponent* StatsComp, TSharedPtr<FJsonObject> StatsJson)
		{
			if (StatsComp)
			{
				StatsJson->SetNumberField(TEXT("level"), 0);
				StatsJson->SetNumberField(TEXT("hpMax"), StatsComp->MaxHealthPoints);
				StatsJson->SetNumberField(TEXT("hp"), StatsComp->HealthPoints);

				TSharedPtr<FJsonObject> SubStats = MakeShareable(new FJsonObject);
				SubStats->SetNumberField(TEXT("phy_atk"), StatsComp->PhysicalAttack);
				SubStats->SetNumberField(TEXT("phy_def"), StatsComp->PhysicalDefense);
				SubStats->SetNumberField(TEXT("spi_atk"), StatsComp->SpiritualAttack);
				SubStats->SetNumberField(TEXT("spi_def"), StatsComp->SpiritualDefense);
				SubStats->SetNumberField(TEXT("ele_atk"), StatsComp->ElementalAttack);
				SubStats->SetNumberField(TEXT("ele_def"), StatsComp->ElementalDefense);

				StatsJson->SetObjectField(TEXT("stats"), SubStats);
			}
			else
			{
				StatsJson->SetNumberField(TEXT("level"), 0);
				StatsJson->SetNumberField(TEXT("hpMax"), 0);
				StatsJson->SetNumberField(TEXT("hp"), 0);

				TSharedPtr<FJsonObject> SubStats = MakeShareable(new FJsonObject);
				SubStats->SetNumberField(TEXT("phy_atk"), 0);
				SubStats->SetNumberField(TEXT("phy_def"), 0);
				SubStats->SetNumberField(TEXT("spi_atk"), 0);
				SubStats->SetNumberField(TEXT("spi_def"), 0);
				SubStats->SetNumberField(TEXT("ele_atk"), 0);
				SubStats->SetNumberField(TEXT("ele_def"), 0);

				StatsJson->SetObjectField(TEXT("stats"), SubStats);
			}
		};

	FillStats(AiStatsComp, AiStatsJson);
	FillStats(PlayerStatsComp, PlyStatsJson);

	TSharedPtr<FJsonObject> CombatStatsJson = MakeShareable(new FJsonObject);
	CombatStatsJson->SetObjectField(TEXT("ai_stats"), AiStatsJson);
	AiStatsJson->SetObjectField(TEXT("ply_stats"), PlyStatsJson);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetObjectField(TEXT("combat_stats"), CombatStatsJson);

	return ResultJson;
}

TSharedPtr<FJsonObject> UCombatJsonExporter::CombatStateToJson(const ATurnGameState* GameState)
{
	TSharedPtr<FJsonObject> CombatStateJson = MakeShareable(new FJsonObject);
	CombatStateJson->SetNumberField(TEXT("round_count"), GameState ? GameState->CurrentRound : 0);
	CombatStateJson->SetNumberField(TEXT("action_left"), GameState ? 3 - GameState->numberOfActions : 0);

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetObjectField(TEXT("combat_state"), CombatStateJson);

	return ResultJson;
}

TSharedPtr<FJsonObject> UCombatJsonExporter::BuildDisponibleActionToJson(const ATurnGameState* GameState)
{
	if (!GameState) return MakeShareable(new FJsonObject);

	TArray<TSharedPtr<FJsonValue>> ActionsJsonArray;

	for (AActor* Actor : GameState->TurnOrder)
	{
		if (Actor->GetName() != TEXT("BP_Main_Character"))
		{
			TArray<UUAttackDataComponent*> AvailableActions;
			Actor->GetComponents<UUAttackDataComponent>(AvailableActions);

			for (int actionIdx = 0; actionIdx < 4; ++actionIdx)
			{
				UUAttackDataComponent* action = (actionIdx < AvailableActions.Num()) ? AvailableActions[actionIdx] : nullptr;
				TSharedPtr<FJsonObject> ActionJson = MakeShareable(new FJsonObject);
				ActionJson->SetNumberField(TEXT("cost"), action ? action->AttackCost : 0);

				TArray<TSharedPtr<FJsonValue>> EffectsArray;
				TArray<UActionComponent*> AttackActions = action ? action->GetAttackActions() : TArray<UActionComponent*>();
				for (int i = 0; i < 2; ++i)
				{
					UActionComponent* act = (i < AttackActions.Num()) ? AttackActions[i] : nullptr;
					TSharedPtr<FJsonObject> EffectJson = MakeShareable(new FJsonObject);
					EffectJson->SetNumberField(TEXT("code"), act ? act->getTypeCode() : 0);
					EffectJson->SetNumberField(TEXT("multiplier"), act ? act->getBaseDamage() : 0);
					EffectJson->SetNumberField(TEXT("duration"), act ? act->getDuration() : 0);
					EffectsArray.Add(MakeShareable(new FJsonValueObject(EffectJson)));
				}

				ActionJson->SetArrayField(TEXT("effects"), EffectsArray);
				ActionsJsonArray.Add(MakeShareable(new FJsonValueObject(ActionJson)));
			}
			break; // On ne prend que le premier ennemi
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetArrayField(TEXT("ai_available_actions"), ActionsJsonArray);
	return ResultJson;
}

TSharedPtr<FJsonObject> UCombatJsonExporter::BuildAILastActionsToJson(const ATurnGameState* GameState)
{
	if (!GameState) return MakeShareable(new FJsonObject);

	TArray<UUAttackDataComponent*> LastActions = GameState->LastEnemyAction;
	TArray<TSharedPtr<FJsonValue>> ActionsJsonArray;

	for (int actionIdx = 0; actionIdx < 2; ++actionIdx)
	{
		UUAttackDataComponent* action = (actionIdx < LastActions.Num()) ? LastActions[actionIdx] : nullptr;
		TSharedPtr<FJsonObject> ActionJson = MakeShareable(new FJsonObject);

		ActionJson->SetNumberField(TEXT("cost"), action ? action->AttackCost : 0);

		TArray<TSharedPtr<FJsonValue>> EffectsArray;
		TArray<UActionComponent*> AttackActions = action ? action->GetAttackActions() : TArray<UActionComponent*>();
		for (int i = 0; i < 2; ++i)
		{
			UActionComponent* act = (i < AttackActions.Num()) ? AttackActions[i] : nullptr;
			TSharedPtr<FJsonObject> EffectJson = MakeShareable(new FJsonObject);
			EffectJson->SetNumberField(TEXT("code"), act ? act->getTypeCode() : 0);
			EffectJson->SetNumberField(TEXT("multiplier"), act ? act->getBaseDamage() : 0);
			EffectJson->SetNumberField(TEXT("duration"), act ? act->getDuration() : 0);
			EffectsArray.Add(MakeShareable(new FJsonValueObject(EffectJson)));
		}

		ActionJson->SetArrayField(TEXT("effects"), EffectsArray);
		ActionsJsonArray.Add(MakeShareable(new FJsonValueObject(ActionJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetArrayField(TEXT("ai_action_history"), ActionsJsonArray);
	return ResultJson;
}

// ---- Main Export Function ----
TSharedPtr<FJsonObject> UCombatJsonExporter::MakeCombatStateToJson(const ATurnGameState* GameState)
{
	TSharedPtr<FJsonObject> PlayerActionsJson = BuildPLayerLastActionsToJson(GameState);
	TSharedPtr<FJsonObject> AiEffectsJson = AIActiveEffectToJson(GameState);
	TSharedPtr<FJsonObject> PlyEffectsJson = PlayerActiveEffectToJson(GameState);
	TSharedPtr<FJsonObject> CombatStatsJson = CombatantStatsToJson(GameState);
	TSharedPtr<FJsonObject> CombatStateJson = CombatStateToJson(GameState);
	TSharedPtr<FJsonObject> AiAvailableJson = BuildDisponibleActionToJson(GameState);
	TSharedPtr<FJsonObject> AiHistoryJson = BuildAILastActionsToJson(GameState);

	TSharedPtr<FJsonObject> ActivesJson = MakeShareable(new FJsonObject);
	ActivesJson->SetArrayField(TEXT("ai_active_effects"), AiEffectsJson->HasField(TEXT("ai_active_effects")) ? AiEffectsJson->GetArrayField(TEXT("ai_active_effects")) : TArray<TSharedPtr<FJsonValue>>());
	ActivesJson->SetArrayField(TEXT("ply_active_effects"), PlyEffectsJson->HasField(TEXT("ply_active_effects")) ? PlyEffectsJson->GetArrayField(TEXT("ply_active_effects")) : TArray<TSharedPtr<FJsonValue>>());

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetArrayField(TEXT("ply_recent_actions"), PlayerActionsJson->GetArrayField(TEXT("ply_recent_actions")));
	ResultJson->SetObjectField(TEXT("actives"), ActivesJson);
	ResultJson->SetObjectField(TEXT("combat_stats"), CombatStatsJson->GetObjectField(TEXT("combat_stats")));
	ResultJson->SetObjectField(TEXT("combat_state"), CombatStateJson->GetObjectField(TEXT("combat_state")));
	ResultJson->SetArrayField(TEXT("ai_available_actions"), AiAvailableJson->HasField(TEXT("ai_available_actions")) ? AiAvailableJson->GetArrayField(TEXT("ai_available_actions")) : TArray<TSharedPtr<FJsonValue>>());
	ResultJson->SetArrayField(TEXT("ai_actions_history"), AiHistoryJson->HasField(TEXT("ai_action_history")) ? AiHistoryJson->GetArrayField(TEXT("ai_action_history")) : TArray<TSharedPtr<FJsonValue>>());

	return ResultJson;
}
