#include "CombatPolicyRunner.h"
#include "Misc/AssertionMacros.h"
#include <cfloat>
#include <cstring>
#include "TurnGameState.h"
#include "CombatJsonExporter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// --- Lifecycle -------------------------------------------------------------
UCombatPolicyRunner::UCombatPolicyRunner()
{
	// Desactiver le tick par defaut (l'activer si besoin via SetComponentTickEnabled(true))
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatPolicyRunner::BeginPlay()
{
	Super::BeginPlay();
	// Initialisation specifique si necessaire
}

void UCombatPolicyRunner::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Logique par frame si activee
}


// ---------- JSON helpers ----------
TSharedPtr<FJsonObject> UCombatPolicyRunner::ParseJson(const FString& JsonStr) const
{
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return MakeShareable(new FJsonObject);
	return Root;
}
float UCombatPolicyRunner::JNum(const TSharedPtr<FJsonObject>& O, const TCHAR* K, float D) const
{
	if (!O.IsValid()) return D; double v = 0.0; return O->TryGetNumberField(K, v) ? float(v) : D;
}
TArray<TSharedPtr<FJsonValue>> UCombatPolicyRunner::JArr(const TSharedPtr<FJsonObject>& O, const TCHAR* K) const
{
	const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
	return (O.IsValid() && O->TryGetArrayField(K, A)) ? *A : TArray<TSharedPtr<FJsonValue>>{};
}

void UCombatPolicyRunner::PadTo(TArray<TSharedPtr<FJsonObject>>& A, int32 N) const
{
	while (A.Num() < N) { A.Add(MakeShareable(new FJsonObject)); }
	if (A.Num() > N) A.SetNum(N);
}

// ---------- Bounds ----------
float UCombatPolicyRunner::Clamp01(float x) const { return FMath::Clamp(x, 0.f, 1.f); }
float UCombatPolicyRunner::N01(float v, float lo, float hi) const { return (hi <= lo) ? 0.f : Clamp01((v - lo) / (hi - lo)); }
UCombatPolicyRunner::FBounds UCombatPolicyRunner::MakeBounds(int Level) const
{
	Level = FMath::Clamp(Level, 1, 5);
	FBounds B;
	const float rMin = 80.f, rMax = 120.f, mult = 1.f + 0.75f * float(Level - 1);
	B.HpMaxMin = rMin * mult; B.HpMaxMax = rMax * mult;
	B.HpMin = 0.f;  B.HpMax = B.HpMaxMax;
	B.SpeedMin = 3.f + Level;  B.SpeedMax = 5.f + Level;
	B.ApMaxMin = 4.f; B.ApMaxMax = (Level >= 5) ? 5.f : 4.f;
	B.ApMin = 0.f;   B.ApMax = B.ApMaxMax;
	B.StatMin = 0.f; B.StatMax = 10.5f * Level;
	B.CostMin = 0.f; B.CostMax = 4.f;
	B.CodeMin = 0.f; B.CodeMax = 10.f;
	B.MultMin = 0.f; B.MultMax = 125.f;
	B.DurMin = 0.f;  B.DurMax = 4.f;
	B.RoundMin = 0.f; B.RoundMax = 50.f;
	B.ActsMin = 0.f;  B.ActsMax = 3.f;
	return B;
}

// ---------- Normalization of leafs ----------
UCombatPolicyRunner::FNormEffect UCombatPolicyRunner::NEff(const TSharedPtr<FJsonObject>& E, const FBounds& B) const
{
	FNormEffect r;
	r.Code = N01(JNum(E, TEXT("code")), B.CodeMin, B.CodeMax);
	r.Mult = N01(JNum(E, TEXT("multiplier")), B.MultMin, B.MultMax);
	r.Dur = N01(JNum(E, TEXT("duration")), B.DurMin, B.DurMax);
	return r;
}
UCombatPolicyRunner::FNormAction UCombatPolicyRunner::NAct(const TSharedPtr<FJsonObject>& A, const FBounds& B) const
{
	FNormAction r; r.Cost = N01(JNum(A, TEXT("cost")), B.CostMin, B.CostMax);
	TArray<TSharedPtr<FJsonValue>> arr = JArr(A, TEXT("effects"));
	TArray<TSharedPtr<FJsonObject>> objs; for (auto& v : arr) objs.Add(v->AsObject());
	PadTo(objs, 2);
	r.E1 = NEff(objs[0], B); r.E2 = NEff(objs[1], B);
	return r;
}

