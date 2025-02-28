# DaylonLevellerLandscape
An Unreal Engine landscape import plugin that supports Daylon Leveller documents.

## Prerequistes

- Unreal Engine 5.5 (other versions may work but haven't been tested).

- A UE project that has been configured for C++, so that it has a .sln file (if you're using Visual Studio)
to which you can add the plugin's source code and then compile.

- Or, if you've built UE from source, you can add the plugin to the engine so that all
of your UE projects can access it.


## Installing

We'll assume you want to use the plugin with a specific UE project.

If your UE project doesn't already have a folder named "Plugins", create it.

Clone the repository to your local computer. If you clone it to inside the Plugins folder, 
then you can go to the next step. Otherwise, you need to move or copy the cloned repo into 
your project's Plugins folder. You can also just download a ZIP archive of this repo and 
extract it to your Plugins folder (but rename it to "DaylonLevellerLandscape" because 
Github will name it "DaylonLevellerLandscape-main"). In any case, your project should have
this directory structure:

	YourUEProject
		YourUEProject.uproject
		YourUEProject.sln (if using Visual Studio)
		(probably some other files and folders)
		Binaries
		Content
		Source
		Plugins
			DaylonLevellerLandscape
				DaylonLevellerLandscape.uplugin
				Content
				Resources
				Source
					DaylonLevellerLandscape
						Private
						Public
						DaylonLevellerLandscape.Build.cs
					
If using Windows, right-click the .uproject file and choose "Generate Visual Studio project files".
For other platforms, follow their steps for regenerating project files.

Open your project in your development environment (e.g. Visual Studio). The plugin files 
should be visible inside the Games/YourUEProject/Plugins folder.

Switch (if necessary) to the Development Editor build type.

Build your UE project to compile the plugin.

Launch your project to bring up the UE Editor for it.

## Using the plugin

In the UE editor, choose "Landscape" from the Mode menu in the main toolbar in the upper left of the editor window, 
or press Shift-2. A "Landscape" tab should appear with buttons named Manage, Sculpt, and Paint. Click "Manage".

To import a Leveller document into a new Landscape actor, click New, then Import from File. You should be able 
to specify a Leveller document's filename in the "Heightmap File" field. If you click the field's "..." button, 
the Import dialog should offer the Leveller filetype .ter as a format choice.

To import a Leveller document to replace an existing Landscape actor, select the actor, then 
click Import inside the Landscape tab's panel, and choose the Import radio button. Check "Heightmap File" 
and specify the Leveller document's filename in the field (or click the "..." button).

More information is available on the Daylon Graphics website [here](https://www.daylongraphics.com/support/ue_interop.php).