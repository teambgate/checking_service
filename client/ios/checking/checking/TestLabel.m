//
//  TestLabel.m
//  TestUI
//
//  Created by Apple on 6/5/17.
//  Copyright © 2017 BGATE. All rights reserved.
//

#import "TestLabel.h"
#import <native_ui/util.h>

@interface TestLabel ()

@end

@implementation TestLabel {
}

- (void) attachTapHandler
{
    [self setUserInteractionEnabled:YES];
    UIGestureRecognizer *touchy = [[UITapGestureRecognizer alloc]
                                   initWithTarget:self action:@selector(handleTap:)];
    [self addGestureRecognizer:touchy];
}

- (id) init
{
    self = [super init];
    [self attachTapHandler];
    return self;
}

- (void) copy: (id) sender
{
    UIPasteboard *pboard = [UIPasteboard generalPasteboard];
    pboard.string = self.text;
}

- (BOOL) canPerformAction: (SEL) action withSender: (id) sender
{
    return (action == @selector(copy:));
}

- (void) handleTap: (UIGestureRecognizer*) recognizer
{
    [self becomeFirstResponder];
    UIMenuController *menu = [UIMenuController sharedMenuController];
    [menu setTargetRect:self.frame inView:self.superview];
    [menu setMenuVisible:YES animated:YES];
}

-(BOOL)canBecomeFirstResponder
{
    return YES;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
    return custom_his_test(self, point, event);
}


@end
