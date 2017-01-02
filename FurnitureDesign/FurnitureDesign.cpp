
#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>

#include <codecvt>
#include <map>


using namespace adsk::core;
using namespace adsk::fusion;

Ptr<Application> _app;
Ptr<UserInterface> _ui;

namespace
{
	const std::string c_wardrobeWidthInputId = "WardrobeWidthInput";
	const std::string c_wardrobeHeightInputId = "WardrobeHeightInput";
	const std::string c_outerwallDepthInputId = "OuterwallDepthInput";
	const std::string c_wallThicknessInputId = "WallThicknessInput";
	const std::string c_innerwallDepthInputId = "InnerwallDepthInput";
	const std::string c_comboCountInputId = "ComboCountInput";
	const std::string c_partitionCountInputId = "PartitionCountInput";
	const std::string c_woodMatInputId = "WoodMatInput";

	std::string wstring2string(const std::wstring & wstr)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
		std::string str = convert.to_bytes(wstr);
		return str;
	}

	typedef std::map<std::string, Ptr<Material> > MatNameObjMap;
	MatNameObjMap _woodMatNameObjMap;
	std::vector<std::string> getWoodMaterilList(const Ptr<Application>& app)
	{
		std::vector<std::string> woodNames;
		if (!_woodMatNameObjMap.empty())
		{
			for (MatNameObjMap::const_iterator it = _woodMatNameObjMap.begin(); it != _woodMatNameObjMap.end(); ++it)
				woodNames.emplace_back(it->first);

			return woodNames;
		}

		if (!app)
			return woodNames;

		Ptr<MaterialLibraries> libs = _app->materialLibraries();
		if (!libs)
			return woodNames;

		for (size_t i = 0; i < libs->count(); ++i)
		{
			Ptr<MaterialLibrary> lib = libs->item(i);
			if (!lib)
				continue;

			Ptr<Materials> mats = lib->materials();
			if (!mats)
				continue;

			for (size_t j = 0; j < mats->count(); ++j)
			{
				Ptr<Material> mat = mats->item(j);
				if (!mat)
					continue;

				Ptr<Properties> props = mat->materialProperties();
				if (!props)
					continue;

				Ptr<ChoiceProperty> prop = props->itemById("physmat_Type");
				if (prop && prop->value() == "physmat_wood")
				{
					woodNames.emplace_back(mat->name());
					_woodMatNameObjMap[mat->name()] = mat;
				}
			}
		}

		return woodNames;
	}

	Ptr<Material> getWoodMaterialByName(const std::string& name)
	{
		MatNameObjMap::const_iterator it = _woodMatNameObjMap.find(name);
		if (it != _woodMatNameObjMap.end())
			return it->second;
		else
			return nullptr;
	}

	void buildWardrobe(const Ptr<Component>& comp,
		double widthOfWardrobe,
		double heightOfWardrobe,
		double depthOfOuterwall,
		double depthOfInnerwall,
		double thicknessOfWall,
		int countOfCombo,
		int countOfPartition,
		const std::string& woodName)
	{
		if (!comp)
			return;

		// Create a new sketch
		Ptr<Sketches> sketches = comp->sketches();
		if (!sketches)
			return;

		Ptr<Sketch> sketch = sketches->add(comp->xYConstructionPlane());
		if (!sketch)
			return;
		sketch->isVisible(false);

		// Create 4 outer walls
		Ptr<Point3D> cornerpt1 = Point3D::create(0, 0, 0);
		Ptr<Point3D> cornerpt2 = Point3D::create(widthOfWardrobe, 0, 0);
		Ptr<Point3D> cornerpt3 = Point3D::create(widthOfWardrobe, heightOfWardrobe, 0);
		Ptr<Point3D> cornerpt4 = Point3D::create(0, heightOfWardrobe, 0);

		Ptr<SketchCurves> curves = sketch->sketchCurves();
		if (!curves)
			return;

		Ptr<SketchLines> lines = curves->sketchLines();
		if (!lines)
			return;

		Ptr<ObjectCollection> outerwalls = ObjectCollection::create();
		outerwalls->add(lines->addByTwoPoints(cornerpt1, cornerpt2));
		outerwalls->add(lines->addByTwoPoints(cornerpt2, cornerpt3));
		outerwalls->add(lines->addByTwoPoints(cornerpt3, cornerpt4));
		outerwalls->add(lines->addByTwoPoints(cornerpt4, cornerpt1));

		// Create inner walls according to the number of combo and partition
		double widthOfCombo = widthOfWardrobe / countOfCombo;
		double heightOfPartition = heightOfWardrobe / countOfPartition;
		Ptr<ObjectCollection> innerwalls = ObjectCollection::create();
		for (int i = 0; i < countOfCombo; ++i)
		{
			if (i != 0)
			{
				Ptr<Point3D> leftwallpt1 = Point3D::create(i * widthOfCombo + thicknessOfWall / 2 + 0.05, thicknessOfWall, 0);
				Ptr<Point3D> leftwallpt2 = Point3D::create(i * widthOfCombo + thicknessOfWall / 2 + 0.05, heightOfWardrobe - thicknessOfWall, 0);
				innerwalls->add(lines->addByTwoPoints(leftwallpt1, leftwallpt2));
			}
			if (i != countOfCombo - 1)
			{
				Ptr<Point3D> rightwallpt1 = Point3D::create((i + 1) * widthOfCombo - thicknessOfWall / 2 - 0.05, thicknessOfWall, 0);
				Ptr<Point3D> rightwallpt2 = Point3D::create((i + 1) * widthOfCombo - thicknessOfWall / 2 - 0.05, heightOfWardrobe - thicknessOfWall, 0);
				innerwalls->add(lines->addByTwoPoints(rightwallpt1, rightwallpt2));
				Ptr<Point3D> middlewallpt1 = Point3D::create((i + 0.5) * widthOfCombo, thicknessOfWall, 0);
				Ptr<Point3D> middlewallpt2 = Point3D::create((i + 0.5) * widthOfCombo, heightOfWardrobe - thicknessOfWall, 0);
				innerwalls->add(lines->addByTwoPoints(middlewallpt1, middlewallpt2));
			}

			for (int j = 1; j < countOfPartition; ++j)
			{
				Ptr<Point3D> p1 = Point3D::create(i * widthOfCombo + thicknessOfWall, j * heightOfPartition, 0);
				Ptr<Point3D> p2 = Point3D::create((i + 0.5) * widthOfCombo - thicknessOfWall / 2 - 0.05, j * heightOfPartition, 0);
				Ptr<Point3D> p3 = Point3D::create((i + 0.5) * widthOfCombo + thicknessOfWall / 2 + 0.05, j * heightOfPartition, 0);
				Ptr<Point3D> p4 = Point3D::create((i + 1) * widthOfCombo - thicknessOfWall, j * heightOfPartition, 0);
				if (i != countOfCombo - 1)
				{
					innerwalls->add(lines->addByTwoPoints(p1, p2));
					innerwalls->add(lines->addByTwoPoints(p3, p4));
				}
				else if (j == countOfPartition - 1)
					innerwalls->add(lines->addByTwoPoints(p1, p4));
			}
		}

		// Extrude outer walls
		Ptr<Features> features = comp->features();
		if (!features)
			return;

		Ptr<ExtrudeFeatures> extrudes = features->extrudeFeatures();
		if (!extrudes)
			return;

		Ptr<Profile> outerwallsprofile = comp->createOpenProfile(outerwalls);
		if (!outerwallsprofile)
			return;

		Ptr<ExtrudeFeatureInput> outerwallsinput = extrudes->createInput(outerwallsprofile, FeatureOperations::NewBodyFeatureOperation);
		if (!outerwallsinput)
			return;

		outerwallsinput->isSolid(false);
		outerwallsinput->setDistanceExtent(false, ValueInput::createByReal(depthOfOuterwall));
		Ptr<ExtrudeFeature> outerwallsextrude = extrudes->add(outerwallsinput);
		if (!outerwallsextrude)
			return;

		// Thicken outer walls
		Ptr<ThickenFeatures> thickens = features->thickenFeatures();
		if (!thickens)
			return;

		Ptr<ObjectCollection> outerwallsfaces = ObjectCollection::create();
		Ptr<BRepFaces> outerwallsidefaces = outerwallsextrude->sideFaces();
		if (!outerwallsfaces || !outerwallsidefaces)
			return;

		for (int i = 0; i < outerwallsidefaces->count(); ++i)
			outerwallsfaces->add(outerwallsidefaces->item(i));

		Ptr<ThickenFeatureInput> outerwallthickeninput = thickens->createInput(outerwallsfaces, ValueInput::createByReal(-1 * thicknessOfWall), false, FeatureOperations::NewBodyFeatureOperation, false);
		thickens->add(outerwallthickeninput);

		// Create back wall
		Ptr<PatchFeatures> patches = features->patchFeatures();
		if (!patches)
			return;

		Ptr<PatchFeatureInput> backwallinput = patches->createInput(outerwalls, FeatureOperations::NewBodyFeatureOperation);
		if (!backwallinput)
			return;

		Ptr<PatchFeature> backwallpatch = patches->add(backwallinput);
		if (!backwallpatch)
			return;

		// Thicken back wall
		Ptr<ObjectCollection> backwallfaces = ObjectCollection::create();
		Ptr<BRepFaces> backwallbrepfaces = backwallpatch->faces();
		if (!backwallfaces || !backwallbrepfaces)
			return;

		for (int i = 0; i < backwallbrepfaces->count(); ++i)
			backwallfaces->add(backwallbrepfaces->item(i));

		Ptr<ThickenFeatureInput> backwallthickeninput = thickens->createInput(backwallfaces, ValueInput::createByReal(thicknessOfWall), false, FeatureOperations::NewBodyFeatureOperation, false);
		thickens->add(backwallthickeninput);

		// Extrude inner walls
		Ptr<ObjectCollection> innerwallsprofiles = ObjectCollection::create();
		if (!innerwallsprofiles)
			return;

		for (auto innerwall : innerwalls)
			innerwallsprofiles->add(comp->createOpenProfile(innerwall));
		
		Ptr<ExtrudeFeatureInput> innerwallsinput = extrudes->createInput(innerwallsprofiles, FeatureOperations::NewBodyFeatureOperation);
		if (!innerwallsinput)
			return;

		innerwallsinput->isSolid(false);
		innerwallsinput->setDistanceExtent(false, ValueInput::createByReal(depthOfInnerwall));
		Ptr<ExtrudeFeature> innerwallsextrude = extrudes->add(innerwallsinput);
		if (!innerwallsextrude)
			return;

		// Thicken inner walls
		Ptr<ObjectCollection> innerwallsfaces = ObjectCollection::create();
		Ptr<BRepFaces> innerwallsidefaces = innerwallsextrude->sideFaces();
		if (!innerwallsfaces || !innerwallsidefaces)
			return;

		for (int i = 0; i < innerwallsidefaces->count(); ++i)
			innerwallsfaces->add(innerwallsidefaces->item(i));

		Ptr<ThickenFeatureInput> innerwallsthickeninput = thickens->createInput(innerwallsfaces, ValueInput::createByReal(thicknessOfWall / 4), true, FeatureOperations::NewBodyFeatureOperation, false);
		thickens->add(innerwallsthickeninput);

		// Set wood material
		Ptr<Material> woodMat = getWoodMaterialByName(woodName);
		if (woodMat)
			comp->material(woodMat);
	}
}

