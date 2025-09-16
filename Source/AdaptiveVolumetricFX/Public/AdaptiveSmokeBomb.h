// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AdaptiveSmokeBomb.generated.h"

USTRUCT(BlueprintType)
struct FExplodeLayer
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> Nodes;
};

UCLASS()
class ADAPTIVEVOLUMETRICFX_API AAdaptiveSmokeBomb : public AActor
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke Bomb")
	TObjectPtr<UInstancedStaticMeshComponent> MeshComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke Bomb")
	float SpreadWeight;
	
	UPROPERTY(BlueprintReadWrite, Category = "Smoke Bomb")
	TArray<FExplodeLayer> ExplodeLayers;
	
public:
	AAdaptiveSmokeBomb();
	
protected:
	virtual void BeginPlay() override;
	
public:
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Smoke Bomb")
	void Explode();
	
protected:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Smoke Bomb")
	void ExplodeWithAnim(int32 LayerIndex);
};