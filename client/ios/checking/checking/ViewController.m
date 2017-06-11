//
//  ViewController.m
//  TestUI
//
//  Created by Apple on 5/20/17.
//  Copyright © 2017 BGATE. All rights reserved.
//

#import "ViewController.h"
#import <native_ui/view.h>
#import <native_ui/view_controller.h>
#import <native_ui/align.h>
#import <native_ui/native_ui_manager.h>
#import <native_ui/parser.h>
#import <native_ui/action.h>
#import <native_ui/touch_handle.h>
#import "TestView.h"
#import "TestLabel.h"
#import <cherry/stdio.h>
#import <cherry/memory.h>
#import <cherry/array.h>
#import <cherry/string.h>
#import <cherry/map.h>
#import <cherry/math/math.h>
#import <checking_client/controller_utils.h>

#define DEGREES_TO_RADIANS(angle) ((angle) / 180.0 * M_PI)

struct native_view *__root;

@interface ViewController ()

@property (strong, nonatomic) NSTimer *timer;

@end

@implementation ViewController {

    UIView *parent;
    struct native_view *nv;
    CADisplayLink *link;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    /*
     * register view controller allocator
     */
    native_view_controller_set_from_name_delegate(checking_client_native_view_controller_alloc);
    
    parent = [[TestView alloc] init];
    [self.view addSubview:parent];
    parent.userInteractionEnabled = YES;
    parent.multipleTouchEnabled = NO;
    self.view.multipleTouchEnabled = NO;
    
    CGRect newFrame = [[UIScreen mainScreen] bounds];
    parent.frame      = newFrame;
    
    nv = native_view_alloc();
    UIView *v         = (__bridge id)(nv->ptr);
    [parent addSubview:v];
    native_view_set_size(nv, (union vec2){newFrame.size.width, newFrame.size.height});
    native_view_set_position(nv, (union vec2){newFrame.size.width/2, newFrame.size.height/2});
    __root  = nv;
    native_view_set_layout_type(nv, NATIVE_UI_LAYOUT_RELATIVE);
    
    native_view_set_user_interaction_enabled(nv, 1);
    {
        struct native_view_parser *parser = native_view_parser_alloc();
        native_view_parser_parse_file(parser, "res/layout/root.xml");
        
        struct native_view *view = (struct native_view *)
        ((char *)parser->view.next - offsetof(struct native_view, parser));
        
        native_view_add_child(nv, view);
        
        native_view_update_layout(nv);
    }
    link = [CADisplayLink displayLinkWithTarget:self selector:@selector(handleDisplayLink:)];
    link.preferredFramesPerSecond = 60;
    link.paused = NO;
    [link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    
    //    native_view_free(nv);
    //    cache_free();
    //    dim_memory();	
}

- (void)handleDisplayLink:(CADisplayLink *)displayLink
{
    native_ui_manager_update(native_ui_manager_shared(), 1.0f / 60);
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
