#include "pch.h"
#include "additionaltxd.h"
#include <CTxdStore.h>
#include <CStreaming.h>

using f_RwTexDictionaryFindNamedTexture = RwTexture * (*__cdecl)(RwTexDictionary* dict, const char* name);
using f_AssignRemapTxd = int (*__cdecl)(const char* txdName, __int16 txdID);
f_RwTexDictionaryFindNamedTexture ogFindNamedTex;
f_AssignRemapTxd ogAssignRemapTex;

RwTexture* CAdditionalTXD::hkRwTexDictionaryFindNamedTexture(RwTexDictionary* dict, const char* name)
{
    // 1. Najpierw szukamy w oryginalnym słowniku (tak jak robi to gra)
    RwTexture* pTex = ogFindNamedTex(dict, name);

    // 2. Jeśli tekstury brak i mamy załadowane dodatkowe TXD (fastloader), szukamy w nich
    if (!pTex && bAdditionalTxdUsed)
    {
        for (auto& extraDict : TxdDictStore)
        {
            if (extraDict)
            {
                pTex = RwTexDictionaryFindNamedTexture(extraDict, name);
                if (pTex)
                {
                    break; // Znaleziono teksturę w fastloaderze
                }
            }
        }
    }

    return pTex;
}

void CAdditionalTXD::hkAssignRemapTxd(const char* txdName, uint16_t txdId)
{
    if (!txdName)
    {
        return;
    }

    // Sprawdź, czy nazwa TXD zaczyna się od "fastloader" (np. fastloader.txd, fastloader1.txd)
    // 10 to długość słowa "fastloader"
    if (!strncmp(txdName, "fastloader", 10))
    {
        // Dodaj ID do listy do załadowania
        TxdIDStore.push_back(txdId);

        // Zwiększ licznik referencji, aby gra nie usunęła tego TXD
        CTxdStore::AddRef(txdId);

        bAdditionalTxdUsed = true;
    }
    else
    {
        // Jeśli to nie jest fastloader, wykonaj standardową funkcję gry
        ogAssignRemapTex(txdName, txdId);
    }
}

void CAdditionalTXD::Load()
{
    if (bAdditionalTxdUsed)
    {
        // 1. Zgłoś żądanie załadowania wszystkich znalezionych plików fastloader
        for (auto& id : TxdIDStore)
        {
            CStreaming::RequestTxdModel(id, (eStreamingFlags::PRIORITY_REQUEST | eStreamingFlags::KEEP_IN_MEMORY));
        }

        // 2. Wymuś natychmiastowe załadowanie
        CStreaming::LoadAllRequestedModels(true);

        // 3. Pobierz wskaźniki do załadowanych słowników TXD, aby używać ich później w hooku
        for (auto& id : TxdIDStore)
        {
            // 0x408340 to adres funkcji CTxdStore::GetTxd (getTexDictionary)
            auto* dict = ((RwTexDictionary * (__cdecl*)(int))0x408340)(id);
            if (dict) {
                TxdDictStore.push_back(dict);
            }
        }
    }
}

void CAdditionalTXD::Init()
{
    plugin::Events::initRwEvent.after += []()
        {
            // Hook funkcji szukającej tekstury (RwTexDictionaryFindNamedTexture)
            ogFindNamedTex = (f_RwTexDictionaryFindNamedTexture)plugin::patch::GetPointer(0x731733 + 1);
            ogFindNamedTex = f_RwTexDictionaryFindNamedTexture((int)ogFindNamedTex + (plugin::GetGlobalAddress(0x731733) + 5));
            plugin::patch::RedirectCall(0x731733, hkRwTexDictionaryFindNamedTexture, true);

            // Hook funkcji rejestrującej TXD (CModelInfo::AddTextrues / AssignRemap)
            ogAssignRemapTex = (f_AssignRemapTxd)plugin::patch::GetPointer(0x5B62C2 + 1);
            ogAssignRemapTex = f_AssignRemapTxd((int)ogAssignRemapTex + (plugin::GetGlobalAddress(0x5B62C2) + 5));
            plugin::patch::RedirectCall(0x5B62C2, hkAssignRemapTxd, true);
        };

    plugin::Events::initScriptsEvent += []()
        {
            Load();
        };
}