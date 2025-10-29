#include "pch.h"
#include "loader_core.h"
#include "audio.h"
#include "cargrp.h"
#include "objdat.h"
#include "additionaltxd.h"
#include "default_audio_data.h" // <<< ADDED NEW HEADER FILE

FastLoader::FastLoader(HINSTANCE pluginHandle)
{  
    // ... (no changes)
    handle = pluginHandle;
    if (!IsPluginNameValid())
    {
        return;
    }
    HandleVanillaDataFiles();
    ParseModloader();
    if (gConfig.ReadInteger("MAIN", "AdditionalTxdLoader", 1) == 1)
    {
        AdditionalTXD.Init();
    }
    if (gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1) == 1)
    {
        FLAAudioLoader.Process();
    }
}

bool FastLoader::IsPluginNameValid()
{
    // ... (no changes)
    char buf[MAX_PATH];
    DWORD result = GetModuleFileName(handle, buf, MAX_PATH);
    if (!result)
    {
        MessageBox(NULL, "Failed to fetch filename", MODNAME, MB_OK);
        return false;
    }
    std::string curName = buf;
    size_t lastSlash = curName.find_last_of("\\/");
    if (lastSlash != std::string::npos)
    {
        curName = curName.substr(lastSlash + 1);
    }
    if (curName != MODNAME_EXT)
    {
        MessageBox(NULL, "The plugin was renamed. Exiting...!", MODNAME, MB_OK);
        return false;
    }
    return true;
}