// ---------- Full normalize ----------
UCombatPolicyRunner::FNormalizedState UCombatPolicyRunner::Normalize(const TSharedPtr<FJsonObject>& Root) const
{
	FNormalizedState S;
	const TSharedPtr<FJsonObject> CS = Root->GetObjectField(TEXT("combat_stats"));
	const TSharedPtr<FJsonObject> AIS = CS.IsValid() ? CS->GetObjectField(TEXT("ai_stats")) : MakeShareable(new FJsonObject);
	const int LevelRaw = int(JNum(AIS, TEXT("level"), 1.f));
	const FBounds B = MakeBounds(LevelRaw);

	// recent (from ply_recent_actions)
	{
		TArray<TSharedPtr<FJsonValue>> v = JArr(Root, TEXT("ply_recent_actions"));
		TArray<TSharedPtr<FJsonObject>> a; for (auto& x : v) a.Add(x->AsObject());
		PadTo(a, 3);
		for (int i = 0; i < 3; ++i) S.EnemyRecent[i] = NAct(a[i], B);
	}
	// actives
	{
		TSharedPtr<FJsonObject> A = Root->GetObjectField(TEXT("actives"));
		TArray<TSharedPtr<FJsonValue>> ai = JArr(A, TEXT("ai_active_effects"));
		TArray<TSharedPtr<FJsonObject>> aio; for (auto& x : ai) aio.Add(x->AsObject());
		PadTo(aio, 2);
		S.AiActifs[0] = NEff(aio[0], B); S.AiActifs[1] = NEff(aio[1], B);

		TArray<TSharedPtr<FJsonValue>> en = JArr(A, TEXT("ply_active_effects"));
		TArray<TSharedPtr<FJsonObject>> eno; for (auto& x : en) eno.Add(x->AsObject());
		PadTo(eno, 2);
		S.EnemyActifs[0] = NEff(eno[0], B); S.EnemyActifs[1] = NEff(eno[1], B);
	}
	// AI stats
	{
		S.AI.Level = N01(JNum(AIS, TEXT("level")), B.LevelMin, B.LevelMax);
		S.AI.HpMax = N01(JNum(AIS, TEXT("hpMax")), B.HpMaxMin, B.HpMaxMax);
		S.AI.Hp = N01(JNum(AIS, TEXT("hp")), B.HpMin, B.HpMax);
		S.AI.Speed = N01(JNum(AIS, TEXT("speed")), B.SpeedMin, B.SpeedMax);

		const float apMaxRaw = AIS->HasField(TEXT("apMax")) ? JNum(AIS, TEXT("apMax")) : JNum(AIS, TEXT("apmax"));
		S.AI.ApMax = N01(apMaxRaw, B.ApMaxMin, B.ApMaxMax);
		S.AI.Ap = N01(JNum(AIS, TEXT("ap")), B.ApMin, B.ApMax);

		TSharedPtr<FJsonObject> Det = AIS->GetObjectField(TEXT("stats"));
		S.AI.Stats.PhyAtk = N01(JNum(Det, TEXT("phy_atk")), B.StatMin, B.StatMax);
		S.AI.Stats.PhyDef = N01(JNum(Det, TEXT("phy_def")), B.StatMin, B.StatMax);
		S.AI.Stats.SpiAtk = N01(JNum(Det, TEXT("spi_atk")), B.StatMin, B.StatMax);
		S.AI.Stats.SpiDef = N01(JNum(Det, TEXT("spi_def")), B.StatMin, B.StatMax);
		S.AI.Stats.EleAtk = N01(JNum(Det, TEXT("ele_atk")), B.StatMin, B.StatMax);
		S.AI.Stats.EleDef = N01(JNum(Det, TEXT("ele_def")), B.StatMin, B.StatMax);
	}
	// Enemy (nested ply_stats)
	{
		TSharedPtr<FJsonObject> Ply = AIS->GetObjectField(TEXT("ply_stats"));
		S.Enemy.Level = N01(JNum(Ply, TEXT("level")), B.LevelMin, B.LevelMax);
		S.Enemy.HpMax = N01(JNum(Ply, TEXT("hpMax")), B.HpMaxMin, B.HpMaxMax);
		S.Enemy.Hp = N01(JNum(Ply, TEXT("hp")), B.HpMin, B.HpMax);
	}
	// combat_state
	{
		TSharedPtr<FJsonObject> St = Root->GetObjectField(TEXT("combat_state"));
		S.RoundCount = N01(JNum(St, TEXT("round_count")), B.RoundMin, B.RoundMax);
		S.ActionLeft = N01(JNum(St, TEXT("action_left")), B.ActsMin, B.ActsMax);
	}
	// avail + history
	{
		TArray<TSharedPtr<FJsonValue>> av = JArr(Root, TEXT("ai_available_actions"));
		TArray<TSharedPtr<FJsonObject>> avo; for (auto& x : av) avo.Add(x->AsObject());
		PadTo(avo, 4);
		for (int i = 0; i < 4; ++i) S.AIAvail[i] = NAct(avo[i], B);

		TArray<TSharedPtr<FJsonValue>> hi = JArr(Root, TEXT("ai_actions_history"));
		TArray<TSharedPtr<FJsonObject>> hio; for (auto& x : hi) hio.Add(x->AsObject());
		PadTo(hio, 2);
		for (int i = 0; i < 2; ++i) S.AIHist[i] = NAct(hio[i], B);
	}
	return S;
}

