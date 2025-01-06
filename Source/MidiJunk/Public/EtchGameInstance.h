// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EtchGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class MIDIJUNK_API UEtchGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
    // Constructor or any initialization code you want
    UEtchGameInstance();

    virtual void Init() override;
    virtual void Shutdown() override;

    // Your custom functionality here
};
