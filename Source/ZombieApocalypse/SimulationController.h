// Copyright University of Inland Norway

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include <vector>
#include "SimulationController.generated.h"


// Struct for the Unreal DataTable
USTRUCT(BlueprintType)
struct FPopulationDensityEffect : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PopulationDensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NormalPopulationDensity;
};

// Conveyor system for bitten people
struct ConveyorBatch
{
	float amountOfPeople;
	float remainingDays;
};

UCLASS()
class ZOMBIEAPOCALYPSE_API ASimulationController : public AActor
{
	GENERATED_BODY()
	
public:	
	ASimulationController();

	// Runs one simulation step each Tick 
	virtual void Tick(float DeltaTime) override;

	// Function to read data from Unreal DataTable into the graphPts vector
	void ReadDataFromTableToVectors();


	//Win or loss trigger functions;
	UFUNCTION(BlueprintImplementableEvent, Category = "EndGameEvents")
	void Lose();
	UFUNCTION(BlueprintImplementableEvent, Category = "EndGameEvents")
	void Win();


	// Unreal Lookup table for population density effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	class UDataTable* PopulationDensityEffectTable{ nullptr };

	// How long each time step is in Unreal, in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float SimulationStepTime{ 1.f };

	// Turn on/off debug printing to the Output Log
	UPROPERTY(EditAnywhere, Category = "Simulation Variables")
	bool bShouldDebug{ false };


	// Stocks (initial)

	//Susceptible (People) - choose a number that has a clean sqrt!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float Susceptible{ 100.f };
	// Zombies = patient_zero
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float Zombies{ 1.f };		
	// Just to check if we are correctly updating stocks - used in SimulationHUD  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float Bitten{ 0.f };          // s Bitten -> in BittenArraySize

	// ----- CONSTANTS / INITIALIZATION -----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float days_to_become_infected_from_bite{15.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")  
	float Bitten_capacity{100.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float patient_zero{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float CONVERSION_FROM_PEOPLE_TO_ZOMBIES{1.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float normal_number_of_bites{1.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float land_area{1000.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Variables")
	float normal_population_density{0.1f};
	
	// GRAPH points: population_density_effect_on_zombie_bites
	// Values read in from Unreal DataTable for more flexibility
	std::vector<std::pair<float, float>> graphPts;
		//= {
		//{0.000f, 0.014f}, {0.200f, 0.041f}, {0.400f, 0.101f}, {0.600f, 0.189f}, {0.800f, 0.433f},
		//{1.000f, 1.000f}, {1.200f, 1.217f}, {1.400f, 1.282f}, {1.600f, 1.300f}, {1.800f, 1.300f},
		//{2.000f, 1.300f}
		//};

	// Time accumulator for simulation steps, used in Tick function
	float AccumulatedTime{ 0.f };

	// Number of time steps completed - to keep track and compare to Stella
	int TimeStepsFinished{ 0 };

	std::vector<ConveyorBatch> conveyor;

	void RunSimulationStep();

protected:
	virtual void BeginPlay() override;

private:
	float conveyor_content();
	float graph_lookup(float xIn);

};
