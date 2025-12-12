#include "SimulationController.h"
#include  <cmath>

ASimulationController::ASimulationController()
{
    PrimaryActorTick.bCanEverTick = true;

}

void ASimulationController::BeginPlay()
{
    Super::BeginPlay();

    // Checking if the DataTable is assigned
    if (!PopulationDensityEffectTable)
    {
          UE_LOG(LogTemp, Error, TEXT("PopulationDensityEffectTable is not assigned!"));
    }
    else
    {
       // Table found, read data into vector
       ReadDataFromTableToVectors();
    }
}

void ASimulationController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    AccumulatedTime += DeltaTime;

    if (AccumulatedTime >= SimulationStepTime)
    {
        AccumulatedTime = 0.f;
        RunSimulationStep();
        TimeStepsFinished++;
    }  
}

void ASimulationController::RunSimulationStep()
{
    // Calculate auxiliaries
    Bitten = conveyor_content();
    float non_zombie_population = Bitten + Susceptible;
    float population_density = non_zombie_population / land_area;
    float x = population_density / normal_population_density;
    
    float population_density_effect_on_zombie_bites = graph_lookup(x);
    float number_of_bites_per_zombie_per_day = normal_number_of_bites * population_density_effect_on_zombie_bites;
    float total_bitten_per_day = FMath::RoundToFloat(Zombies * number_of_bites_per_zombie_per_day);
    
    float denom = FMath::Max(non_zombie_population, 1.f);
    float number_of_bites_from_total_zombies_on_susceptible = FMath::RoundToFloat((Susceptible / denom) * total_bitten_per_day);
    
    // Enforce non-negative susceptible
    float getting_bitten = FMath::Min(number_of_bites_from_total_zombies_on_susceptible, FMath::Floor(Susceptible));
    
    // Conveyor mechanics
    for (ConveyorBatch &b : conveyor)
    {
        b.remainingDays -= 1.f;
    }
    

    std::vector<ConveyorBatch> next_conveyor;
    float raw_outflow_people = 0.f;
    next_conveyor.reserve(conveyor.size());
    
    for (ConveyorBatch &b : conveyor)
    {
        if (b.remainingDays <= 0.0)
            raw_outflow_people += b.amountOfPeople;
        else
            next_conveyor.push_back(b);
    }
    conveyor.swap(next_conveyor);
    
    // Capacity check for new inflow
    float current_content = conveyor_content();
    float free_cap = FMath::Max(0.f, Bitten_capacity - current_content);
    float inflow_people = FMath::Max(0.f, FMath::Min(getting_bitten, free_cap));
    
    if (inflow_people > 0.f)
        conveyor.push_back(ConveyorBatch{inflow_people, days_to_become_infected_from_bite});
    
    // Convert outflow to zombies
    float becoming_infected = raw_outflow_people * CONVERSION_FROM_PEOPLE_TO_ZOMBIES;
    
    // Update stocks
    Susceptible = FMath::Max(0.f, Susceptible - getting_bitten);
    Zombies = FMath::Max(0.f, Zombies + becoming_infected);
    Bitten = conveyor_content();


    //Check if win or lose.

    if (Susceptible <= 45) {
        Lose();
    }
    if (Zombies <= 0 || TimeStepsFinished > 180 ) {
        Win();
    }
}

float ASimulationController::conveyor_content()
{
    float sum = 0.0;
    for (ConveyorBatch &b : conveyor)
        sum += b.amountOfPeople;
    return sum;
}

float ASimulationController::graph_lookup(float xIn)
{
    if (graphPts.empty()) return 1.0f;
    
    if (xIn <= graphPts.front().first)
        return graphPts.front().second;
    if (xIn >= graphPts.back().first)
        return graphPts.back().second;

    for (size_t i = 1; i < graphPts.size(); ++i)
    {
        if (xIn <= graphPts[i].first)
        {
            float x0 = graphPts[i-1].first, x1 = graphPts[i].first;
            float y0 = graphPts[i-1].second, y1 = graphPts[i].second;
            float t = (xIn - x0) / (x1 - x0);
            return y0 + t*(y1 - y0);
        }
    }
    return graphPts.back().second;
}

// Function to read data from Unreal DataTable into the graphPts vector
void ASimulationController::ReadDataFromTableToVectors()
{
    if (bShouldDebug)
       UE_LOG(LogTemp, Log, TEXT("Read Data From Table To Vectors"))

    TArray<FName> rowNames = PopulationDensityEffectTable->GetRowNames();

    for (int i{0}; i < rowNames.Num(); i++)
    {
       if (bShouldDebug)
          UE_LOG(LogTemp, Log, TEXT("Reading table row index: %d"), i)

       FPopulationDensityEffect* rowData = PopulationDensityEffectTable->FindRow<FPopulationDensityEffect>(rowNames[i], TEXT(""));
       if (rowData)
       {
           graphPts.emplace_back(rowData->PopulationDensity, rowData->NormalPopulationDensity);

           if (bShouldDebug)
           {
               std::pair<float, float> lastPair = graphPts.back();
               UE_LOG(LogTemp, Warning, TEXT("Reading table row: %d, pair: (%f, %f)"), i, lastPair.first, lastPair.second);
           }
       }
    }
}