class WardrobeCmdExecuteEventHandler : public CommandEventHandler
{
public:
	void notify(const Ptr<CommandEventArgs>& eventArgs) override
	{
		if (!_app)
			return;

		Ptr<Design> design = _app->activeProduct();
		if (!design)
			return;

		Ptr<Component> comp = design->rootComponent();
		if (!comp)
			return;

		Ptr<Command> cmd = eventArgs->command();
		if (!cmd)
			return;

		Ptr<CommandInputs> inputs = cmd->commandInputs();
		if (!inputs)
			return;

		Ptr<ValueCommandInput> wardrobeWidthInput = inputs->itemById(c_wardrobeWidthInputId);
		if (!wardrobeWidthInput)
			return;

		Ptr<ValueCommandInput> wardrobeHeightInput = inputs->itemById(c_wardrobeHeightInputId);
		if (!wardrobeHeightInput)
			return;

		Ptr<ValueCommandInput> outerwallDepthInput = inputs->itemById(c_outerwallDepthInputId);
		if (!outerwallDepthInput)
			return;

		Ptr<ValueCommandInput> wallThicknessInput = inputs->itemById(c_wallThicknessInputId);
		if (!wallThicknessInput)
			return;

		Ptr<ValueCommandInput> innerwallDepthInput = inputs->itemById(c_innerwallDepthInputId);
		if (!innerwallDepthInput)
			return;

		Ptr<IntegerSpinnerCommandInput> comboCountInput = inputs->itemById(c_comboCountInputId);
		if (!comboCountInput)
			return;

		Ptr<IntegerSpinnerCommandInput> partitionCountInput = inputs->itemById(c_partitionCountInputId);
		if (!partitionCountInput)
			return;

		Ptr<DropDownCommandInput> woodMatInput = inputs->itemById(c_woodMatInputId);
		if (!woodMatInput)
			return;

		buildWardrobe(comp,
			wardrobeWidthInput->value(),
			wardrobeHeightInput->value(),
			outerwallDepthInput->value(),
			innerwallDepthInput->value(),
			wallThicknessInput->value(),
			comboCountInput->value(),
			partitionCountInput->value(),
			woodMatInput->selectedItem()->name());

		// Fit view
		Ptr<Viewport> activeView = _app->activeViewport();
		if (activeView)
			activeView->fit();
	}
} _wardrobeCmdExecute;

