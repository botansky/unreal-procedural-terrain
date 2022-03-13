// Matan Botansky

#include "TerrainGenerator.h"
#include "ImageUtils.h"
#include "AssetRegistryModule.h"
#include <limits>
#include "Kismet/KismetMathLibrary.h"
#include "Components/MeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h" 
#include "Math/UnrealMathUtility.h"
#include "Math/Color.h"

// Sets default values
ATerrainGenerator::ATerrainGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//TerrainMesh = NewObject<UProceduralMeshComponent>(this, TEXT("TerrainMesh"));
	TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
	RootComponent = TerrainMesh;


}

void ATerrainGenerator::OnConstruction(const FTransform& Transform) {
	//force square noisemap
	int Width = Size;
	int Height = Size;

	//generate a noise map the size of the map
	GeneratedNoiseMap = GenerateNoiseMap(Width, Height, NoiseScale, Octaves, Persistance, Lacunarity);
	CreateTerrainMesh(GeneratedNoiseMap);

	DrawNoiseMapTexture(GeneratedNoiseMap);
}

FVector ATerrainGenerator::CalculateAdjustedNoiseValue(float XPosition, float YPosition)
{
	float Perlin = FMath::PerlinNoise2D(FVector2D(XPosition, YPosition));
	float PerlinAlt = FMath::PerlinNoise2D(FVector2D(XPosition+1, YPosition+1)); //next seed
	float XValue = FMath::Abs(Perlin) * 0.8f + Perlin * 0.2f;
	float YValue = FMath::Abs(Perlin) * 0.2f + Perlin * 0.8f;
	float ZValue = FMath::Abs(PerlinAlt) * 0.8f + Perlin * 0.2f;

	return FVector(XValue, YValue, ZValue);
}

//generate a nested array of floats of set parameters
TArray<TArray<FVector>> ATerrainGenerator::GenerateNoiseMap(int NoiseWidth, int NoiseHeight, float Scale,
	int NoiseOctaves, float NoisePersistance, float NoiseLacunarity)
{
	TArray<TArray<FVector>> NoiseMap;
	NoiseMap.AddDefaulted(NoiseWidth);

	//use these to find min and max noise values to normalize the noise between 0 and 1 later
	float XMinNoiseHeight = std::numeric_limits<float>::max();
	float XMaxNoiseHeight = std::numeric_limits<float>::min();
	float YMinNoiseHeight = std::numeric_limits<float>::max();
	float YMaxNoiseHeight = std::numeric_limits<float>::min();
	float ZMinNoiseHeight = std::numeric_limits<float>::max();
	float ZMaxNoiseHeight = std::numeric_limits<float>::min();

	TArray<FVector> NoiseColumn; //an array for each column of noise points
	for (int x = 0; x < NoiseWidth; x++) {
		NoiseColumn.AddDefaulted(NoiseHeight);
		for (int y = 0; y < NoiseHeight; y++)
		{
			float Amplitude = 1.0f;
			float Frequency = 1.0f;
			FVector PointHeight = FVector(0.0f, 0.0f, 0.0f);

			if (Scale <= 0.0f) {
				Scale = 0.001f;
			}

			for (int o = 0; o < Octaves; o++) {
				float ScaledX = (x + Offset.X * NoiseWidth) / (Scale * Frequency);
				float ScaledY = (y + Offset.Y * NoiseHeight) / (Scale * Frequency);

				int HighestSeed = std::numeric_limits<int>::max(); //get the most possible seeds
				FVector NoiseValue = CalculateAdjustedNoiseValue(ScaledX + (RandomSeed % HighestSeed), ScaledY + (RandomSeed % HighestSeed));

				PointHeight += Amplitude * NoiseValue;
				
				Amplitude *= NoisePersistance;
				Frequency *= NoiseLacunarity;

			}

			if (PointHeight.X > XMaxNoiseHeight) {
				XMaxNoiseHeight = PointHeight.X;
			} else if (PointHeight.X < XMinNoiseHeight) {
				XMinNoiseHeight = PointHeight.X;
			}
			if (PointHeight.Y > YMaxNoiseHeight) {
				YMaxNoiseHeight = PointHeight.Y;
			}
			else if (PointHeight.Y < YMinNoiseHeight) {
				YMinNoiseHeight = PointHeight.Y;
			}
			if (PointHeight.Z > ZMaxNoiseHeight) {
				ZMaxNoiseHeight = PointHeight.Z;
			}
			else if (PointHeight.Z < ZMinNoiseHeight) {
				ZMinNoiseHeight = PointHeight.Z;
			}

			NoiseColumn[y] = PointHeight;
		}

		NoiseMap[x] = NoiseColumn; //add column to nested array
	}

	//normalize values
	for (int x = 0; x < NoiseWidth; x++) {
		for (int y = 0; y < NoiseHeight; y++)
		{
			NoiseMap[x][y].X = UKismetMathLibrary::NormalizeToRange(NoiseMap[x][y].X, XMinNoiseHeight, XMaxNoiseHeight);
			NoiseMap[x][y].Y = UKismetMathLibrary::NormalizeToRange(NoiseMap[x][y].Y, YMinNoiseHeight, YMaxNoiseHeight);
			NoiseMap[x][y].Z = UKismetMathLibrary::NormalizeToRange(NoiseMap[x][y].Z, ZMinNoiseHeight, ZMaxNoiseHeight);
		}
	}

	return NoiseMap; //VECTORS MUST NOT BE NORMALIZED!!
}

