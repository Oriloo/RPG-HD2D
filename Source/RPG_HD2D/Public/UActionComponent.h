// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UActionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPG_HD2D_API UActionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UActionComponent();
    UActionComponent(const FString& type, int baseDamage, int duration);
    ~UActionComponent();
    
    const FString& getType();
	const int getTypeCode();
    void setType(const FString& newType);
    int getBaseDamage();
    void setBaseDamage(int newBaseDamage);
    int getDuration();
    void setDuration(int newDuration);

private: 
    /*const FString& type; */
    FString type;
    int baseDamage;
    int duration;
};