class WardrobeCmdInputChangedHandler : public adsk::core::InputChangedEventHandler
{
public:
	void notify(const Ptr<InputChangedEventArgs>& eventArgs) override
	{
	}
} _wardrobeCmdInputChanged;

class WardrobeCmdValidateInputsEventHandler : public adsk::core::ValidateInputsEventHandler
{
public:
	void notify(const Ptr<ValidateInputsEventArgs>& eventArgs) override
	{
		Ptr<CommandInputs> inputs = eventArgs->inputs();
		if (!inputs)
			return;

		Ptr<ValueCommandInput> wardrobeWidthInput = inputs->itemById(c_wardrobeWidthInputId);
		if (!wardrobeWidthInput)
			return;

		Ptr<ValueCommandInput> wardrobeHeightInput = inputs->itemById(c_wardrobeHeightInputId);
		if (!wardrobeHeightInput)
			return;

		Ptr<ValueCommandInput> outerwallDepthInput = inputs->itemById(c_outerwallDepthInputId);
		if (!outerwallDepthInput)
			return;

		Ptr<ValueCommandInput> wallThicknessInput = inputs->itemById(c_wallThicknessInputId);
		if (!wallThicknessInput)
			return;

		Ptr<ValueCommandInput> innerwallDepthInput = inputs->itemById(c_innerwallDepthInputId);
		if (!innerwallDepthInput)
			return;

		Ptr<IntegerSpinnerCommandInput> comboCountInput = inputs->itemById(c_comboCountInputId);
		if (!comboCountInput)
			return;

		Ptr<IntegerSpinnerCommandInput> partitionCountInput = inputs->itemById(c_partitionCountInputId);
		if (!partitionCountInput)
			return;

		if (wardrobeWidthInput->value() <= 0 
			|| wardrobeHeightInput->value() <= 0 
			|| wallThicknessInput->value() <= 0
			|| outerwallDepthInput->value() <= 0
			|| innerwallDepthInput->value() <= 0)
			eventArgs->areInputsValid(false);
		else if (wardrobeWidthInput->value() / comboCountInput->value() <= 4 * wallThicknessInput->value())
			eventArgs->areInputsValid(false);
		else if (wardrobeHeightInput->value() / partitionCountInput->value() <= 2 * wallThicknessInput->value())
			eventArgs->areInputsValid(false);
		else if (outerwallDepthInput->value() < innerwallDepthInput->value())
			eventArgs->areInputsValid(false);
	}
} _wardrobeCmdValidateInputs;

