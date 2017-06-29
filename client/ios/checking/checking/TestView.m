//
//  TestView.m
//  TestUI
//
//  Created by Apple on 5/24/17.
//  Copyright © 2017 BGATE. All rights reserved.
//

#import "TestView.h"
#import <native_ui/util.h>

@implementation TestView

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
    return custom_his_test(self, point, event);
}

@end
