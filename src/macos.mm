/*
 * Copyright (c) 2019-2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "macos.h"
#import <AppKit/NSApplication.h>
#import <AppKit/NSDockTile.h>
#import <AppKit/NSImageView.h>
#import <AppKit/NSProgressIndicator.h>
#import <AppKit/NSScreen.h>
#import <AppKit/NSView.h>
#import <AppKit/NSWindow.h>
#import <objc/runtime.h>

#include <atomic>

void removeMacosTabBar()
{
#ifdef AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER
    // Remove View > Show Tab Bar.
    if ([NSWindow respondsToSelector:@selector(allowsAutomaticWindowTabbing)])
        NSWindow.allowsAutomaticWindowTabbing = NO;
#endif
}

static NSProgressIndicator *dockProgressBar()
{
    static NSProgressIndicator *s_pi = nil;
    if (!s_pi) {
        NSDockTile *tile = NSApp.dockTile;
        NSView *contentView = tile.contentView;
        if (!contentView) {
            contentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, tile.size.width, tile.size.height)];
            NSImageView *iconView = [[NSImageView alloc] initWithFrame:contentView.bounds];
            iconView.image = NSApp.applicationIconImage;
            [contentView addSubview:iconView];
            tile.contentView = contentView;
        }
        s_pi = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(0, 4, contentView.frame.size.width, 18)];
        s_pi.style = NSProgressIndicatorStyleBar;
        s_pi.minValue = 0;
        s_pi.maxValue = 100;
        s_pi.indeterminate = NO;
        [contentView addSubview:s_pi];
    }
    return s_pi;
}

void macosHideFromDock()
{
    [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
}

void macosSetDockProgress(int percent)
{
    NSProgressIndicator *pi = dockProgressBar();
    pi.doubleValue = percent;
    pi.hidden = NO;
    NSApp.dockTile.badgeLabel = @"";
    [NSApp.dockTile display];
}

void macosPauseDockProgress(int percent)
{
    NSProgressIndicator *pi = dockProgressBar();
    pi.doubleValue = percent;
    pi.hidden = NO;
    NSApp.dockTile.badgeLabel = @"\u23F8";
    [NSApp.dockTile display];
}

void macosResetDockProgress()
{
    dockProgressBar().hidden = YES;
    NSApp.dockTile.badgeLabel = @"";
    [NSApp.dockTile display];
}

void macosFinishDockProgress(bool isSuccess, bool stopped)
{
    dockProgressBar().hidden = YES;
    if (isSuccess) {
        NSApp.dockTile.badgeLabel = @"";
        [NSApp.dockTile display];
        [NSApp requestUserAttention:NSInformationalRequest];
    } else if (stopped) {
        NSApp.dockTile.badgeLabel = @"\u25A0";
        [NSApp.dockTile display];
    } else {
        NSApp.dockTile.badgeLabel = @"!";
        [NSApp.dockTile display];
        [NSApp requestUserAttention:NSCriticalRequest];
    }
}

// ---------------------------------------------------------------------------
// EDR headroom override via method swizzle on NSScreen
// ---------------------------------------------------------------------------
static std::atomic<bool> s_edrOverrideEnabled{false};
static bool s_swizzled = false;

// Category that holds the swizzled implementation.
@interface NSScreen (ShotcutEdrOverride)
- (CGFloat)shotcut_maximumExtendedDynamicRangeColorComponentValue;
@end

@implementation NSScreen (ShotcutEdrOverride)
- (CGFloat)shotcut_maximumExtendedDynamicRangeColorComponentValue
{
    // After swizzling, calling the swizzled selector invokes the ORIGINAL impl.
    CGFloat real = [self shotcut_maximumExtendedDynamicRangeColorComponentValue];
    if (s_edrOverrideEnabled.load(std::memory_order_relaxed) && real < 2.0) {
        return self.maximumPotentialExtendedDynamicRangeColorComponentValue;
    }
    return real;
}
@end

void macosOverrideEdrHeadroom(bool enable)
{
    if (!s_swizzled) {
        s_swizzled = true;
        Method original = class_getInstanceMethod(
            [NSScreen class],
            @selector(maximumExtendedDynamicRangeColorComponentValue));
        Method swizzled = class_getInstanceMethod(
            [NSScreen class],
            @selector(shotcut_maximumExtendedDynamicRangeColorComponentValue));
        method_exchangeImplementations(original, swizzled);
        NSLog(@"macosOverrideEdrHeadroom: swizzled NSScreen.maximumExtendedDynamicRangeColorComponentValue");
    }
    s_edrOverrideEnabled.store(enable, std::memory_order_relaxed);
    NSLog(@"macosOverrideEdrHeadroom: %s", enable ? "enabled" : "disabled");
}

float macosCurrentEdrHeadroom(uintptr_t windowId)
{
    NSView *view = reinterpret_cast<NSView *>(windowId);
    if (!view || !view.window || !view.window.screen)
        return 1.0f;
    return static_cast<float>(view.window.screen.maximumExtendedDynamicRangeColorComponentValue);
}

float macosPotentialEdrHeadroom(uintptr_t windowId)
{
    NSView *view = reinterpret_cast<NSView *>(windowId);
    if (!view || !view.window || !view.window.screen)
        return 1.0f;
    return static_cast<float>(view.window.screen.maximumPotentialExtendedDynamicRangeColorComponentValue);
}

float macosReferenceEdrHeadroom(uintptr_t windowId)
{
    NSView *view = reinterpret_cast<NSView *>(windowId);
    if (!view || !view.window || !view.window.screen)
        return 1.0f;
    if (@available(macOS 12.0, *))
        return static_cast<float>(view.window.screen.maximumReferenceExtendedDynamicRangeColorComponentValue);
    return 1.0f;
}
