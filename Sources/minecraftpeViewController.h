#ifndef GLES_SILENCE_DEPRECATION
#define GLES_SILENCE_DEPRECATION
#endif

#import <UIKit/UIKit.h>

#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#import "Sources/App.h"
#import "Sources/AppPlatform_iOS.h"
#import "Sources/NinecraftApp.h"
#import "ShowKeyboardView.h"

#import "Sources/InAppSettingsKit/Controllers/IASKAppSettingsViewController.h"

@class EAGLContext;
@class EAGLView;
//@class App;
//@class AppContext;
//@class AppPlatform_iOS;
@class BaseDialogController;

@interface minecraftpeViewController : UIViewController<IASKSettingsDelegate> {
    EAGLContext *context;
    
    // App and AppPlatform
    App* _app;
    AppContext* _context;
    AppPlatform_iOS* _platform;   
    
    UITouch** _touchMap;
    
    BOOL animating;
    NSInteger animationFrameInterval;
    CADisplayLink *displayLink;
    
    BaseDialogController* _dialog;
    
    int _dialogResultStatus;
    std::vector<std::string> _dialogResultStrings;
    
    ShowKeyboardView* _keyboardView;

    @public
    float viewScale;
}

@property (nonatomic, retain) EAGLView *glView;
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

@property (nonatomic, retain) IASKAppSettingsViewController *appSettingsViewController;


- (void)startAnimation;
- (void)stopAnimation;
- (void)enteredBackground;

- (void)setAudioEnabled:(BOOL)status;

-(int) getUserInputStatus;
-(std::vector<std::string>)getUserInput;

- (void)showDialog_CreateWorld;
- (void)showDialog_MainMenuOptions;
- (void)showDialog_RenameMPWorld;
- (void)showDialog_SetUsername;

- (void)showKeyboard;
- (void)hideKeyboard;

- (void) closeDialog;
- (BaseDialogController*) dialog;

+ (void) initialize;

@end
