// Fill out your copyright notice in the Description page of Project Settings.


#include "UActionComponent.h"

UActionComponent::UActionComponent() : type(TEXT("")), baseDamage(0), duration(0) {}

UActionComponent::UActionComponent(const FString& type, int duration, int baseDamage)
{
	this->type = type; 
	this->duration = duration;
	this->baseDamage = baseDamage;
	
}

const FString& UActionComponent::getType()
{
	return type;
}

const int UActionComponent::getTypeCode()
{
	if (type == "phy")
	{
		return 1;
	}
	else if (type == "elm")
	{
		return 2;
	}
	else if (type == "spr")
	{
		return 3;
	}
	else if (type == "heal")
	{
		return 6;
	}
	else if (type == "Stun")
	{
		return 7;
	}
	else if (type == "Poison")
	{
		return 8;
	}
	else if (type == "Intimidate")
	{
		return 9;
	}
	else if (type == "AttackUp")
	{
		return 4;
	}
	else if (type == "DefenseUp")
	{
		return 5;
	}
	else
	{
		return 0; // Unknown type
	}
}

void UActionComponent::setType(const FString& newType)
{
	type = newType; 
}

int UActionComponent::getBaseDamage()
{
	return baseDamage;
}
void UActionComponent::setBaseDamage(int newBaseDamage)
{
	baseDamage = newBaseDamage;
}
int UActionComponent::getDuration()
{
	return duration;
}
void UActionComponent::setDuration(int newDuration)
{
	duration = newDuration;
}


UActionComponent::~UActionComponent()
{
}
