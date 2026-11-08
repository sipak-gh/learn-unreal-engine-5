// Chapter 2 example: FString, FName and FText, and converting between them.

#include "ExStringTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogExStrings, Log, All);

// LOCTEXT needs a namespace for the localization keys it creates.
#define LOCTEXT_NAMESPACE "ExStringTypes"

static const TCHAR* ExStrBool(bool bValue)
{
	return bValue ? TEXT("true") : TEXT("false");
}

AExStringTypes::AExStringTypes()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AExStringTypes::BeginPlay()
{
	Super::BeginPlay();

	// FString: a mutable, heap-allocated string for building and editing text.
	const FString Level = TEXT("L_Level01");
	const FString Status = FString::Printf(TEXT("%s has %d gems"), *Level, 3);
	const FString Joined = Level + TEXT(" / ") + FString::FromInt(42);
	UE_LOG(LogExStrings, Log, TEXT("FString: '%s' (Len %d)"), *Status, Status.Len());
	UE_LOG(LogExStrings, Log, TEXT("FString: '%s'"), *Joined);

	// FString == ignores case. Ask for case-sensitive comparison explicitly.
	const FString Upper = Level.ToUpper();
	UE_LOG(LogExStrings, Log, TEXT("FString: == %s, Equals CaseSensitive %s"),
		ExStrBool(Level == Upper), ExStrBool(Level.Equals(Upper, ESearchCase::CaseSensitive)));

	// FName: an immutable, case-insensitive identifier. Comparing two FNames
	// compares two small integers, not the characters.
	const FName GemName(TEXT("Gem"));
	const FName ShoutedName(TEXT("GEM"));
	UE_LOG(LogExStrings, Log, TEXT("FName: Gem == GEM %s, default FName is None %s"),
		ExStrBool(GemName == ShoutedName), ExStrBool(FName().IsNone()));

	// FText: display text for players. It is localizable and formats numbers
	// for the current culture.
	const FText Score = FText::Format(
		LOCTEXT("ScoreFormat", "Score {0}, gems {1}/{2}"),
		FText::AsNumber(12345), FText::AsNumber(2), FText::AsNumber(3));
	UE_LOG(LogExStrings, Log, TEXT("FText: %s"), *Score.ToString());

	// Conversions. FText -> FName is deliberately missing: go through FString.
	const FName NameFromString(*Level);                   // FString -> FName
	const FString StringFromName = GemName.ToString();      // FName -> FString
	const FText TextFromString = FText::FromString(Level);  // FString -> FText
	const FText TextFromName = FText::FromName(GemName);    // FName -> FText
	const FString StringFromText = Score.ToString();        // FText -> FString
	const int32 Parsed = FCString::Atoi(TEXT("42"));        // FString -> int32
	UE_LOG(LogExStrings, Log, TEXT("Convert: %s %s %s %s %d"),
		*NameFromString.ToString(), *StringFromName, *TextFromString.ToString(),
		*TextFromName.ToString(), Parsed);
	UE_LOG(LogExStrings, Log, TEXT("Convert: FText back to FString has Len %d"), StringFromText.Len());
}

#undef LOCTEXT_NAMESPACE
