// GridSnake - course project for "Learn Unreal Engine 5".

#include "Data/GSLevelList.h"

int32 UGSLevelList::FindLevelIndex(FName LevelId) const
{
	return Levels.IndexOfByPredicate([LevelId](const FGSLevelInfo& Level)
	{
		return Level.GetLevelId() == LevelId;
	});
}

const FGSLevelInfo* UGSLevelList::GetLevel(int32 Index) const
{
	return Levels.IsValidIndex(Index) ? &Levels[Index] : nullptr;
}
