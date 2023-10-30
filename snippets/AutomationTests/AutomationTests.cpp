DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER

IMPLEMENT_SIMPLE_AUTOMATION_TEST


//In header:
USTRUCT()
struct FTestState
{
    GENERATED_BODY()

public:
    //Success flags
    UPROPERTY()
    bool bFlagOne;
    
    FAutomationTestBase* Test = nullptr;
}

//In CPP:

FATestState testState;

bool UTestHelper::Init(FAutomationTestBase* testBase, const int32 TestFlags)
{
    bool isValid = false;
    UWorld* World = GetGameWorld(TestFlags);

    testState.Test = testBase;
    testState.TestingFlags = TestFlags;

    testState.bFlags = false;

    if (World != nullptr) {
        isValid = true;
    }

    return isValid;
}

UWorld* UTestHelper::GetGameWorld(const int32 TestFlags)
{
    if (((TestFlags & EAutomationTestFlags::EditorContext) || (TestFlags & EAutomationTestFlags::ClientContext)) == false)
    {
        return nullptr;
    }
    if (GEngine->GetWorldContexts().Num() == 0)
    {
        return nullptr;
    }
    if (GEngine->GetWorldContexts()[0].WorldType != EWorldType::Game && GEngine->GetWorldContexts()[0].WorldType != EWorldType::Editor)
    {
        return nullptr;
    }

    if (GEngine != nullptr)
    {
        const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();

        for (const FWorldContext& Context : WorldContexts)
        {
            if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
            {
                return Context.World();
            }
        }
    }

    return nullptr;
}

//Get the player character
AProjectCharacter* UTestHelper::GetPlayer()
{
    TArray<AActor*> players;
    AProjectCharacter* player = nullptr;
    UGameInstance* gameInst = UGameInstance::GetInstance(GetGameWorld(testState.TestingFlags));

    if (gameInst != nullptr) {
        UWorld* const World = gameInst->GetWorld();

        UGameplayStatics::GetAllActorsOfClass(World, AProjectCharacter::StaticClass(), players);
        for (AActor* actor : players)
        {
            player = Cast<AProjectCharacter>(actor);
            break;
        }
    }

    return player;
}

//Check the level name is not a specific level, ie the Menu.
bool UTestHelper::CheckUnexpectedLevel(FName levelName) {
    UGameInstance* gameInst = UGameInstance::GetInstance(GetGameWorld(testState.TestingFlags));
    UWorld* const World = gameInst->GetWorld();
    FString CurrentLevel = World->GetMapName();

    if (CurrentLevel == levelName) {
        UE_LOG(LogTemp, Error, TEXT("Unexpected returned to main menu."));

        return true;
    }

    return false;
}

//Open a game level by a specific level name
void UTestHelper::OpenLevel(FName levelName)
{
    UGameInstance* gameInst = UGameInstance::GetInstance(GetGameWorld(testState.TestingFlags));

    //Setup your game
	gameInst->SetNewSaveFile(false);
	gameInst->SetGameMode(EGameMode::STANDARD);

    //Open the level
    UGameplayStatics::OpenLevel(gameInst->GetWorld(), levelName, true);

    gameInst->SetCompleted(EFlag::Something);
}


#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#define AUTOMATION_TEST_FILTER EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter

//DEFINE TESTS

static FString MainMapName = "Main";

//Create a simple test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestSomethingSpecific, "Project.Tests.SimpleTest", AUTOMATION_TEST_FILTER)
bool FTestSomethingSpecific::RunTest(const FString& Parameters)
{
    UTestHelper* Helper = NewObject<UTestHelper>();

    if (Helper->Init(this, GetTestFlags())) {
        //Check the game instance has loaded
        ADD_LATENT_AUTOMATION_COMMAND(FSomethingToTest(Helper));
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
        //Load the level
        ADD_LATENT_AUTOMATION_COMMAND(FInitLevel(Helper, FName(*MapNaMainMapNameme)));
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
        //Check the playable character has loaded
        ADD_LATENT_AUTOMATION_COMMAND(FTestPlayerCharacter(Helper));

        //Additional tests
    }

    return true;
}

//DEFINE COMMAND FUNCTIONS

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FSomethingToTest, UTestHelper*, Helper);
bool FSomethingToTest::Update()
{
    UGameInstance* gameInst = UGameInstance::GetInstance(Helper->GetGameWorld(testState.TestingFlags));

    if (gameInst != nullptr) {
        isSomethingReady = gameInst->GetSomething();

		if (isSomethingReady)
		{
			testState.Test->TestTrue(TEXT("Get Ready"), isSomethingReady);

			isDataMatch = gameInst->GetInfo().version == gameInst->GetVersion();

			testState.Test->TestTrue(TEXT("Is Matching"), isDataVersionsMatching);

			if (!isDataMatch) {
				testState.bTimeout = true;

				return true;
			}
		}
    }

    return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FInitLevel, UTestHelper*, Helper, FName, MapName);
bool FInitLevel::Update()
{
	Helper->OpenLevel(MapName);

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FTestPlayerCharacter, UTestHelper*, Helper);
bool FTestPlayerCharacter::Update()
{
    UGameInstance* gameInst = UGameInstance::GetInstance(Helper->GetGameWorld(testState.TestingFlags));
    AProjectCharacter* player = Helper->GetPlayer();

	if ((gameInst != nullptr && !Helper->CheckTimeElapsed()) || testState.bTimeout) 
	{
		testState.bTimeout = true;

		return true;
	}

    if (gameInst != nullptr) {
		if (player != nullptr)
		{
			bool isPlayerSomething = player->GetPlayerState();

			testState.Test->TestTrue(TEXT("Player is ready"), isPlayerSomething);

			if (isPlayerSomething) {
				return true;
			}
		}
    }

    return false;
}