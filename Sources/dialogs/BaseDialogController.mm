//
//  BaseDialogController.m
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import "BaseDialogController.h"
#import "../minecraftpeViewController.h"
#import "../util/Mth.h"

@implementation BaseDialogController

- (id)init
{
    self = [super init];
    if (self) {
        // Initialization code here.
        _status = -1;
        _listener = nil;
    }
    
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Create a container view for Safe Area mapping
    self.containerView = [[UIView alloc] initWithFrame:self.view.bounds];
    self.containerView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.containerView.backgroundColor = self.view.backgroundColor;
    self.view.backgroundColor = [UIColor blackColor];
    
    // Move all subviews to container
    NSArray *subviews = [self.view.subviews copy];
    for (UIView *subview in subviews) {
        [self.containerView addSubview:subview];
    }
    
    [self.view addSubview:self.containerView];
}

- (void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    
    CGRect safeFrame;
    if (@available(iOS 11.0, *)) {
        safeFrame = UIEdgeInsetsInsetRect(self.view.bounds, self.view.safeAreaInsets);
    } else {
        safeFrame = self.view.bounds;
    }
    
    // Cap aspect ratio to 16:9 for modern ultra-wide devices
    CGFloat targetAspect = 16.0 / 9.0;
    CGFloat currentAspect = safeFrame.size.width / safeFrame.size.height;
    CGRect newFrame = safeFrame;
    
    if (currentAspect > targetAspect) {
        CGFloat newWidth = safeFrame.size.height * targetAspect;
        newFrame.origin.x += (safeFrame.size.width - newWidth) / 2.0;
        newFrame.size.width = newWidth;
    }
    
    self.containerView.frame = newFrame;
}


//
// Interface
//
- (void)addToView:(UIView*)parentView {
	
	// Add this view as a subview of EAGLView
	[parentView addSubview:self.view];
    
	// ...then fade it in using core animation
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	[UIView beginAnimations:nil context:NULL];
	//self.view.alpha = 1.0f;
	[UIView commitAnimations];
#pragma clang diagnostic pop
}

- (int) getUserInputStatus { return _status; }

-(std::vector<std::string>)getUserInput
{
    return _strings;
}

-(void)setListener:(minecraftpeViewController*)listener
{
    _listener = listener;
}

//
// Helpers
//
- (void) setOk {
    _status = 1;
}

- (void) setCancel {
    _status = 0;
}

- (void) closeOk {
    [self setOk];
    NSLog(@"Close dialog %p\n", _listener);
    [_listener closeDialog];
}

- (void) closeCancel {
    [self setCancel];
    [_listener closeDialog];
}

- (void) addString: (std::string) s {
    _strings.push_back(s);
}

@end
