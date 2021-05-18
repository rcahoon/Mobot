#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>

#include "read_stream.h"

int main(int, char**) {
  const std::string videoStreamAddress = "http://127.0.0.1:7663/video.mpng";

  readStream(videoStreamAddress, [](cv::Mat image) {
    cv::imshow("Output Window", image);
    const bool should_exit = (cv::waitKey(1) & 0xff) < 128;
    return !should_exit;
  });
}