// <<< MODIFIED FUNCTION TO HANDLE BACKUPS AND RESET >>>
void FastLoader::HandleVanillaDataFiles()
{
    // 1. Read INI settings
    bool bObjDatLoaderEnabled = (gConfig.ReadInteger("MAIN", "ObjDatLoader", 1) == 1);
    bool bCargrpLoaderEnabled = (gConfig.ReadInteger("MAIN", "CargrpLoader", 1) == 1);

    // Read the integer value for audio
    int flaAudioLoaderSetting = gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1);

    std::string settingsPath = GAME_PATH((char*)"data/gtasa_vehicleAudioSettings.cfg");
    std::string backupPath = settingsPath + ".fastloader.bak";


    // <<< START OF AUDIO LOGIC (NOW WITH 3 STATES) >>>

    if (flaAudioLoaderSetting == 1) // --- STATE 1: LOADER IS ON ---
    {
        // This is the existing backup logic
        if (std::filesystem::exists(settingsPath) && !std::filesystem::exists(backupPath))
        {
            int result = MessageBox(NULL,
                "FastLoader (FLAAudioLoader) is about to modify 'gtasa_vehicleAudioSettings.cfg' to add new vehicle sounds.\n\n"
                "Do you want to create a one-time backup of the original file? (Recommended)",
                MODNAME,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

            if (result == IDYES)
            {
                try {
                    std::filesystem::copy_file(settingsPath, backupPath, std::filesystem::copy_options::overwrite_existing);
                }
                catch (const std::exception& e) {
                    MessageBox(NULL, ("Failed to create audio backup: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
                }
            }
        }
    }
    else if (flaAudioLoaderSetting == -1) // --- STATE -1: RESET TO DEFAULT ---
    {
        try
        {
            // Open the .cfg file for writing (overwriting it)
            std::ofstream out(settingsPath);
            if (!out.is_open())
            {
                throw std::runtime_error("Could not open settings file for writing.");
            }

            // Write the default content from the header file
            out << DefaultAudioData::GtaSaVehicleAudioSettings;
            out.close();

            // IMPORTANT: Set the INI value back to 0 (Off),
            // to avoid resetting the file every time the game starts.
            // Assuming you have a WriteInteger function or similar.
            gConfig.WriteInteger("MAIN", "FLAAudioLoader", 0);

            MessageBox(NULL, "'gtasa_vehicleAudioSettings.cfg' has been reset to default.\n\nFLAAudioLoader has been set to 0 (Off) in your INI.", MODNAME, MB_OK | MB_ICONINFORMATION);
        }
        catch (const std::exception& e)
        {
            MessageBox(NULL, ("Failed to reset audio settings: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
        }
    }
    // else (flaAudioLoaderSetting == 0) // --- STATE 0: LOADER IS OFF ---
    // {
    //     // Do nothing, file is left as-is
    // }

    // <<< END OF AUDIO LOGIC >>>


    // <<< START OF .DAT FILE LOGIC (TRAVERSAL) >>>
    if (!bObjDatLoaderEnabled && !bCargrpLoaderEnabled)
    {
        return;
    }

    std::function<void(const std::filesystem::path&)> traverse;
    traverse = [&](const std::filesystem::path& dir)
        {
            // ... (rest of this function is unchanged) ...
            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    if (!folderName.empty() && folderName[0] == '.')
                    {
                        continue;
                    }
                    traverse(entry.path());
                    continue;
                }
                if (!entry.is_regular_file())
                {
                    continue;
                }
                std::string fileName = entry.path().filename().string();

                if (bObjDatLoaderEnabled && fileName == "object.dat")
                {
                    std::string path = entry.path().string();
                    std::string parentPath = entry.path().parent_path().string();
                    int result = MessageBox(NULL, "Found object.dat in your modloader folder. Do you want to rename?", MODNAME, MB_YESNO | MB_ICONQUESTION);
                    if (result == IDYES)
                    {
                        std::string newName = fileName + ".bak";
                        std::string newPath = parentPath + "\\" + newName;
                        try {
                            std::filesystem::rename(path, newPath);
                        }
                        catch (const std::exception& e) {
                            MessageBox(NULL, ("Failed to rename: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
                        }
                    }
                }

                if (bCargrpLoaderEnabled && fileName == "cargrp.dat")
                {
                    std::string path = entry.path().string();
                    std::string parentPath = entry.path().parent_path().string();
                    int result = MessageBox(NULL, "Found cargrp.dat in your modloader folder. Do you want to rename?", MODNAME, MB_YESNO | MB_ICONQUESTION);
                    if (result == IDYES)
                    {
                        std::string newName = fileName + ".bak";
                        std::string newPath = parentPath + "\\" + newName;
                        try {
                            std::filesystem::rename(path, newPath);
                        }
                        catch (const std::exception& e) {
                            MessageBox(NULL, ("Failed to rename: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
                        }
                    }
                }
            }
        };

    traverse(GAME_PATH((char*)"modloader"));
}



// <<< MODIFIED PARSEMODLOADER FUNCTION (PARSING ONLY) >>>
void FastLoader::ParseModloader()
{
    // ... (this function is unchanged) ...
    std::function<void(const std::filesystem::path&)> traverse;
    traverse = [&](const std::filesystem::path& dir)
        {
            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    if (!folderName.empty() && folderName[0] == '.')
                    {
                        continue;
                    }
                    traverse(entry.path());
                    continue;
                }
                if (!entry.is_regular_file())
                {
                    continue;
                }
                std::string ext = entry.path().extension().string();
                std::string path = entry.path().string();

                if (ext == ".fastloader")
                {
                    std::ifstream in(path);
                    std::string line;
                    while (getline(in, line))
                    {
                        if (line.starts_with(";") || line.starts_with("//") || line.starts_with("#"))
                        {
                            continue;
                        }

                        if (gConfig.ReadInteger("MAIN", "CargrpLoader", 1) == 1)
                        {
                            CargrpLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "ObjDatLoader", 1) == 1)
                        {
                            ObjDatLoader.Parse(line);
                        }
                        if (gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1) == 1)
                        {
                            FLAAudioLoader.Parse(line);
                        }
                    }
                    in.close();
                }
            }
        };

    traverse(GAME_PATH((char*)"modloader"));
}