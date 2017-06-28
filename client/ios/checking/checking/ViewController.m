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
#import <native_ui/manager.h>
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

struct nview *__root;

@interface ViewController ()

@property (strong, nonatomic) NSTimer *timer;

@end

@implementation ViewController {

    UIView *parent;
    struct nview *nv;
    CADisplayLink *link;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    /*
     * register view controller allocator
     */
    nexec_set_fnf(checking_client_nexec_alloc);
    
    parent = [[TestView alloc] init];
    [self.view addSubview:parent];
    parent.userInteractionEnabled = YES;
    parent.multipleTouchEnabled = NO;
    self.view.multipleTouchEnabled = NO;
    
    CGRect newFrame = [[UIScreen mainScreen] bounds];
    parent.frame      = newFrame;
    
    nv = nview_alloc();
    UIView *v         = (__bridge id)(nv->ptr);
    [parent addSubview:v];
    nview_set_size(nv, (union vec2){newFrame.size.width, newFrame.size.height});
    nview_set_position(nv, (union vec2){newFrame.size.width/2, newFrame.size.height/2});
    __root  = nv;
    nview_set_layout_type(nv, NATIVE_UI_LAYOUT_RELATIVE);
    
    nview_set_user_interaction_enabled(nv, 1);
    {
        struct nparser *parser = nparser_alloc();
        nparser_parse_file(parser, "res/layout/root.xml");
        
        struct nview *view = (struct nview *)
        ((char *)parser->view.next - offsetof(struct nview, parser));
        
        nview_add_child(nv, view);
        
        nview_update_layout(nv);
    }
    link = [CADisplayLink displayLinkWithTarget:self selector:@selector(handleDisplayLink:)];
    link.preferredFramesPerSecond = 60;
    link.paused = NO;
    [link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    
    //    nview_free(nv);
    //    cache_free();
    //    dim_memory();	
}

- (void)handleDisplayLink:(CADisplayLink *)displayLink
{
    nmanager_update(nmanager_shared(), 1.0f / 60);
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
