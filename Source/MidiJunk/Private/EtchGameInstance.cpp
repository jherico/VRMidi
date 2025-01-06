// Fill out your copyright notice in the Description page of Project Settings.


#include "EtchGameInstance.h"
#include "TribeMIDISubsystem.h"

UEtchGameInstance::UEtchGameInstance()
{
    // Constructor: You can initialize custom properties here
}

void UEtchGameInstance::Init()
{
    Super::Init();


    // Access your subsystem instance
    UTribeMIDISubsystem* midiSubsystem = GetSubsystem<UTribeMIDISubsystem>();

    if (midiSubsystem)
    {
        // Call functions or interact with the subsystem
        UE_LOG(LogTemp, Log, TEXT("UTribeMIDISubsystem initialized"));
    }

    // Add any custom initialization logic for your game instance
}

void UEtchGameInstance::Shutdown()
{
    Super::Shutdown();

    // Add any cleanup logic when the game instance is shut down
}