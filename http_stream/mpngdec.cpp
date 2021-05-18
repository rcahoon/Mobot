#include "mpngdec.h"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <string>
#include <string_view>

#include "string_utils.h"

class Stream final {
 public:
  bool readUntil(const std::string_view delim, std::string_view* data) {
    *data = std::string_view();
    std::string::size_type p = buffer_.find(delim, iter_);
    if (p == std::string::npos) {
      return false;
    }
    *data = std::string_view(buffer_).substr(iter_, p - iter_);
    iter_ = p + delim.size();
    return true;
  }

  bool read(const std::size_t count, std::string_view* data) {
    *data = std::string_view();
    if (buffer_.size() - iter_ < count) {
      return false;
    }
    *data = std::string_view(buffer_).substr(iter_, count);
    iter_ += count;
    return true;
  }

  std::string& buffer() {
    return buffer_;
  }

  void shrink() {
    buffer_.erase(0, iter_);
    iter_ = 0;
  }

 private:
  std::string buffer_;
  std::string::size_type iter_ = 0;
};

enum class MPNGState {
  kInitPart,
  kReadHeader,
  kReadPayload,
  kReadDelimiter,
  kBadStream,
};

struct MPNGDemuxContext {
  std::string boundary;
  MPNGState state = MPNGState::kBadStream;
  Stream stream;
  std::string_view content_type;
  int payload_size;
};

static bool getLine(Stream& stream, std::string_view* line) {
  if (!stream.readUntil("\r\n", line)) {
    return false;
  }

  trimRight(line);
  return true;
}

static int parseContentLength(const std::string_view value)
{
  int val;
  if (std::from_chars(value.data(),
                      value.data() + value.size(),
                      val,
                      10).ec != std::errc()) {
    return -1;
  }
  return val;
}

static bool parseMultipartHeader(MPNGDemuxContext& mpng) {
  std::string_view line;
  while (getLine(mpng.stream, &line)) {
    if (line.empty()) {
      mpng.state = MPNGState::kReadPayload;
      return true;
    }

    std::string_view tag, value;
    if (!splitTagValue(line, &tag, &value)) {
      mpng.state = MPNGState::kBadStream;
      return true;
    }
    if (value.empty() || tag.empty()) {
      continue;
    }

    if (strcasecmp(tag, "Content-Type")) {
      mpng.content_type = value;
    } else if (strcasecmp(tag, "Content-Length")) {
      const auto size = parseContentLength(value);
      if (size <= 0) {
        std::cerr << "Invalid Content-Length value: " << value << "\n";
        continue;
      }
      mpng.payload_size = size;
    }
  }
  // Reached end of current data without finding end of header.
  return false;
}


static std::string_view mpngGetBoundary(const std::string_view mime_type) {
  // get MIME type, and skip to the first parameter
  std::string_view start = mime_type;
  while (!start.empty()) {
    const auto p = start.find(';');
    if (p == std::string_view::npos) {
      break;
    }
    start.remove_prefix(p + 1);

    trimLeft(&start);

    constexpr std::string_view kParam = "boundary=";
    if (strcasecmp(start.substr(0, kParam.size()), kParam)) {
      auto res = start.substr(kParam.size());
      const auto end = res.find(';');
      if (end != std::string_view::npos) {
        res = res.substr(0, end);
      }

      // some endpoints may enclose the boundary in Content-Type in quotes
      if (res.size() > 2 && res.front() == '"' && res.back() == '"') {
        res.remove_prefix(1);
        res.remove_suffix(1);
      }
      return res;
    }
  }
  return std::string_view();
}

MPNGDemuxContextPtr mpngCreate(const std::string_view mime_type) {
  const std::string_view boundary = mpngGetBoundary(mime_type);
  if (boundary.empty()) {
    return MPNGDemuxContextPtr(nullptr, [](MPNGDemuxContext*) {});
  }
  auto mpng = MPNGDemuxContextPtr(
      new MPNGDemuxContext(), [](MPNGDemuxContext* p) { delete p; });
  mpng->boundary = "\r\n--";
  mpng->boundary += boundary;
  mpng->boundary += "\r\n";
  mpng->state = MPNGState::kInitPart;
  return mpng;
}

bool mpngReadPacket(MPNGDemuxContext& mpng,
                    const std::string_view input,
                    ImageDataCallback callback) {
  mpng.stream.buffer() += input;
  while (true) {
    switch (mpng.state) {
      case MPNGState::kBadStream:
        return false;
      case MPNGState::kInitPart:
        mpng.payload_size = -1;
        mpng.content_type = std::string_view();
        mpng.state = MPNGState::kReadHeader;
        mpng.stream.shrink();
        break;
      case MPNGState::kReadHeader:
        if (!parseMultipartHeader(mpng)) {
          return true;
        }
        break;
      case MPNGState::kReadPayload: {
        std::string_view pkt;
        if (mpng.payload_size > 0) {
          // Size has been provided to us in the MIME header.
          if (!mpng.stream.read(mpng.payload_size, &pkt)) {
            return true;
          }
          // We've read the payload data, but we still need to consume the
          // boundary delimiter.
          mpng.state = MPNGState::kReadDelimiter;
        } else {
          // No size was given, we will read until the next boundary delimiter.
          if (!mpng.stream.readUntil(mpng.boundary, &pkt)) {
            return true;
          }
          // Done reading this part. Continue on to reading the next part.
          mpng.state = MPNGState::kInitPart;
        }

        if (strcasecmp(mpng.content_type, "image/png") ||
            strcasecmp(mpng.content_type, "image/jpeg")) {
          if (!callback(pkt, mpng.content_type)) {
            return false;
          }
        } else {
          std::cerr << "Unexpected Content-Type: " << mpng.content_type << "\n";
        }
      } break;
      case MPNGState::kReadDelimiter: {
        std::string_view scratch;
        if (!mpng.stream.readUntil(mpng.boundary, &scratch)) {
          return true;
        }
        mpng.state = MPNGState::kInitPart;
      } break;
    }
  }
}