// ---------- Flatten to 92 ----------
TArray<float> UCombatPolicyRunner::EncodeLikeStateEncoder(const FNormalizedState& N) const
{
	TArray<float> V; V.Reserve(92);
	auto Eff = [&](const FNormEffect& e) { V.Add(e.Code); V.Add(e.Mult); V.Add(e.Dur); };
	auto Act = [&](const FNormAction& a) { V.Add(a.Cost); Eff(a.E1); Eff(a.E2); };

	for (int i = 0; i < 3; ++i) Act(N.EnemyRecent[i]);
	for (int i = 0; i < 2; ++i) Eff(N.AiActifs[i]);
	for (int i = 0; i < 2; ++i) Eff(N.EnemyActifs[i]);
	V.Add(N.AI.Level); V.Add(N.AI.HpMax); V.Add(N.AI.Hp);
	V.Add(N.AI.Speed); V.Add(N.AI.ApMax); V.Add(N.AI.Ap);
	V.Add(N.AI.Stats.PhyAtk); V.Add(N.AI.Stats.PhyDef);
	V.Add(N.AI.Stats.SpiAtk); V.Add(N.AI.Stats.SpiDef);
	V.Add(N.AI.Stats.EleAtk); V.Add(N.AI.Stats.EleDef);
	V.Add(N.Enemy.Level); V.Add(N.Enemy.HpMax); V.Add(N.Enemy.Hp);
	V.Add(N.RoundCount); V.Add(N.ActionLeft);
	for (int i = 0; i < 4; ++i) Act(N.AIAvail[i]);
	for (int i = 0; i < 2; ++i) Act(N.AIHist[i]);
	check(V.Num() == 92);
	return V;
}

// ---------- Public: make 92 features ----------
TArray<float> UCombatPolicyRunner::EncodeJsonToFeatures92(const FString& JsonStr)
{
	TSharedPtr<FJsonObject> Root = ParseJson(JsonStr);
	if (!Root.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
		return {};
	}
	const FNormalizedState N = Normalize(Root);
	TArray<float> Features = EncodeLikeStateEncoder(N);
	if (Features.Num() != 92)
	{
		UE_LOG(LogTemp, Error, TEXT("Expected 92 features, got %d"), Features.Num());
		return {};
	}
	return Features;
}

