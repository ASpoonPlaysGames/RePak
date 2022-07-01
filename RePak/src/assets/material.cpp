#include "pch.h"
#include "Assets.h"

void Assets::AddMaterialAsset(std::vector<RPakAssetEntryV7>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
    Debug("Adding matl asset '%s'\n", assetPath);

    uint32_t assetUsesCount = 0; // track how many other assets are used by this asset
    MaterialHeaderV12* mtlHdr = new MaterialHeaderV12();
    std::string sAssetPath = std::string(assetPath);

    std::string type = "skn";
    uint32_t version = 16;

    if (mapEntry.HasMember("type"))
        type = mapEntry["type"].GetStdString();
    else
        Warning("Adding material without an explicitly defined type. Assuming 'skn'...\n");

    // version check
    if (mapEntry.HasMember("version"))
        version = mapEntry["version"].GetInt();
    else
        Warning("Adding material without an explicitly defined version. Assuming '16'... \n");


    std::string sFullAssetRpakPath = "material/" + sAssetPath + "_" + type + ".rpak"; // Make full rpak asset path.

    mtlHdr->AssetGUID = RTech::StringToGuid(sFullAssetRpakPath.c_str()); // Convert full rpak asset path to textureGUID and set it in the material header.

    // Game ignores this field when parsing, retail rpaks also have this as 0. But In-Game its being set to either 0x4, 0x5, 0x9.
    // Based on resolution.
    // 512x512 = 0x5
    // 1024x1024 = 0x4
    // 2048x2048 = 0x9
    //if (mapEntry.HasMember("signature"))
    //    mtlHdr->UnknownSignature = mapEntry["signature"].GetInt();

    if (mapEntry.HasMember("width")) // Set material width.
        mtlHdr->Width = mapEntry["width"].GetInt();

    if (mapEntry.HasMember("height")) // Set material width.
        mtlHdr->Height = mapEntry["height"].GetInt();

    //if (mapEntry.HasMember("flags")) // Set flags properly. Responsible for texture stretching, tiling etc.
    //    mtlHdr->ImageFlags = mapEntry["flags"].GetUint();

    std::string surface = "default";

    // surfaces are defined in scripts/surfaceproperties.rson
    if (mapEntry.HasMember("surface"))
        surface = mapEntry["surface"].GetStdString();

    // Get the size of the texture guid section.
    size_t textureRefSize = 0;

    if (mapEntry.HasMember("textures"))
    {
        textureRefSize = mapEntry["textures"].GetArray().Size() * 8;
    }
    else
    {
        Warning("Trying to add material with no textures. Skipping asset...\n");
        return;
    }

    uint32_t assetPathSize = (sAssetPath.length() + 1);
    uint32_t dataBufSize = (assetPathSize + (assetPathSize % 4)) + (textureRefSize * 2) + (surface.length() + 1);

    // asset header
    _vseginfo_t subhdrinfo = RePak::CreateNewSegment(sizeof(MaterialHeaderV12), 0, 8);

    // asset data
    _vseginfo_t dataseginfo = RePak::CreateNewSegment(dataBufSize, 1, 64);

    char* dataBuf = new char[dataBufSize] {};
    char* tmp = dataBuf;

    // ===============================
    // write the material path into the buffer
    snprintf(dataBuf, sAssetPath.length() + 1, "%s", assetPath);
    uint8_t assetPathAlignment = (assetPathSize % 4);
    dataBuf += sAssetPath.length() + 1 + assetPathAlignment;

    // ===============================
    // add the texture guids to the buffer
    size_t guidPageOffset = sAssetPath.length() + 1 + assetPathAlignment;

    int textureIdx = 0;
    int fileRelationIdx = -1;
    for (auto& it : mapEntry["textures"].GetArray()) // Now we setup the first TextureGUID Map.
    {
        if (it.GetStdString() != "")
        {
            uint64_t textureGUID = RTech::StringToGuid((it.GetStdString() + ".rpak").c_str()); // Convert texture path to guid.
            *(uint64_t*)dataBuf = textureGUID;
            RePak::RegisterGuidDescriptor(dataseginfo.index, guidPageOffset + (textureIdx * sizeof(uint64_t))); // Register GUID descriptor for current texture index.

            if (fileRelationIdx == -1)
                fileRelationIdx = RePak::AddFileRelation(assetEntries->size());
            else
                RePak::AddFileRelation(assetEntries->size());

            RPakAssetEntryV7* txtrAsset = RePak::GetAssetByGuid(assetEntries, textureGUID, nullptr);

            txtrAsset->m_nRelationsStartIdx = fileRelationIdx;
            txtrAsset->m_nRelationsCounts++;

            assetUsesCount++;
        }
        dataBuf += sizeof(uint64_t);
        textureIdx++; // Next texture index coming up.
    }

    textureIdx = 0; // reset index for next TextureGUID Section.
    for (auto& it : mapEntry["textures"].GetArray()) // Now we setup the second TextureGUID Map.
    {
        if (it.GetStdString() != "")
        {
            //uint64_t guid = RTech::StringToGuid((it.GetStdString() + ".rpak").c_str());

            //there should not be anything her so it shall remain 0
            uint64_t guid = 0x0000000000000000;

            *(uint64_t*)dataBuf = guid;
            //RePak::RegisterGuidDescriptor(dataseginfo.index, guidPageOffset + textureRefSize + (textureIdx * sizeof(uint64_t)));

            //RePak::AddFileRelation(assetEntries->size());

            if (guid != 0)
            {
                RPakAssetEntryV7* txtrAsset = RePak::GetAssetByGuid(assetEntries, guid, nullptr);

                txtrAsset->m_nRelationsStartIdx = fileRelationIdx;
                txtrAsset->m_nRelationsCounts++;


                assetUsesCount++; // Next texture index coming up.
            }
        }
        dataBuf += sizeof(uint64_t);
        textureIdx++;
    }

    // ===============================
    // write the surface name into the buffer
    snprintf(dataBuf, surface.length() + 1, "%s", surface.c_str());

    // get the original pointer back so it can be used later for writing the buffer
    dataBuf = tmp;

    // ===============================
    // fill out the rest of the header
    mtlHdr->Name.m_nIndex = dataseginfo.index;
    mtlHdr->Name.m_nOffset = 0;

    mtlHdr->SurfaceName.m_nIndex = dataseginfo.index;
    mtlHdr->SurfaceName.m_nOffset = (sAssetPath.length() + 1) + assetPathAlignment + (textureRefSize * 2);

    RePak::RegisterDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, Name));
    RePak::RegisterDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, SurfaceName));

    // Type Handling
    if (type == "skn_01")
    {
        // I HAVE PROBABLY BROKEN THIS AT SOME POINT - spoon

        //for testing purposes ""sknp"" will be a material with a full suite of textures.

        // These should always be constant (per each material type)
        // There's different versions of these for each material type
        // GUIDRefs[4] is Colpass entry.

        //apex default shader
        /*mtlHdr->GUIDRefs[0] = 0x2B93C99C67CC8B51;
        mtlHdr->GUIDRefs[1] = 0x1EBD063EA03180C7;
        mtlHdr->GUIDRefs[2] = 0xF95A7FA9E8DE1A0E;
        mtlHdr->GUIDRefs[3] = 0x227C27B608B3646B;*/

        mtlHdr->GUIDRefs[0] = 0x39C739E9928E555C;
        mtlHdr->GUIDRefs[1] = 0x67D89B36EDCDDF6E;
        mtlHdr->GUIDRefs[2] = 0x43A9D8D429698B9F;

        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs));
        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs) + 8);
        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs) + 16);

        RePak::AddFileRelation(assetEntries->size(), 3);
        assetUsesCount += 3;

        //mtlHdr->ShaderSetGUID = 0x1D9FFF314E152725;
        mtlHdr->ShaderSetGUID = 0x586783F71E99553D;
    }
    else if (type == "skn")
    {
        mtlHdr->GUIDRefs[0] = 0xA4728358C3B043CA;
        mtlHdr->GUIDRefs[1] = 0x370BABA9D9147F3D;
        mtlHdr->GUIDRefs[2] = 0x12DCE94708487F8C;

        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs));
        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs) + 8);
        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs) + 16);

        RePak::AddFileRelation(assetEntries->size(), 3);
        assetUsesCount += 3;

        mtlHdr->ShaderSetGUID = 0xC3ACAF7F1DC7F389;

        // default flags for skn
        mtlHdr->Flags2 = 0x56000020;

        mtlHdr->unknownSection[0].unkBlock1_1 = 0xF0138004;
        mtlHdr->unknownSection[0].unkBlock1_2 = 0xF0138004;
        mtlHdr->unknownSection[0].unkBlock1_3 = 0xF0138004;
        mtlHdr->unknownSection[0].unkBlock1_4 = 0x00138004;
        mtlHdr->unknownSection[0].unkBlock1_5 = 0x00000004;
        mtlHdr->unknownSection[0].unkBlock1_6 = 0x00060017;

        mtlHdr->unknownSection[1].unkBlock1_1 = 0xF0138004;
        mtlHdr->unknownSection[1].unkBlock1_2 = 0xF0138004;
        mtlHdr->unknownSection[1].unkBlock1_3 = 0xF0138004;
        mtlHdr->unknownSection[1].unkBlock1_4 = 0x00138004;
        mtlHdr->unknownSection[1].unkBlock1_5 = 0x00000004;
        mtlHdr->unknownSection[1].unkBlock1_6 = 0x00060017;
    }
    else if (type == "wldc")
    {

        // THIS IS 'wld' IN TITANFALL (I think)

        // GUIDRefs[4] is Colpass entry which is optional for wldc.
        mtlHdr->GUIDRefs[0] = 0x435FA77E363BEA48; // DepthShadow
        mtlHdr->GUIDRefs[1] = 0xF734F96BE92E0E71; // DepthPrepass
        mtlHdr->GUIDRefs[2] = 0xD306370918620EC0; // DepthVSM
        mtlHdr->GUIDRefs[3] = 0xDAB17AEAD2D3387A; // DepthShadowTight

        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs));
        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs) + 8);
        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs) + 16);
        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs) + 24);

        //RePak::AddFileRelation(assetEntries->size(), 4);
        //assetUsesCount += 4;

        mtlHdr->ShaderSetGUID = 0x4B0F3B4CBD009096;
    }
    else if (type == "gen")
    {
        // These should always be constant (per each material type)
        // There's different versions of these for each material type
        // GUIDRefs[3] is Colpass entry, however loadscreens do not have colpass materials.

        mtlHdr->GUIDRefs[0] = 0x0000000000000000;
        mtlHdr->GUIDRefs[1] = 0x0000000000000000;
        mtlHdr->GUIDRefs[2] = 0x0000000000000000;

        mtlHdr->ShaderSetGUID = 0xA5B8D4E9A3364655;
    }

    RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, ShaderSetGUID));
    RePak::AddFileRelation(assetEntries->size());
    assetUsesCount++;

    // Is this a colpass asset?
    bool bColpass = false;
    if (mapEntry.HasMember("colpass"))
    {
        std::string colpassPath = "material/" + mapEntry["colpass"].GetStdString() + ".rpak";
        mtlHdr->GUIDRefs[3] = RTech::StringToGuid(colpassPath.c_str());

        RePak::RegisterGuidDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, GUIDRefs) + 32);
        RePak::AddFileRelation(assetEntries->size());
        assetUsesCount++;

        bColpass = false;
    }


    mtlHdr->TextureGUIDs.m_nIndex = dataseginfo.index;
    mtlHdr->TextureGUIDs.m_nOffset = guidPageOffset;

    mtlHdr->TextureGUIDs2.m_nIndex = dataseginfo.index;
    mtlHdr->TextureGUIDs2.m_nOffset = guidPageOffset + textureRefSize;

    RePak::RegisterDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, TextureGUIDs));
    RePak::RegisterDescriptor(subhdrinfo.index, offsetof(MaterialHeaderV12, TextureGUIDs2));

    /*mtlHdr->something = 0x72000000;
    mtlHdr->something2 = 0x100000;

    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 8; ++j)
            mtlHdr->UnkSections[i].Unknown5[j] = 0xf0000000;

        uint32_t f1 = bColpass ? 0x5 : 0x17;

        mtlHdr->UnkSections[i].Unknown6 = 4;
        mtlHdr->UnkSections[i].Flags1 = f1;
        mtlHdr->UnkSections[i].Flags2 = 6;
    }*/

    //////////////////////////////////////////
    /// cpu

    // required for accurate colour
    /*unsigned char testData[544] = {
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0xAB, 0xAA, 0x2A, 0x3E, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x1C, 0x46, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
        0x81, 0x95, 0xE3, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xDE, 0x88, 0x1B, 0x3D, 0xDE, 0x88, 0x1B, 0x3D, 0xDE, 0x88, 0x1B, 0x3D,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };*/

    //0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,

    std::uint64_t cpuDataSize = sizeof(MaterialCPUDataV12);

    // cpu data
    _vseginfo_t cpuseginfo = RePak::CreateNewSegment(sizeof(MaterialCPUHeader) + cpuDataSize, 3, 16);

    MaterialCPUHeader cpuhdr{};
    cpuhdr.m_nUnknownRPtr.m_nIndex = cpuseginfo.index;
    cpuhdr.m_nUnknownRPtr.m_nOffset = sizeof(MaterialCPUHeader);
    cpuhdr.m_nDataSize = cpuDataSize;

    RePak::RegisterDescriptor(cpuseginfo.index, 0);

    char* cpuData = new char[sizeof(MaterialCPUHeader) + cpuDataSize];

    // copy header into the start
    memcpy_s(cpuData, 16, &cpuhdr, 16);

    // copy the rest of the data after the header
    MaterialCPUDataV12 cpudata{};



    memcpy_s(cpuData + sizeof(MaterialCPUHeader), cpuDataSize, &cpudata, cpuDataSize);
    //////////////////////////////////////////

    RePak::AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)mtlHdr });
    RePak::AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)dataBuf });
    RePak::AddRawDataBlock({ cpuseginfo.index, cpuseginfo.size, (uint8_t*)cpuData });

    //////////////////////////////////////////

    RPakAssetEntryV7 asset;

    asset.InitAsset(RTech::StringToGuid(sFullAssetRpakPath.c_str()), subhdrinfo.index, 0, subhdrinfo.size, cpuseginfo.index, 0, -1, -1, (std::uint32_t)AssetType::MATL);
    asset.m_nVersion = version;

    asset.m_nPageEnd = cpuseginfo.index + 1;
    asset.unk1 = bColpass ? 7 : 8; // what

    asset.m_nUsesStartIdx = fileRelationIdx;
    asset.m_nUsesCount = assetUsesCount;

    assetEntries->push_back(asset);
}