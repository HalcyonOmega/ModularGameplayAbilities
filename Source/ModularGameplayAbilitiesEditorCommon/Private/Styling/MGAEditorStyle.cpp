// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "Styling/MGAEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

FMGAEditorStyle::FMGAEditorStyle()
	: FSlateStyleSet(TEXT("MGAEditorStyleSet"))
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ModularGameplayAbilities"));
	check(Plugin.IsValid());

	const FString PluginContentDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/Icons")));
	FSlateStyleSet::SetContentRoot(PluginContentDir);

	const FVector2D Icon16X16 = FVector2D(16.0);
	Set(
		"Icons.HeaderView",
		new FSlateVectorImageBrush(FSlateStyleSet::RootToContentDir("BlueprintHeader_16", TEXT(".svg")), Icon16X16)
	);

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

FMGAEditorStyle::~FMGAEditorStyle()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*this);
}
