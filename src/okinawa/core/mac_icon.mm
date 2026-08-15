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

bool okSetDockIcon(const unsigned char *rgba, int width, int height) {
  if (rgba == nullptr || width <= 0 || height <= 0) {
    return false;
  }

  @autoreleasepool {
    // The representation borrows the caller's rows rather than copying,
    // so the image has to be drawn from before this returns -- which is
    // what setApplicationIconImage does.
    auto *bytes = const_cast<unsigned char *>(rgba);
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:&bytes
                      pixelsWide:width
                      pixelsHigh:height
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSDeviceRGBColorSpace
                     bytesPerRow:width * 4
                    bitsPerPixel:32];
    if (rep == nil) {
      return false;
    }

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
