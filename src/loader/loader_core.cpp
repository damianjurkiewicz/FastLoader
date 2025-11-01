#include "pch.h"
#include "loader_core.h"
#include "audio.h"
#include "cargrp.h"
#include "objdat.h"
#include "additionaltxd.h"
#include "default_audio_data.h" // This include should now work

FastLoader::FastLoader(HINSTANCE pluginHandle)
{
    // ... (no changes in this function)
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
    MessageBox(
        NULL,
        "Asi nr 1'.",
        MODNAME,
        MB_OK | MB_ICONINFORMATION
    );
    return true;
}

// <<< MODIFIED FUNCTION TO HANDLE BACKUPS AND RESET >>>
void FastLoader::HandleVanillaDataFiles()
{
    // 1. Read INI settings
    bool bObjDatLoaderEnabled = (gConfig.ReadInteger("MAIN", "ObjDatLoader", 1) == 1);
    bool bCargrpLoaderEnabled = (gConfig.ReadInteger("MAIN", "CargrpLoader", 1) == 1);

    int flaAudioLoaderSetting = gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1);

    std::string settingsPath = GAME_PATH((char*)"data/gtasa_vehicleAudioSettings.cfg");
    std::string backupPath = settingsPath + ".fastloader.bak";


    // <<< START OF AUDIO LOGIC (NOW WITH 3 STATES) >>>

    if (flaAudioLoaderSetting == 1) // --- STATE 1: LOADER IS ON ---
    {
        // ... (this 'if' block is unchanged)
        if (std::filesystem::exists(settingsPath) && !std::filesystem::exists(backupPath))
        {
            MessageBox(
                NULL,
                "Asi nr 1'.",
                MODNAME,
                MB_OK | MB_ICONINFORMATION
            );

        }
    }
    else if (flaAudioLoaderSetting == -1) // --- STATE -1: RESET TO DEFAULT ---
    {
        // <<< THIS BLOCK IS CHANGED >>>
        try
        {
            // Open the .cfg file for writing in BINARY mode
            std::ofstream out(settingsPath, std::ios::binary);
            if (!out.is_open())
            {
                throw std::runtime_error("Could not open settings file for writing.");
            }

            // Write the default content from the header file as raw bytes
            out.write(reinterpret_cast<const char*>(DefaultAudioData::GtaSaVehicleAudioSettings_data),
                DefaultAudioData::GtaSaVehicleAudioSettings_len);
            out.close();

            // Set the INI value back to 0 (Off)
            gConfig.WriteInteger("MAIN", "FLAAudioLoader", 0);

            MessageBox(NULL, "'gtasa_vehicleAudioSettings.cfg' has been reset to default.\n\nFLAAudioLoader has been set to 0 (Off) in your INI.", MODNAME, MB_OK | MB_ICONINFORMATION);
        }
        catch (const std::exception& e)
        {
            MessageBox(NULL, ("Failed to reset audio settings: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
        }
        // <<< END OF CHANGES >>>
    }

    // <<< END OF AUDIO LOGIC >>>


    // <<< START OF .DAT FILE LOGIC (TRAVERSAL) >>>
    if (!bObjDatLoaderEnabled && !bCargrpLoaderEnabled)
    {
        return;
    }

    // ... (rest of the function is unchanged) ...
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
    // ... (no changes in this function) ...
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