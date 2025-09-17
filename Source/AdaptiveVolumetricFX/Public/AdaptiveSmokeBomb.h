/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smoke Bomb")
	TObjectPtr<UTextureRenderTarget2D> SmokeSDFTexture;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smoke Bomb")
	float BoundsSize;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smoke Bomb")
	float InnerRadius;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smoke Bomb")
	float OuterRadius;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smoke Bomb")
	float FactorK;
	
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