#include "pch.h"
#include "loader_core.h"
#include "audio.h"
#include "cargrp.h"
#include "objdat.h"
#include "additionaltxd.h"

FastLoader::FastLoader(HINSTANCE pluginHandle)
{
    handle = pluginHandle;
    if (!IsPluginNameValid())
    {
        return;
    }

    // <<< REORDERED >>>
    // 1. First, check for conflicting .dat files and handle backups
    HandleVanillaDataFiles();

    // 2. Only then, parse the .fastloader files to load mod data
    ParseModloader();

    // 3. Process the loaded data
    if (gConfig.ReadBoolean("MAIN", "AdditionalTxdLoader", true))
    {
        AdditionalTXD.Init();
    }
    if (gConfig.ReadBoolean("MAIN", "FLAAudioLoader", true))
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

// <<< MODIFIED FUNCTION TO HANDLE ALL BACKUPS >>>
void FastLoader::HandleVanillaDataFiles()
{
    // 1. Read INI settings at the very beginning
    bool bObjDatLoaderEnabled = gConfig.ReadBoolean("MAIN", "ObjDatLoader", true);
    bool bCargrpLoaderEnabled = gConfig.ReadBoolean("MAIN", "CargrpLoader", true);
    bool bFLAAudioLoaderEnabled = gConfig.ReadBoolean("MAIN", "FLAAudioLoader", true); // Added this check

    // <<< START OF AUDIO BACKUP LOGIC >>>
    // This logic runs independently, as it checks the 'data' folder, not 'modloader'
    if (bFLAAudioLoaderEnabled)
    {
        std::string settingsPath = GAME_PATH((char*)"data/gtasa_vehicleAudioSettings.cfg");
        std::string backupPath = settingsPath + ".fastloader.bak"; // Use a unique, persistent backup name

        // Check if the original file exists AND our backup does NOT exist yet
        if (std::filesystem::exists(settingsPath) && !std::filesystem::exists(backupPath))
        {
            // Ask the user for permission
            int result = MessageBox(NULL,
                "FastLoader (FLAAudioLoader) is about to modify 'gtasa_vehicleAudioSettings.cfg' to add new vehicle sounds.\n\n"
                "Do you want to create a one-time backup of the original file? (Recommended)",
                MODNAME,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

            if (result == IDYES)
            {
                try
                {
                    // This is a TRUE backup. We copy the file, not rename it.
                    std::filesystem::copy_file(settingsPath, backupPath, std::filesystem::copy_options::overwrite_existing);
                }
                catch (const std::exception& e)
                {
                    MessageBox(NULL, ("Failed to create audio backup: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
                }
            }
            // If user clicks NO, we just continue. The file will be patched without a backup.
        }
    }
    // <<< END OF AUDIO BACKUP LOGIC >>>


    // <<< START OF .DAT FILE LOGIC (TRAVERSAL) >>>
    // If both .dat loaders are disabled, no need to scan the modloader folder
    if (!bObjDatLoaderEnabled && !bCargrpLoaderEnabled)
    {
        // We return here, after the audio check is complete
        return;
    }

    // Use the same traversal logic, but only for .dat files in 'modloader'
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

                // --- Logic for object.dat ---
                if (bObjDatLoaderEnabled && fileName == "object.dat")
                {
                    std::string path = entry.path().string();
                    std::string parentPath = entry.path().parent_path().string();
                    int result = MessageBox(NULL, "Found object.dat in your modloader folder. Do you want to rename?", MODNAME, MB_YESNO | MB_ICONQUESTION);
                    if (result == IDYES)
                    {
                        std::string newName = fileName + ".bak";
                        std::string newPath = parentPath + "\\" + newName;
                        try
                        {
                            std::filesystem::rename(path, newPath);
                        }
                        catch (const std::exception& e)
                        {
                            MessageBox(NULL, ("Failed to rename: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
                        }
                    }
                }

                // --- Logic for cargrp.dat ---
                if (bCargrpLoaderEnabled && fileName == "cargrp.dat")
                {
                    std::string path = entry.path().string();
                    std::string parentPath = entry.path().parent_path().string();
                    int result = MessageBox(NULL, "Found cargrp.dat in your modloader folder. Do you want to rename?", MODNAME, MB_YESNO | MB_ICONQUESTION);
                    if (result == IDYES)
                    {
                        std::string newName = fileName + ".bak";
                        std::string newPath = parentPath + "\\" + newName;
                        try
                        {
                            std::filesystem::rename(path, newPath);
                        }
                        catch (const std::exception& e)
                        {
                            MessageBox(NULL, ("Failed to rename: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
                        }
                    }
                }
            }
        };

    // Run the traversal for the .dat files
    traverse(GAME_PATH((char*)"modloader"));
}



// <<< MODIFIED PARSEMODLOADER FUNCTION (PARSING ONLY) >>>
void FastLoader::ParseModloader()
{
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

                // object.dat logic removed from here
                // cargrp.dat logic removed from here

                // This function now ONLY parses .fastloader files
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

                        if (gConfig.ReadBoolean("MAIN", "CargrpLoader", true))
                        {
                            CargrpLoader.Parse(line);
                        }
                        if (gConfig.ReadBoolean("MAIN", "ObjDatLoader", true))
                        {
                            ObjDatLoader.Parse(line);
                        }
                        if (gConfig.ReadBoolean("MAIN", "FLAAudioLoader", true))
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