// ---------- Mock AI Inference ----------
bool UCombatPolicyRunner::RunModelAndPrint(const TArray<float>& Features, const FString& ModelPath)
{
	UE_LOG(LogTemp, Warning, TEXT("Using Mock AI - LibTorch not integrated."));
	
	// Simple mock AI logic based on features
	TArray<float> MockLogits = {-0.2f, 0.1f, 0.8f, -0.3f, 0.4f}; // [Defense, Heal, Attack, Flee, Special]
	TArray<float> MockProbs = {0.15f, 0.2f, 0.45f, 0.1f, 0.1f};
	
	// Basic strategy based on features
	if (Features.Num() == 92)
	{
		// Use simple heuristics based on normalized features
		float aiHealthRatio = Features.Num() > 15 ? Features[15] : 0.5f; // Approximate AI HP ratio
		float enemyHealthRatio = Features.Num() > 20 ? Features[20] : 0.5f; // Approximate enemy HP ratio
		
		// Adjust strategy based on health
		if (aiHealthRatio < 0.3f) {
			MockLogits[1] = 1.2f; // Favor healing
			MockProbs[1] = 0.5f; MockProbs[2] = 0.2f; // Adjust probabilities
		} else if (enemyHealthRatio < 0.4f) {
			MockLogits[2] = 1.5f; // Favor attack when enemy is weak
			MockProbs[2] = 0.6f; MockProbs[1] = 0.1f;
		}
		
		UE_LOG(LogTemp, Display, TEXT("AI Health Ratio: %.2f, Enemy Health Ratio: %.2f"), aiHealthRatio, enemyHealthRatio);
	}
	
	PrintOutputs(MockLogits, MockProbs);
	
	UE_LOG(LogTemp, Display, TEXT("Mock AI decision completed. Model path: %s"), *ModelPath);
	UE_LOG(LogTemp, Display, TEXT("Features processed: %d/92"), Features.Num());
	
	return true;
}

// ---------- Print ----------
void UCombatPolicyRunner::PrintOutputs(const TArray<float>& Logits, const TArray<float>& Probs) const
{
	FString L; for (int i = 0; i < Logits.Num(); ++i) L += FString::Printf(TEXT("%s%.4f"), (i ? TEXT(", ") : TEXT("")), Logits[i]);
	FString P; for (int i = 0; i < Probs.Num(); ++i) P += FString::Printf(TEXT("%s%.4f"), (i ? TEXT(", ") : TEXT("")), Probs[i]);

	UE_LOG(LogTemp, Display, TEXT("Model logits [5]: [%s]"), *L);
	UE_LOG(LogTemp, Display, TEXT("Softmax probs [5]: [%s]"), *P);

	int BestIdx = 0; float Best = Probs.Num() ? Probs[0] : -FLT_MAX;
	for (int i = 1; i < Probs.Num(); ++i) { if (Probs[i] > Best) { Best = Probs[i]; BestIdx = i; } }
	UE_LOG(LogTemp, Display, TEXT("Picked action index: %d (1-based: %d), prob=%.4f"), BestIdx, BestIdx + 1, Best);
}

void UCombatPolicyRunner::executeAction()
{
	//cree le json 
	ATurnGameState* GameState = Cast<ATurnGameState>(GetOwner()->GetWorld()->GetGameState());
	if (!GameState)
	{
		UE_LOG(LogTemp, Error, TEXT("GameState is not of type ATurnGameState"));
		return;
	}
	UCombatJsonExporter* Exporter = NewObject<UCombatJsonExporter>();
	TSharedPtr<FJsonObject> CombatJson = Exporter->MakeCombatStateToJson(GameState);

    // Chemin vers l'exécutable Python et le script
    FString PythonExecutable = TEXT("python"); // ou chemin complet vers python.exe
    FString ScriptPath = FPaths::ProjectDir() + TEXT("Python/main.py");
    
    // Vérifier si le fichier existe
    if (!FPaths::FileExists(ScriptPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Script Python non trouvé: %s"), *ScriptPath);
        return;
    }
    
    // Paramètres pour le script Python
    FString Parameters = FString::Printf(TEXT("\"%s\""), *ScriptPath);
    
    // Variables pour le processus
    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;
    
    // Créer le processus Python
    FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
        *PythonExecutable,
        *Parameters,
        false,      // bLaunchDetached
        true,       // bLaunchHidden
        true,       // bLaunchReallyHidden
        nullptr,    // OutProcessID
        0,          // PriorityModifier
        nullptr,    // OptionalWorkingDirectory
        WritePipe,  // PipeWriteChild
        ReadPipe    // PipeReadChild
    );
    
    if (ProcessHandle.IsValid())
    {

		UE_LOG(LogTemp, Log, TEXT("Script Python lancé avec succès: %s"), *ScriptPath);
        
        // Attendre la fin du processus (optionnel)
        FPlatformProcess::WaitForProc(ProcessHandle);
        
        // Lire la sortie si nécessaire
        if (ReadPipe)
        {
            FString Output = FPlatformProcess::ReadPipe(ReadPipe);
            if (!Output.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Sortie Python: %s"), *Output);
            }
        }
        
        // Nettoyer
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        FPlatformProcess::CloseProc(ProcessHandle);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Échec du lancement du script Python"));
    }


}