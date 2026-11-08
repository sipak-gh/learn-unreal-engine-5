// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSSettings.h"
#include "GridSnake.h"
#include "Data/GSLevelList.h"

const UGSLevelList* UGSSettings::GetLevelList()
{
	const UGSLevelList* List = Get()->LevelList.LoadSynchronous();
	if (!List)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("No level list - set Project Settings > Game > GridSnake > Level List"));
	}
	return List;
}
