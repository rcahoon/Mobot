#include "read_stream.h"

#include <iostream>

#include <curl/curl.h>
#include <opencv2/imgcodecs.hpp>

#include "mpngdec.h"
#include "string_utils.h"

struct Context {
  MPNGDemuxContextPtr demux{nullptr, [](MPNGDemuxContext*) {}};
  ImageCallback callback;
};

static size_t headerCallback(const char *contents,
                             size_t size,
                             size_t nitems,
                             void *userdata) {
  const size_t realsize = size * nitems;
  std::string_view header(contents, realsize);
  trimRight(&header);
  Context* context = (Context*)userdata;

  std::string_view tag, value;
  if (!splitTagValue(header, &tag, &value)) {
    return realsize;
  }
  if (value.empty() || tag.empty()) {
    return realsize;
  }

  if (strcasecmp(tag, "Content-Type")) {
    context->demux = mpngCreate(value);
    if (!context->demux) {
      std::cerr << "Failed to initialize demuxer\n";
      return 0;
    }
  }
  return realsize;
}

static size_t dataCallback(const char *contents,
                           size_t size,
                           size_t nitems,
                           void *userdata) {
  const size_t realsize = size * nitems;
  const std::string_view data(contents, realsize);
  Context* context = (Context*)userdata;
  if (!context->demux) {
    return 0;
  }

  if (mpngReadPacket(*context->demux,
                     data,
                     [context](std::string_view img_data,
                               std::string_view mime_type) {
                       cv::Mat img = cv::imdecode(
                           {img_data.data(), (int)img_data.size()},
                           cv::IMREAD_COLOR);
                       if (img.empty()) {
                         std::cerr << "Error decoding frame\n";
                         return true;
                       }
                       return context->callback(std::move(img));
                     })) {
    return realsize;
  } else {
    return 0;
  }
}
 
void readStream(const std::string& http_address, ImageCallback callback) {
  Context context;
  context.callback = std::move(callback);
 
  curl_global_init(CURL_GLOBAL_ALL);
 
  CURL* curl_handle = curl_easy_init();
 
  curl_easy_setopt(curl_handle, CURLOPT_URL, http_address.c_str());
 
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &dataCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &context);
  curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, &headerCallback);
  curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &context);
 
  if (auto res = curl_easy_perform(curl_handle); res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
  }
 
  curl_easy_cleanup(curl_handle);
 
  curl_global_cleanup();
}