class WardrobeDesignCmdCreatedEventHandler : public CommandCreatedEventHandler
{
public:
	void notify(const Ptr<CommandCreatedEventArgs>& eventArgs) override
	{
		Ptr<Command> cmd = eventArgs->command();
		if (!cmd)
			return;

		cmd->isExecutedWhenPreEmpted(false);
		Ptr<CommandInputs> inputs = cmd->commandInputs();
		if (!inputs)
			return;

		Ptr<ValueCommandInput> wardrobeWidthInput = inputs->addValueInput(c_wardrobeWidthInputId, wstring2string(L"柜宽："), "m", adsk::core::ValueInput::createByReal(200));
		if (!wardrobeWidthInput)
			return;

		Ptr<ValueCommandInput> wardrobeHeightInput = inputs->addValueInput(c_wardrobeHeightInputId, wstring2string(L"柜高："), "m", adsk::core::ValueInput::createByReal(240));
		if (!wardrobeHeightInput)
			return;

		Ptr<ValueCommandInput> outerwallDepthInput = inputs->addValueInput(c_outerwallDepthInputId, wstring2string(L"柜深："), "mm", adsk::core::ValueInput::createByReal(60));
		if (!outerwallDepthInput)
			return;

		Ptr<ValueCommandInput> wallThicknessInput = inputs->addValueInput(c_wallThicknessInputId, wstring2string(L"柜板厚度："), "mm", adsk::core::ValueInput::createByReal(1.8));
		if (!wallThicknessInput)
			return;

		Ptr<ValueCommandInput> innerwallDepthInput = inputs->addValueInput(c_innerwallDepthInputId, wstring2string(L"隔板深度："), "mm", adsk::core::ValueInput::createByReal(49.5));
		if (!innerwallDepthInput)
			return;

		Ptr<IntegerSpinnerCommandInput> comboCountInput = inputs->addIntegerSpinnerCommandInput(c_comboCountInputId, wstring2string(L"组合数："), 1, 100, 1, 2);
		if (!comboCountInput)
			return;

		Ptr<IntegerSpinnerCommandInput> partitionCountInput = inputs->addIntegerSpinnerCommandInput(c_partitionCountInputId, wstring2string(L"柜格数："), 1, 30, 1, 6);
		if (!partitionCountInput)
			return;

		std::vector<std::string> woodNames = getWoodMaterilList(_app);
		if (!woodNames.empty())
		{
			Ptr<DropDownCommandInput> woodMatInput = inputs->addDropDownCommandInput(c_woodMatInputId, wstring2string(L"材质："), DropDownStyles::TextListDropDownStyle);
			if (!woodMatInput)
				return;

			Ptr<ListItems> woodMatList = woodMatInput->listItems();
			if (!woodMatList)
				return;

			for (std::string woodName : woodNames)
				woodMatList->add(woodName, false);

			Ptr<ListItem> firstItem = woodMatList->item(0);
			if (firstItem)
				firstItem->isSelected(true);

			Ptr<InputChangedEvent> inputChangedEvent = cmd->inputChanged();
			if (!inputChangedEvent)
				return;

			bool bRes = inputChangedEvent->add(&_wardrobeCmdInputChanged);
			if (!bRes)
				return;

			Ptr<ValidateInputsEvent> validateInputsEvent = cmd->validateInputs();
			if (!validateInputsEvent)
				return;

			bRes = validateInputsEvent->add(&_wardrobeCmdValidateInputs);
			if (!bRes)
				return;

			Ptr<CommandEvent> executeEvent = cmd->execute();
			if (!executeEvent)
				return;

			bRes = executeEvent->add(&_wardrobeCmdExecute);
			if (!bRes)
				return;
		}
	}
} _wardrobeCmdCreated;

