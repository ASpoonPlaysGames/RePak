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

    // create the segment for the subheader
    _vseginfo_t subhdrinfo = pak->CreateNewSegment(sizeof(RuiHeader), SF_HEAD | SF_CLIENT, 8);
    // make the subheader
    RuiHeader* pHdr = new RuiHeader();

    ////////////////
    // ASSET NAME
    // the asset name, without the file extension or anything like that
    std::string name = std::filesystem::path(assetPath).stem().string();
    // create a buffer for the name segment
    char* namebuf = new char[name.size() + 1];
    sprintf_s(namebuf, name.length() + 1, "%s", name.c_str());
    Log("-> asset name: '%s'\n", namebuf);

    // create the segment for the name
    _vseginfo_t assetNameInfo{};
    assetNameInfo = pak->CreateNewSegment(name.size() + 1, SF_CLIENT | SF_CPU, 1);

    ////////////////////////////
    // ASSET CONSTS/ARGS/VARS

    // GET THE SIZE OF THE STRUCT AND VALUES

    size_t constsSize = 0;
    size_t stringsSize = 0;

    // loop through the consts and get the total size needed
    for (auto& it : mapEntry["consts"].GetArray())
    {
        std::string typeStr = it["type"].GetStdString();
        RuiArgumentType_t type = ruiArgTypeFromStr[typeStr];
        constsSize += ruiArgSizeFromType[type];

        switch (type)
        {
        case (TYPE_STRING):
        case (TYPE_ASSET):
        case (TYPE_UIHANDLE):
            stringsSize += it["value"].GetStdString().length() + 1;
        }

    }
    Log("-> consts size: 0x%x\n", constsSize);

    size_t argsSize = 0;

    // loop through the args and get the total size needed
    for (auto& it : mapEntry["args"].GetArray())
    {
        std::string typeStr = it["type"].GetStdString();
        RuiArgumentType_t type = ruiArgTypeFromStr[typeStr];
        argsSize += ruiArgSizeFromType[type];

        switch (type)
        {
        case (TYPE_STRING):
        case (TYPE_ASSET):
        case (TYPE_UIHANDLE):
            stringsSize += it["defaultValue"].GetStdString().length() + 1;
        }
    }
    Log("-> args size: 0x%x\n", argsSize);

    size_t varsSize = 0;

    // loop through the vars and get the total size needed
    for (auto& it : mapEntry["vars"].GetArray())
    {
        std::string typeStr = it["type"].GetStdString();
        RuiArgumentType_t type = ruiArgTypeFromStr[typeStr];
        varsSize += ruiArgSizeFromType[type];
    }
    Log("-> vars size: 0x%x\n", varsSize);

    // WRITE THE CONSTS AND ARGS

    // create buffer for values
    char* valuesBuf = new char[constsSize + argsSize]{};
    rmem vBuf(valuesBuf);

    // create buffer for default strings
    char* stringsBuf = new char[stringsSize + 1]{};
    uint32_t curStringOffset = 0;

    // create the seginfo for the values
    _vseginfo_t valuesInfo{};
    valuesInfo = pak->CreateNewSegment(constsSize + argsSize, SF_CLIENT | SF_CPU, 1);

    // create the seginfo for the default strings
    _vseginfo_t stringsInfo{};
    stringsInfo = pak->CreateNewSegment(stringsSize, SF_CLIENT | SF_CPU, 1);

    // create map for consts and their offsets into vBuf
    std::map<std::string, short> valuesOffsets = {};
    short curValueOffset = 0;
    
    for (auto& it : mapEntry["consts"].GetArray())
    {
        std::string nameStr = it["name"].GetStdString();
        std::string typeStr = it["type"].GetStdString();

        // get the type and size of the const
        RuiArgumentType_t type = ruiArgTypeFromStr[typeStr];
        short size = ruiArgSizeFromType[type];

        valuesOffsets[nameStr] = curValueOffset;
        curValueOffset += size;

        switch (type)
        {
        case (TYPE_STRING):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_ASSET):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_BOOL):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_INT):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_FLOAT):
            vBuf.write<float>(it["value"].GetFloat());
            break;
        case (TYPE_FLOAT2):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_FLOAT3):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_COLOR_ALPHA):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_GAMETIME):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_WALLTIME):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_UIHANDLE):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_IMAGE):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_FONT_FACE):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_FONT_HASH):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_ARRAY):
            Error("const '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        default:
            Error("const '%s' is invalid type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        }
    }

    for (auto& it : mapEntry["args"].GetArray())
    {
        std::string nameStr = it["name"].GetStdString();
        std::string typeStr = it["type"].GetStdString();

        // get the type and size of the const
        RuiArgumentType_t type = ruiArgTypeFromStr[typeStr];
        short size = ruiArgSizeFromType[type];

        valuesOffsets[nameStr] = curValueOffset;
        

        // cant initialise in switch statement
        RPakPtr ptr = {};
        std::string str;

        switch (type)
        {
        case (TYPE_STRING):
            

            break;
        case (TYPE_ASSET):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_BOOL):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_INT):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_FLOAT):
            vBuf.write<float>(it["defaultValue"].GetFloat());
            break;
        case (TYPE_FLOAT2):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_FLOAT3):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_COLOR_ALPHA):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_GAMETIME):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_WALLTIME):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_UIHANDLE):
            
            pak->AddPointer(valuesInfo.index, curValueOffset);
            ptr = { stringsInfo.index, curStringOffset };
            str = it["defaultValue"].GetStdString();
            sprintf_s(stringsBuf + curStringOffset, str.length() + 1, "%s", str.c_str());
            curStringOffset += str.length() + 1;
            vBuf.write<RPakPtr>(ptr);
            
            break;
        case (TYPE_IMAGE):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_FONT_FACE):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_FONT_HASH):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        case (TYPE_ARRAY):
            Error("arg '%s' is unimplemented type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        default:
            Error("arg '%s' is invalid type '%s'", nameStr.c_str(), typeStr.c_str());
            break;
        }

        curValueOffset += size;
    }

    // create the seginfo for the args
    _vseginfo_t argsInfo{};
    // for now, just contains a single arg cluster and all of the args
    argsInfo = pak->CreateNewSegment(sizeof(RuiArgCluster) + sizeof(RuiArg) * mapEntry["args"].GetArray().Size(), SF_CLIENT | SF_CPU, 1);
    
    char* argsBuf = new char[sizeof(RuiArgCluster) + sizeof(RuiArg) * mapEntry["args"].GetArray().Size()]{};
    rmem aBuf(argsBuf);

    // write the arg cluster
    RuiArgCluster* cluster = new RuiArgCluster();
    cluster->argCount = mapEntry["args"].GetArray().Size();
    cluster->argIndex = 0;

    aBuf.write<RuiArgCluster>(*cluster);

    // write the args
    for (auto& it : mapEntry["args"].GetArray())
    {
        RuiArg* arg = new RuiArg();
        // short hash, fuuuuuuuuuuuuck
    }

    
    //////////////////////
    // ASSET SUBHEADER
    

    // set the width and height
    pHdr->elementWidth = mapEntry["width"].GetFloat();
    pHdr->elementWidthRatio = 1 / pHdr->elementWidth;
    pHdr->elementHeight = mapEntry["height"].GetFloat();
    pHdr->elementHeightRatio = 1 / pHdr->elementHeight;

    // set args size and total struct size
    pHdr->valueSize = constsSize + argsSize;
    pHdr->structSize = pHdr->valueSize + varsSize;

    // register the pointer to the name
    pak->AddPointer(subhdrinfo.index, offsetof(RuiHeader, name));
    // set the pointer to the asset name
    pHdr->name = { assetNameInfo.index, 0 };

    // register the pointer to the values
    pak->AddPointer(valuesInfo.index, offsetof(RuiHeader, values));
    // set the pointer to the asset values
    pHdr->values = { valuesInfo.index, 0 };
    


    ////////////////////
    // FINISHING UP
    // add the raw data blocks in order

    // subheader
    RPakRawDataBlock shdb{ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr };
    pak->AddRawDataBlock(shdb);

    // asset name
    RPakRawDataBlock andb{ assetNameInfo.index, assetNameInfo.size, (uint8_t*)namebuf };
    pak->AddRawDataBlock(andb);

    // values
    RPakRawDataBlock vdb{ valuesInfo.index, valuesInfo.size, (uint8_t*)valuesBuf };
    pak->AddRawDataBlock(vdb);

    // default string values
    RPakRawDataBlock sdb{ stringsInfo.index, stringsInfo.size, (uint8_t*)stringsBuf };
    pak->AddRawDataBlock(sdb);

    // create and init the asset entry
    RPakAssetEntry asset;
    asset.InitAsset(RTech::StringToGuid((std::string(assetPath) + ".rpak").c_str()), subhdrinfo.index, 0, subhdrinfo.size, assetNameInfo.index, 0, -1, -1, (std::uint32_t)AssetType::UI);
    asset.version = 0x1E;

    asset.pageEnd = assetNameInfo.index + 1; // number of the highest page that the asset references pageidx + 1
    asset.unk1 = 1;

    // add the asset entry
    assetEntries->push_back(asset);
}
