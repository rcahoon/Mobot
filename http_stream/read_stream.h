#pragma once

#include <functional>
#include <string>

#include <opencv2/core/mat.hpp>

using ImageCallback = std::function<bool(cv::Mat)>;

void readStream(const std::string& http_address, ImageCallback callback);
