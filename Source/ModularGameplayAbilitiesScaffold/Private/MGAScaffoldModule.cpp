// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#include "MGAScaffoldModule.h"

#include "ContentBrowserModule.h"
#include "MGAScaffoldPreviewSettings.h"
#include "ToolMenus.h"
#include "Attributes/MGAAttributeSetBlueprint.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IMainFrameModule.h"
#include "Slate/SMGAAttributeSetWizard.h"
#include "Styling/MGAEditorStyle.h"
#include "Styling/SlateStyle.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/AssetEditorToolkitMenuContext.h"

#define LOCTEXT_NAMESPACE "FMGAScaffoldModule"

FTextBlockStyle FMGAScaffoldModule::HeaderViewTextStyle;
FTableRowStyle FMGAScaffoldModule::HeaderViewTableRowStyle;

void FMGAScaffoldModule::StartupModule()
{
	// Setup preview style (colors, etc.) from settings
	{
		// TODO: Custom Dev Settings - UBlueprintHeaderViewSettings updates static FBlueprintHeaderViewModule::HeaderViewTextStyle on property change
		HeaderViewTextStyle = FTextBlockStyle()
			.SetFont(FCoreStyle::GetDefaultFontStyle("Mono", GetDefault<UMGAScaffoldPreviewSettings>()->FontSize))
			.SetColorAndOpacity(FSlateColor::UseForeground());

		HeaderViewTableRowStyle = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");
		HeaderViewTableRowStyle.ActiveBrush.TintColor = GetDefault<UMGAScaffoldPreviewSettings>()->SelectionColor;
		HeaderViewTableRowStyle.ActiveHoveredBrush.TintColor = GetDefault<UMGAScaffoldPreviewSettings>()->SelectionColor;
	}
	
	SetupAssetEditorMenuExtender();
	SetupContentBrowserContextMenuExtender();
}

void FMGAScaffoldModule::ShutdownModule()
{
	if (ContentBrowserExtenderDelegateHandle.IsValid())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		CBMenuExtenderDelegates.RemoveAll([ContentBrowserExtenderDelegateHandle = ContentBrowserExtenderDelegateHandle](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
		{
			return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle;
		});
	}
}

bool FMGAScaffoldModule::IsClassHeaderViewSupported(const UClass* InClass)
{
	return InClass == UMGAAttributeSetBlueprint::StaticClass();
}

void FMGAScaffoldModule::OpenHeaderViewForAsset(const FAssetData InAssetData)
{
	const FMGAAttributeWindowArgs Args;
	CreateAttributeWizardWindow(InAssetData, Args);
}

TSharedRef<SWindow> FMGAScaffoldModule::CreateAttributeWizard(const FAssetData& InAssetData, const FMGAAttributeWindowArgs& InArgs) const
{
	return CreateAttributeWizardWindow(InAssetData, InArgs);
}

TSharedRef<SWindow> FMGAScaffoldModule::CreateAttributeWizardWindow(const FAssetData& InAssetData, const FMGAAttributeWindowArgs& InArgs)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(InArgs.Title)
		.ClientSize(InArgs.Size)
		.SizingRule(InArgs.SizingRule)
		.MinWidth(InArgs.Size.X * 0.8)
		.MinHeight(InArgs.Size.Y * 0.8)
		.SupportsMinimize(InArgs.bSupportsMinimize)
		.SupportsMaximize(InArgs.bSupportsMaximize);

	const TSharedRef<SMGAAttributeSetWizard> Wizard = SNew(SMGAAttributeSetWizard, InAssetData);
	Window->SetContent(Wizard);

	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		const IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	if (InArgs.bIsModal)
	{
		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
	}
	else if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(Window, ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}

	return Window;
}

void FMGAScaffoldModule::SetupAssetEditorMenuExtender()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Asset");
	FToolMenuSection& Section = Menu->FindOrAddSection("HeaderViewActions");
	
	Section.AddDynamicEntry("BlueprintHeaderViewCommands", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
	{
		const UAssetEditorToolkitMenuContext* MenuContext = InSection.FindContext<UAssetEditorToolkitMenuContext>();
		if (MenuContext && MenuContext->Toolkit.IsValid())
		{
			const TSharedPtr<FAssetEditorToolkit> Toolkit = MenuContext->Toolkit.Pin();
			if (Toolkit->IsActuallyAnAsset() && Toolkit->GetObjectsCurrentlyBeingEdited()->Num() == 1)
			{
				for (const UObject* EditedObject : *Toolkit->GetObjectsCurrentlyBeingEdited())
				{
					if (IsClassHeaderViewSupported(EditedObject->GetClass()))
					{
						const FAssetData BlueprintAssetData(EditedObject);

						InSection.AddMenuEntry(
							FName("OpenHeaderView"),
							LOCTEXT("OpenAssetHeaderView", "Preview Equivalent C++ Header"),
							LOCTEXT("OpenAssetHeaderViewTooltip", "Provides a preview of what this class could look like in C++"),
							FSlateIcon(FMGAEditorStyle::Get().GetStyleSetName(), "Icons.HeaderView"),
							FUIAction(FExecuteAction::CreateStatic(&FMGAScaffoldModule::OpenHeaderViewForAsset, BlueprintAssetData))
						);
					}
				}
			}
		}
	}));
}

void FMGAScaffoldModule::SetupContentBrowserContextMenuExtender()
{
	// TODO: That could be handled via new UToolMenus API too
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	CBMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FMGAScaffoldModule::OnExtendContentBrowserAssetSelectionMenu));
	ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
}

TSharedRef<FExtender> FMGAScaffoldModule::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	if (SelectedAssets.Num() == 1)
	{
		if (IsClassHeaderViewSupported(SelectedAssets[0].GetClass()))
		{

			Extender->AddMenuExtension(
				"GetAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateLambda([SelectedAssets](FMenuBuilder& MenuBuilder)
				{
					MenuBuilder.AddMenuEntry(
						LOCTEXT("OpenHeaderView", "Preview Equivalent C++ Header"),
						LOCTEXT("OpenHeaderViewTooltip", "Provides a preview of what this class could look like in C++"),
						FSlateIcon(FMGAEditorStyle::Get().GetStyleSetName(), "Icons.HeaderView"),
						FUIAction(FExecuteAction::CreateStatic(&FMGAScaffoldModule::OpenHeaderViewForAsset, SelectedAssets[0]))
					);
				})
			);
		}
	}

	return Extender;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMGAScaffoldModule, ModularGameplayAbilitiesScaffold)
