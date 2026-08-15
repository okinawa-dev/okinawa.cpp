// The macOS half of OkCore::setWindowIcon.
//
// A window on this platform carries no icon, so glfwSetWindowIcon does
// nothing here and the picture has to go somewhere else: the Dock tile,
// which is the application's icon while it runs. An application bundle
// says the same thing statically, through Contents/Resources and
// CFBundleIconFile, and this says it at runtime -- which is what a
// binary run straight from a build directory needs, since it has no
// bundle to read.
//
// Objective-C++ because AppKit is Objective-C and there is no C way in.
// Kept to one function with a plain C++ signature so nothing else in
// the engine has to know that. Built with ARC, so nothing here retains
// or releases by hand.

#import <Cocoa/Cocoa.h>

#include <cstring>

bool okSetDockIcon(const unsigned char *rgba, int width, int height) {
  if (rgba == nullptr || width <= 0 || height <= 0) {
    return false;
  }

  @autoreleasepool {
    // The representation is given its OWN storage and the pixels are
    // copied into it, rather than being pointed at the caller's buffer.
    // AppKit keeps the icon image for as long as the application lives
    // and reads it again whenever it redraws the tile -- including on
    // the way out -- while the caller's pixels are freed as soon as it
    // returns. Lending them out is a use-after-free that only shows
    // itself later, somewhere else, as a crash while quitting.
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:nullptr
                      pixelsWide:width
                      pixelsHigh:height
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSDeviceRGBColorSpace
                     bytesPerRow:static_cast<NSInteger>(width) * 4
                    bitsPerPixel:32];
    if (rep == nil) {
      return false;
    }
    std::memcpy([rep bitmapData], rgba,
                static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

    NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
    [image addRepresentation:rep];

    // NSApp is nil until the application object exists, which GLFW
    // creates when it initializes. Called before that, this would
    // silently do nothing, so it says so instead.
    if (NSApp == nil) {
      return false;
    }
    [NSApp setApplicationIconImage:image];
  }
  return true;
}
