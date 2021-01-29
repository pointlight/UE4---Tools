// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MaterialControlVolume.h"
#include "Components/BrushComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Utils/ActorRenderUtilities.h"


void FParameterHandler::Init(UMeshComponent* MeshComponent, const TArray<FScalarParameter>& InScalars, const TArray<FVectorParameter>& InVectors)
{
	if (MeshComponent == nullptr || (InScalars.Num() <= 0 && InVectors.Num() <= 0))
		return;

	if (bInited)
	{
		SwapParameters();
	}
	else
	{
		// Add new datas.
		MeshComponentID = MeshComponent->GetUniqueID();
		AddParameters(MeshComponent, InScalars, InVectors);
	}

	bInited = true;
	bCompleted = false;
	bEnterOrOut = true;
}

void FParameterHandler::AddParameters(UMeshComponent* MeshComponent, const TArray<FScalarParameter>& InScalars, const TArray<FVectorParameter>& InVectors)
{
	const TArray<UMaterialInterface*> MaterialInterfaces = MeshComponent->GetMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialInterfaces.Num(); ++MaterialIndex)
	{
		// Check.
		UMaterialInterface* MaterialInterface = MaterialInterfaces[MaterialIndex];
		if (!MaterialInterface) continue;

		// Add new handler to handle parameters of this matzheerial.
		for (int i = 0; i < InScalars.Num(); ++i)
		{
			float OutValue;
			if (!MaterialInterface->GetScalarParameterValue(
				FMaterialParameterInfo(InScalars[i].ParameterName), OutValue, !InScalars[i].bForceChange))
			{
				continue;
			}

			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
			if (!DynamicMaterial)
			{
				DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
			}

			auto& Scalars = ScalarParameters.FindOrAdd(DynamicMaterial);
			FScalarParameterData Parameter(InScalars[i].ParameterName, OutValue, InScalars[i].DefaultValue);
			if (Scalars.IsValidIndex(i))
				Scalars[i] = Parameter;
			else
				Scalars.Add(Parameter);
		}

		for (int i = 0; i < InVectors.Num(); ++i)
		{
			FLinearColor OutValue;
			if (!MaterialInterface->GetVectorParameterValue(
				FMaterialParameterInfo(InVectors[i].ParameterName), OutValue, !InVectors[i].bForceChange))
			{
				continue;
			}

			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
			if (!DynamicMaterial)
			{
				DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
			}

			auto& Vector = VectorParameters.FindOrAdd(DynamicMaterial);
			FVectorParameterData Parameter(InVectors[i].ParameterName, OutValue, InVectors[i].DefaultValue);
			if (Vector.IsValidIndex(i))
				Vector[i] = Parameter;
			else
				Vector.Add(Parameter);
		}
	}
}

void FParameterHandler::SwapParameters()
{
	for (auto& ScalarPair : ScalarParameters)
	{
		for (auto& Scalar : ScalarPair.Value)
		{
			float Temp = Scalar.BeginValue;
			Scalar.BeginValue = Scalar.EndValue;
			Scalar.EndValue = Temp;
		}
	}

	for (auto& VectorPair : VectorParameters)
	{
		for (auto& Vector : VectorPair.Value)
		{
			FLinearColor Temp = Vector.BeginValue;
			Vector.BeginValue = Vector.EndValue;
			Vector.EndValue = Temp;
		}
	}

	BlendWeight = (1 - BlendWeight);
	bCompleted = false;
	bEnterOrOut = false;
}

void FParameterHandler::Update(const UCurveFloat* Curve, const float Delta, const float BlendTime)
{
	check(Curve);

	if (bCompleted)
		return;

	BlendWeight += (Delta / BlendTime);
	if (BlendWeight >= 1)
	{
		bCompleted = true;
		BlendWeight = 1;
	}

	float lerpValue = Curve->GetFloatValue(BlendWeight);
	for (const auto& ScalarPair : ScalarParameters)
	{
		auto& DynamicMaterial = ScalarPair.Key;
		if (!DynamicMaterial.IsValid())
			continue;

		for (const auto& Scalar : ScalarPair.Value)
		{
			DynamicMaterial->SetScalarParameterValue(Scalar.Name, FMath::Lerp(Scalar.BeginValue, Scalar.EndValue, lerpValue));
		}
	}
	for (const auto& VectorPair : VectorParameters)
	{
		auto& DynamicMaterial = VectorPair.Key;
		if (!DynamicMaterial.IsValid())
			continue;

		for (const auto& Vector : VectorPair.Value)
		{
			DynamicMaterial->SetVectorParameterValue(Vector.Name, FMath::Lerp(Vector.BeginValue, Vector.EndValue, lerpValue));
		}
	}
}


AMaterialControlVolume::AMaterialControlVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bTickPausing(true)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
}

//void AMaterialControlVolume::BeginPlay()
//{
//	Super::BeginPlay();
//}

void AMaterialControlVolume::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (bTickPausing) return;
	
	// 刷新各角色材质的参数
	bool bAllOut = true;
	bool bAllCompleted = true;
	for (auto& HandlerPairs : ParameterContainer)
	{
		bool bCompleted = true;
		bool bOut = true;

		for (auto& Handler : HandlerPairs.Value)
		{
			UCurveFloat* Curve;
			if (Handler.IsEnterVolumeType())
			{
				Curve = EnterCurve;
			}
			else
			{
				Curve = OutCurve;
			}
			Handler.Update(Curve, DeltaTime, BlendTime);

			// 刷新标志位
			if (!Handler.IsCompleted())
			{
				bCompleted = false;
				bAllCompleted = false;
			}
			if (Handler.IsEnterVolumeType())
			{
				bOut = false;
				bAllOut = false;
			}
		}

		// 如果该角色离开了Volume，并且其材质参数完成平滑渐变，则清除该角色的缓存数据
		if (bOut && bCompleted)
		{
			HandlerPairs.Value.Reset();
		}
	}

	// 所有角色都离开Volume，并且其材质都完成平滑渐变，则清除所有缓存数据
	if (bAllCompleted && bAllOut)
	{
		ParameterContainer.Reset();
	}

	bTickPausing = bAllCompleted;
}

void AMaterialControlVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// Check.
	TArray<UMeshComponent*> OutComponents;
	UActorRenderUtilities::GetActorComponents<UMeshComponent>(OtherActor, OutComponents);
	if (OutComponents.Num() <= 0)
	{
		return;
	}
	
	// Change parameters.
	SetParameters(OtherActor->GetUniqueID(), OutComponents);
}

void AMaterialControlVolume::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (ScalarParameters.Num() <= 0 && VectorParameters.Num() <= 0) return;

	// Restore parameters.
	RestoreParameters(OtherActor->GetUniqueID());
}

void AMaterialControlVolume::SetParameters(const uint32 UniqueID, const TArray<UMeshComponent*> MeshComponents)
{
	auto& Handlers = ParameterContainer.FindOrAdd(UniqueID);
	for (int i = 0; i < MeshComponents.Num(); ++i)
	{
		if (!Handlers.IsValidIndex(i))
		{
			Handlers.Add(FParameterHandler());
		}
		Handlers[i].Init(MeshComponents[i], ScalarParameters, VectorParameters);
	}

	bTickPausing = false;
}

void AMaterialControlVolume::RestoreParameters(const uint32 UniqueID)
{
	auto* HandlerPairs = ParameterContainer.Find(UniqueID);
	if (!HandlerPairs || HandlerPairs->Num() <= 0)
	{
		return;
	}

	for (auto& Handler : (*HandlerPairs))
	{
		Handler.SwapParameters();
	}

	bTickPausing = false;
}