#pragma once

#include <functional>
#include <memory>
#include <string_view>

struct MPNGDemuxContext;

using MPNGDemuxContextPtr = 
    std::unique_ptr<MPNGDemuxContext, void(*)(MPNGDemuxContext*)>;

using ImageDataCallback =
    std::function<bool(std::string_view img_data, std::string_view mime_type)>;

MPNGDemuxContextPtr mpngCreate(const std::string_view mime_type);

bool mpngReadPacket(MPNGDemuxContext& mpng,
                    const std::string_view input,
                    ImageDataCallback callback);