extern "C" XI_EXPORT bool run(const char* context)
{
	_app = Application::get();
	if (!_app)
		return false;

	_ui = _app->userInterface();
	if (!_ui)
		return false;

	Ptr<CommandDefinitions> cmdDefs = _ui->commandDefinitions();
	if (!cmdDefs)
		return false;

	Ptr<CommandDefinition> wardrobeDesignCmdDef = cmdDefs->addButtonDefinition("adskWardrobeDesignCmdDef", wstring2string(L"木柜设计"), wstring2string(L"按照定义的参数来自动生成木柜"), "Resources");
	if (!wardrobeDesignCmdDef)
		return false;

	Ptr<CommandCreatedEvent> cmdCreatedEvent = wardrobeDesignCmdDef->commandCreated();
	if (!cmdCreatedEvent)
		return false;

	bool bRes = cmdCreatedEvent->add(&_wardrobeCmdCreated);
	if (!bRes)
		return false;

	Ptr<Workspaces> workspaces = _ui->workspaces();
	if (!workspaces)
		return false;

	Ptr<Workspace> modelWorkspace = workspaces->itemById("FusionSolidEnvironment");
	if (!modelWorkspace)
		return false;

	Ptr<ToolbarPanels> toolbarPanels = modelWorkspace->toolbarPanels();
	if (!toolbarPanels)
		return false;

	Ptr<ToolbarPanel> furnitureDesignPanel = toolbarPanels->add("adskFurnitureDesignPanel", wstring2string(L"家具设计"));
	if (!furnitureDesignPanel)
		return false;

	Ptr<ToolbarControls> furnitureDesignCtrls = furnitureDesignPanel->controls();
	if (!furnitureDesignCtrls)
		return false;

	Ptr<CommandControl> wardrobeCtrl = furnitureDesignCtrls->addCommand(wardrobeDesignCmdDef);
	if (!wardrobeCtrl)
		return false;
	wardrobeCtrl->isPromoted(true);
	wardrobeCtrl->isPromotedByDefault(true);

	return true;
}

