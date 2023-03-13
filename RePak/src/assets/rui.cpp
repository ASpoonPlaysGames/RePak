#include "pch.h"
#include "Assets.h"
#include <dxutils.h>

void Assets::AddRuiAsset(CPakFile* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
    Log("Adding rui asset '%s'\n", assetPath);

    // data segment order:
    // name
    // values
    // default string values
    // transformFuncs
    // unk8 stuff
    // unk9 stuff
    // argClusters
    // args
    // unk10 data
    // unk10 subdata



    // make the header
    RuiHeader hdr = {};


}
