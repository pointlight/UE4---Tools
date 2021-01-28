// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/TriggerBox.h"
#include "MaterialControlVolume.generated.h"

class UMaterialInstanceDynamic;
struct FCollectionScalarParameter;
struct FCollectionVectorParameter;
struct FTimerHandle;

struct FScalarParameter
{
	FScalarParameter(FName InName, float InBeginValue, float InEndValue)
		: Name(InName), BeginValue(InBeginValue), EndValue(InEndValue)
	{}

	FName Name;
	float BeginValue;
	float EndValue;
};

struct FVectorParameter
{
	FVectorParameter(FName InName, FLinearColor InBeginValue, FLinearColor InEndValue)
		: Name(InName), BeginValue(InBeginValue), EndValue(InEndValue)
	{}

	FName Name;
	FLinearColor BeginValue;
	FLinearColor EndValue;
};

/** For handle parameters of each material, and record the states */
class FParameterHandler
{
private:

	int32 MeshComponentID;

	/**
	 * Parameters of each dynamic material.
	 */
	TMap<TWeakObjectPtr<UMaterialInstanceDynamic>, TArray<FScalarParameter>> ScalarParameters;
	TMap<TWeakObjectPtr<UMaterialInstanceDynamic>, TArray<FVectorParameter>> VectorParameters;

	float BlendWeight;
	bool bInited;
	bool bCompleted;
	bool bEnterOrOut;	/** true表示进入Volume，false表示退出Volume */

public:

	FParameterHandler()
		: BlendWeight(.0f), bInited(false), bCompleted(false)
	{
	}
	
	bool IsCompleted() { return bCompleted; }
	bool IsEnterVolumeType() { return bEnterOrOut; }

	void Init(UMeshComponent* MeshComponent, const TArray<FCollectionScalarParameter>& InScalars, const TArray<FCollectionVectorParameter>& InVectors);
	void AddParameters(UMeshComponent* MeshComponent, const TArray<FCollectionScalarParameter>& InScalars, const TArray<FCollectionVectorParameter>& InVectors);
	void SwapParameters();
	void Update(const float Delta, const float BlendTime);
};

/** An invisible volume used to control material parameters */
UCLASS(hidecategories=(Display, Rendering, Physics))
class GAME_API AMaterialControlVolume : public ATriggerBox
{
	GENERATED_UCLASS_BODY()

private:

	/** The time of blend origin parameter to target. */
	UPROPERTY(EditAnywhere, Category=MaterialParameters)
	float BlendTime = 1;

	/**
	 * For change material property when actor enter this volume.
	 */
	UPROPERTY(EditAnywhere, Category=MaterialParameters)
	TArray<FCollectionScalarParameter> ScalarParameters;

	UPROPERTY(EditAnywhere, Category=MaterialParameters)
	TArray<FCollectionVectorParameter> VectorParameters;

private:

	const float TimerRate = 0.1;

	/** The origin material property of the actor entered this volume. */
	TMap<uint32, TArray<FParameterHandler>> ParameterContainer;

	/** If all element has completed smooth handling, pause tick for optimization. */
	bool bTickPausing;

private:

	//virtual void BeginPlay() override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	/** Set custom parameters to materials. */
	void SetParameters(const uint32 UniqueID, const TArray<UMeshComponent*> MeshComponents);

	/** Restore parameters of materials. */
	void RestoreParameters(const uint32 UniqueID);
};