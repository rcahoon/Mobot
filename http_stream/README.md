The code in this directory can be used to decode a Motion PNG stream received
from an HTTP server into a series of OpenCV images.

The `stream_main` example application will display the images in a GUI window.

Motion PNG uses the same stream format as
[Motion JPEG](https://en.wikipedia.org/wiki/Motion_JPEG), but encodes each frame
as a PNG instead of JPEG in order to avoid the artifacts introduced by JPEG's
lossy compression. The stream is encoded following the multipart MIME standard,
with each frame added as a separate part in the stream.

The HTTP response begins with a standard HTTP header, including the multipart
MIME type in the Content-Type header. The MIME type includes a distinctive
string called the boundary which will be used to separate the parts of the
stream that follow; the separator between parts consists of a \r\n newline,
followed by two dashes, followed by the boundary string, followed by another
\r\n newline. Each part also starts with its own set of HTTP headers; this
should include a Content-Type for the part (which will be image/png since the
frames are PNGs).

```
HTTP/1.0 200 OK
Content-Type: multipart/x-mixed-replace; boundary=Xs9O5Ia8i7hrejvlvHU4

--Xs9O5Ia8i7hrejvlvHU4
Content-Type: image/png

<binary image data for frame 1>
--Xs9O5Ia8i7hrejvlvHU4
Content-Type: image/png

<binary image data for frame 2>
--Xs9O5Ia8i7hrejvlvHU4

...
```

This library can be used to receive the camera stream published by the
[Mobot simulator](https://github.com/rcahoon/MobotSim_public).
