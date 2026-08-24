#ifndef GLES_SILENCE_DEPRECATION
#define GLES_SILENCE_DEPRECATION
#endif

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@class minecraftpeViewController;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
@interface minecraftpeAppDelegate : NSObject <UIApplicationDelegate, AVAudioSessionDelegate> {
#pragma clang diagnostic pop
    AVAudioSession* audioSession;
    NSString*       audioSessionSoundCategory;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet minecraftpeViewController *viewController;

// AVAudioSessionDelegate
- (void)beginInterruption;
- (void)endInterruption;

+ (void) initialize;

@end
