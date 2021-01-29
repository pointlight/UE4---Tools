// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/TriggerBox.h"
#include "MaterialControlVolume.generated.h"

class UMaterialInstanceDynamic;
class UCurveFloat;
struct FTimerHandle;


/** Base struct for collection parameters */
USTRUCT()
struct FParameterBase
{
	GENERATED_USTRUCT_BODY()

	FParameterBase()
	{
		FPlatformMisc::CreateGuid(Id);
	}

	/** The name of the parameter.  Changing this name will break any blueprints that reference the parameter. */
	UPROPERTY(EditAnywhere, Category=Parameter)
	FName ParameterName;

	/** Force to change parameter no matter it enabled or not. */
	UPROPERTY(EditAnywhere, Category=Parameter)
	bool bForceChange;

	/** Uniquely identifies the parameter, used for fixing up materials that reference this parameter when renaming. */
	UPROPERTY(meta = (IgnoreForMemberInitializationTest))
	FGuid Id;
};

/** A scalar parameter */
USTRUCT()
struct FScalarParameter : public FParameterBase
{
	GENERATED_USTRUCT_BODY()

	FScalarParameter()
		: DefaultValue(0.0f)
	{
		ParameterName = FName(TEXT("Scalar"));
	}

	UPROPERTY(EditAnywhere, Category=Parameter)
	float DefaultValue;
};

/** A vector parameter */
USTRUCT()
struct FVectorParameter : public FParameterBase
{
	GENERATED_USTRUCT_BODY()

	FVectorParameter()
		: DefaultValue(ForceInitToZero)
	{
		ParameterName = FName(TEXT("Vector"));
	}

	UPROPERTY(EditAnywhere, Category=Parameter)
	FLinearColor DefaultValue;
};

struct FScalarParameterData
{
	FScalarParameterData(FName InName, float InBeginValue, float InEndValue)
		: Name(InName), BeginValue(InBeginValue), EndValue(InEndValue)
	{}

	FName Name;
	float BeginValue;
	float EndValue;
};

struct FVectorParameterData
{
	FVectorParameterData(FName InName, FLinearColor InBeginValue, FLinearColor InEndValue)
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
	TMap<TWeakObjectPtr<UMaterialInstanceDynamic>, TArray<FScalarParameterData>> ScalarParameters;
	TMap<TWeakObjectPtr<UMaterialInstanceDynamic>, TArray<FVectorParameterData>> VectorParameters;

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

	void Init(UMeshComponent* MeshComponent, const TArray<FScalarParameter>& InScalars, const TArray<FVectorParameter>& InVectors);
	void AddParameters(UMeshComponent* MeshComponent, const TArray<FScalarParameter>& InScalars, const TArray<FVectorParameter>& InVectors);
	void SwapParameters();
	void Update(const UCurveFloat* Curve, const float Delta, const float BlendTime);
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
	 * Change curve of material parameters.
	 */
	UPROPERTY(EditAnywhere, Category=MaterialParameters)
	UCurveFloat* EnterCurve;

	UPROPERTY(EditAnywhere, Category=MaterialParameters)
	UCurveFloat* OutCurve;

	/**
	 * For change material property when actor enter this volume.
	 */
	UPROPERTY(EditAnywhere, Category=MaterialParameters)
	TArray<FScalarParameter> ScalarParameters;

	UPROPERTY(EditAnywhere, Category=MaterialParameters)
	TArray<FVectorParameter> VectorParameters;

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