TArray<FColor> ATerrainGenerator::CreateTextureColorMap(TArray<TArray<FVector>> NoiseMap)
{

	//get dimensions of noise map
	int TextureHeight = NoiseMap[0].Num();
	int TextureWidth = NoiseMap.Num();

	//set up texture colourmap
	TextureColorMap.Empty();
	for (int x = 0; x < TextureWidth; x++) {
		for (int y = 0; y < TextureHeight; y++)
		{
			TextureColorMap.Add(FLinearColor(NoiseMap[x][y]).ToFColor(false));
		}
	}

	return TextureColorMap;
}

void ATerrainGenerator::DrawNoiseMapTexture(TArray<TArray<FVector>> NoiseMap)
{
	UTexture2D* Texture;

	//get dimensions of noise map
	int TextureHeight = NoiseMap[0].Num();
	int TextureWidth = NoiseMap.Num();
	TArray<FColor> ColorMap = CreateTextureColorMap(NoiseMap);

	//create and save package
	UPackage* TexturePackage = CreatePackage(TEXT("/Game/Textures/NoiseTexture"));
	TexturePackage->FullyLoad();
	Texture = FImageUtils::CreateTexture2D(
		TextureWidth, TextureHeight, ColorMap, TexturePackage, TEXT("NoiseTexture"), RF_Public | RF_Standalone, FCreateTexture2DParameters());
	TexturePackage->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Texture);
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		TEXT("/Game/Textures/"), FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(TexturePackage, Texture, RF_Public | RF_Standalone, *PackageFileName);
	
	//get the material and set new texture
	if (TerrainMesh) {
		UMaterial* MeshMaterial = TerrainMesh->GetMaterial(0)->GetMaterial();
		MeshMaterial->SetTextureParameterValueEditorOnly(TEXT("NoiseTexture"), Texture);
		MeshMaterial->PostEditChange();
	}
}

void ATerrainGenerator::CreateTerrainMesh(TArray<TArray<FVector>> NoiseMap)
{
	/* NoiseMap Vector Description:
		X-Component -> Adjusted Mesh Magnitude Value
		Y-Component -> Adjusted Mesh Non-Magnitude Value
		Z-Component -> Alternative Noise Adjusted Mesh Magnitude Value (Different Seed)
	*/

	//clear all declared arrays
	Vertices.Empty();
	Triangles.Empty();
	Normals.Empty();
	Tangents.Empty();
	HeightColorMap.Empty();
	TextureUVs.Empty();

	//get dimensions of noise map
	int NoiseHeight = NoiseMap[0].Num();
	int NoiseWidth = NoiseMap.Num();

	//set up linear color map and vertex array for each noisemap point
	for (int x = 0; x < NoiseWidth; x++)
	{
		for (int y = 0; y < NoiseHeight; y++) {
			int VertexScale = 1.0f;
			HeightColorMap.Add(FLinearColor::LerpUsingHSV(FColor::Black, FColor::White, NoiseMap[x][y].X));
			Vertices.Add(FVector((float)x - NoiseWidth/2.0f, (float)y - NoiseHeight/2.0f, NoiseMap[x][y].X * TerrainHeightFactor) * TerrainWidthtFactor); //assign adjusted mesh magnitude value to each vertex
			TextureUVs.Add(FVector2D((float)y /NoiseHeight , (float)x / NoiseWidth));
		}
	}

	//add triangles to buffer for all vertices except for those in the last column
	for (int i = 0; i < (NoiseHeight * (NoiseWidth - 1)); i++)
	{
		if ( (i+1) % NoiseHeight > 0) {
			Triangles.Add(i);
			Triangles.Add(i + 1);
			Triangles.Add(i + NoiseHeight);

			Triangles.Add(i + 1);
			Triangles.Add(i + NoiseHeight + 1);
			Triangles.Add(i + NoiseHeight);
		}
	}

	//set normals and tangents
	Normals.Add(FVector(1, 0, 0));
	Normals.Add(FVector(1, 0, 0));
	Normals.Add(FVector(1, 0, 0));
	Tangents.Add(FProcMeshTangent(0, 1, 0));
	Tangents.Add(FProcMeshTangent(0, 1, 0));
	Tangents.Add(FProcMeshTangent(0, 1, 0));


	if (TerrainMesh) {
		TerrainMesh->bUseAsyncCooking = true;
		if (TerrainMesh->GetNumSections() == 0) {
			TerrainMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, TextureUVs, HeightColorMap, Tangents, true);
		}
		else {
			TerrainMesh->UpdateMeshSection_LinearColor(0, Vertices, Normals, TextureUVs, HeightColorMap, Tangents);
		}
		TerrainMesh->ContainsPhysicsTriMeshData(true);
	}
}