extern "C" XI_EXPORT bool stop(const char* context)
{
	if (_ui)
	{
		Ptr<Workspaces> workspaces = _ui->workspaces();
		if (!workspaces)
			return false;

		Ptr<Workspace> modelWorkspace = workspaces->itemById("FusionSolidEnvironment");
		if (!modelWorkspace)
			return false;

		Ptr<ToolbarPanels> toolbarPanels = modelWorkspace->toolbarPanels();
		if (!toolbarPanels)
			return false;

		Ptr<ToolbarPanel> furnitureDesignPanel = toolbarPanels->itemById("adskFurnitureDesignPanel");
		if (furnitureDesignPanel)
		{
			Ptr<ToolbarControls> furnitureDesignCtrls = furnitureDesignPanel->controls();
			if (!furnitureDesignCtrls)
				return false;

			Ptr<CommandControl> wardrobeCtrl = furnitureDesignCtrls->itemById("adskWardrobeDesignCmdDef");
			if (wardrobeCtrl)
				wardrobeCtrl->deleteMe();

			furnitureDesignPanel->deleteMe();
		}

		Ptr<CommandDefinitions> cmdDefs = _ui->commandDefinitions();
		if (!cmdDefs)
			return false;

		Ptr<CommandDefinition> wardrobeDesignCmdDef = cmdDefs->itemById("adskWardrobeDesignCmdDef");
		if (wardrobeDesignCmdDef)
			wardrobeDesignCmdDef->deleteMe();
	}

	return true;
}


#ifdef XI_WIN

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // XI_WIN
