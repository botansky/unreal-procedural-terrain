// Matan Botansky

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "TerrainGenerator.generated.h"



UCLASS()
class PROCEDURALTERRAIN_API ATerrainGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATerrainGenerator();
	void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings") int RandomSeed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings") int Size = 256;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings") float NoiseScale = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings") FVector2D Offset = FVector2D(0.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings") int Octaves = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings") float Persistance = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings") float Lacunarity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Settings") int TerrainHeightFactor = 25;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Settings") int TerrainWidthtFactor = 100;

	TArray<TArray<FVector>> GeneratedNoiseMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) UProceduralMeshComponent* TerrainMesh;


protected:
	//mesh properties
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	TArray<FVector2D> TextureUVs;
	TArray<FColor> TextureColorMap;
	TArray<FLinearColor> HeightColorMap;

public:	
	FVector CalculateAdjustedNoiseValue(float XPosition, float YPosition);
	TArray<TArray<FVector>> GenerateNoiseMap(int Width, int Height, float NoiseScale, int Octaves, float Persistance, float Lacunarity);
	TArray<FColor> CreateTextureColorMap(TArray<TArray<FVector>> NoiseMap);
	void DrawNoiseMapTexture(TArray<TArray<FVector>> NoiseMap);
	void CreateTerrainMesh(TArray<TArray<FVector>> NoiseMap);

};