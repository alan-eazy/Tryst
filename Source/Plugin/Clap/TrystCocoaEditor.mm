#import <Cocoa/Cocoa.h>

#include "TrystCocoaEditor.h"
#include "Plugin/PluginEngine.h"

#include <cstdint>
#include <new>
#include <string>
#include <string_view>
#include <vector>

@class TrystEditorController;

namespace tryst::plugin::cocoa
{
namespace
{
struct Binding
{
    std::uint32_t parameterId = 0;
    __unsafe_unretained NSControl* control = nil;
    bool toggle = false;
};
}

struct Editor
{
    PluginEngine* engine = nullptr;
    void* callbackContext = nullptr;
    ParameterChangedCallback callback = nullptr;
    __strong NSView* root = nil;
    __strong TrystEditorController* controller = nil;
    __strong NSTimer* timer = nil;
    std::vector<Binding> bindings;
};
}

@interface TrystBackgroundView : NSView
@end

@implementation TrystBackgroundView
- (void)drawRect:(NSRect)dirtyRect
{
    [[NSColor colorWithSRGBRed:0.055 green:0.047 blue:0.085 alpha:1.0] setFill];
    NSRectFill(dirtyRect);

    [[NSColor colorWithSRGBRed:0.27 green:0.21 blue:0.44 alpha:1.0] setStroke];
    NSBezierPath* path = [NSBezierPath bezierPath];
    [path moveToPoint:NSMakePoint(20.0, 324.0)];
    [path lineToPoint:NSMakePoint(880.0, 324.0)];
    path.lineWidth = 1.0;
    [path stroke];
}
@end

@interface TrystEditorController : NSObject
{
@public
    tryst::plugin::cocoa::Editor* editor;
}
- (void)controlChanged:(id)sender;
- (void)timerFired:(NSTimer*)timer;
@end

@implementation TrystEditorController
- (void)controlChanged:(id)sender
{
    if (editor == nullptr || editor->callback == nullptr || ![sender isKindOfClass:[NSControl class]])
        return;

    NSControl* control = (NSControl*)sender;
    const auto parameterId = static_cast<std::uint32_t>(control.tag);
    float normalized = 0.0f;
    if ([control isKindOfClass:[NSButton class]])
        normalized = ((NSButton*)control).state == NSControlStateValueOn ? 1.0f : 0.0f;
    else
        normalized = static_cast<float>(control.doubleValue);

    editor->callback(editor->callbackContext, parameterId, normalized);
}

- (void)timerFired:(NSTimer*)timerObject
{
    (void)timerObject;
    tryst::plugin::cocoa::refresh(editor);
}
@end

namespace tryst::plugin::cocoa
{
namespace
{
NSTextField* makeLabel(NSRect frame, NSString* text, CGFloat size, NSFontWeight weight)
{
    NSTextField* label = [[NSTextField alloc] initWithFrame:frame];
    label.stringValue = text;
    label.editable = NO;
    label.selectable = NO;
    label.bordered = NO;
    label.drawsBackground = NO;
    label.textColor = [NSColor colorWithSRGBRed:0.93 green:0.91 blue:0.98 alpha:1.0];
    label.font = [NSFont systemFontOfSize:size weight:weight];
    return label;
}

void addSectionTitle(Editor& editor, NSString* title, CGFloat x, CGFloat y, CGFloat width)
{
    NSTextField* label = makeLabel(NSMakeRect(x, y, width, 22.0), title, 13.0, NSFontWeightSemibold);
    label.textColor = [NSColor colorWithSRGBRed:0.70 green:0.57 blue:1.0 alpha:1.0];
    [editor.root addSubview:label];
}

void addSlider(Editor& editor,
               NSString* title,
               std::string_view persistentId,
               CGFloat x,
               CGFloat y,
               CGFloat width)
{
    const auto* parameter = editor.engine->parameterCatalog().find(persistentId);
    if (parameter == nullptr)
        return;

    NSTextField* label = makeLabel(NSMakeRect(x, y + 22.0, width, 18.0), title, 11.0, NSFontWeightMedium);
    [editor.root addSubview:label];

    NSSlider* slider = [[NSSlider alloc] initWithFrame:NSMakeRect(x, y, width, 22.0)];
    slider.minValue = 0.0;
    slider.maxValue = 1.0;
    slider.doubleValue = editor.engine->hostParameterValue(parameter->id);
    slider.continuous = YES;
    slider.target = editor.controller;
    slider.action = @selector(controlChanged:);
    slider.tag = static_cast<NSInteger>(parameter->id);
    [editor.root addSubview:slider];
    editor.bindings.push_back({ parameter->id, slider, false });
}

void addToggle(Editor& editor,
               NSString* title,
               std::string_view persistentId,
               CGFloat x,
               CGFloat y,
               CGFloat width)
{
    const auto* parameter = editor.engine->parameterCatalog().find(persistentId);
    if (parameter == nullptr)
        return;

    NSButton* button = [NSButton checkboxWithTitle:title target:editor.controller action:@selector(controlChanged:)];
    button.frame = NSMakeRect(x, y, width, 22.0);
    button.tag = static_cast<NSInteger>(parameter->id);
    button.state = editor.engine->hostParameterValue(parameter->id) >= 0.5f
        ? NSControlStateValueOn
        : NSControlStateValueOff;
    [editor.root addSubview:button];
    editor.bindings.push_back({ parameter->id, button, true });
}

void buildEditor(Editor& editor)
{
    editor.root = [[TrystBackgroundView alloc] initWithFrame:NSMakeRect(0.0, 0.0, editorWidth, editorHeight)];
    editor.root.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];

    NSTextField* title = makeLabel(NSMakeRect(22.0, 535.0, 300.0, 34.0), @"TRYST", 26.0, NSFontWeightBold);
    title.textColor = [NSColor colorWithSRGBRed:0.77 green:0.66 blue:1.0 alpha:1.0];
    [editor.root addSubview:title];

    NSTextField* subtitle = makeLabel(NSMakeRect(145.0, 541.0, 600.0, 20.0),
                                      @"multi-engine synthesizer  •  prototype editor",
                                      12.0,
                                      NSFontWeightRegular);
    subtitle.textColor = [NSColor colorWithSRGBRed:0.68 green:0.66 blue:0.74 alpha:1.0];
    [editor.root addSubview:subtitle];

    addSectionTitle(editor, @"PERFORMANCE", 22.0, 506.0, 200.0);

    constexpr CGFloat macroWidth = 190.0;
    constexpr CGFloat macroGap = 22.0;
    constexpr CGFloat macroStart = 22.0;
    for (std::size_t index = 0; index < 8; ++index)
    {
        const auto column = static_cast<CGFloat>(index % 4);
        const auto row = static_cast<CGFloat>(index / 4);
        const auto x = macroStart + column * (macroWidth + macroGap);
        const auto y = 449.0 - row * 64.0;
        const auto persistent = std::string("performance.macro.") + std::to_string(index + 1);
        NSString* label = [NSString stringWithFormat:@"Macro %zu", index + 1];
        addSlider(editor, label, persistent, x, y, macroWidth);
    }

    addSlider(editor, @"Morph X", "performance.morph.x", 234.0, 333.0, 190.0);
    addSlider(editor, @"Morph Y", "performance.morph.y", 446.0, 333.0, 190.0);

    addSectionTitle(editor, @"GENERATOR 1  /  ANALOG", 22.0, 290.0, 280.0);
    addSlider(editor, @"Level", "generator.1.generator.level", 22.0, 238.0, 360.0);
    addSlider(editor, @"Tune", "generator.1.generator.tune", 22.0, 176.0, 360.0);
    addSlider(editor, @"Shape  (Sine → Saw → Square)", "generator.1.engine.analog.shape", 22.0, 114.0, 360.0);

    NSTextField* generatorHelp = makeLabel(NSMakeRect(22.0, 62.0, 370.0, 38.0),
                                           @"Generator 1 is the default active analog voice.\nPlay MIDI into the track while moving these controls.",
                                           10.0,
                                           NSFontWeightRegular);
    generatorHelp.textColor = [NSColor colorWithSRGBRed:0.60 green:0.58 blue:0.66 alpha:1.0];
    generatorHelp.maximumNumberOfLines = 2;
    [editor.root addSubview:generatorHelp];

    addSectionTitle(editor, @"DUAL FILTER", 454.0, 290.0, 220.0);
    addToggle(editor, @"Filter 1 enabled", "filter.1.enabled", 454.0, 250.0, 180.0);
    addSlider(editor, @"Filter 1 cutoff", "filter.1.cutoff", 454.0, 204.0, 400.0);
    addSlider(editor, @"Filter 1 resonance", "filter.1.resonance", 454.0, 150.0, 400.0);
    addToggle(editor, @"Filter 2 enabled", "filter.2.enabled", 454.0, 112.0, 180.0);
    addSlider(editor, @"Filter 2 cutoff", "filter.2.cutoff", 454.0, 66.0, 190.0);
    addSlider(editor, @"Filter 2 resonance", "filter.2.resonance", 664.0, 66.0, 190.0);

    NSTextField* footer = makeLabel(NSMakeRect(454.0, 20.0, 420.0, 28.0),
                                    @"First editor pass: core performance, oscillator and filter controls.",
                                    10.0,
                                    NSFontWeightRegular);
    footer.textColor = [NSColor colorWithSRGBRed:0.60 green:0.58 blue:0.66 alpha:1.0];
    [editor.root addSubview:footer];
}
}

Editor* createEditor(PluginEngine& engine,
                     void* context,
                     ParameterChangedCallback callback) noexcept
{
    Editor* editor = new (std::nothrow) Editor;
    if (editor == nullptr)
        return nullptr;

    try
    {
        editor->engine = &engine;
        editor->callbackContext = context;
        editor->callback = callback;
        editor->bindings.reserve(24);
        editor->controller = [[TrystEditorController alloc] init];
        editor->controller->editor = editor;
        buildEditor(*editor);
        editor->timer = [NSTimer scheduledTimerWithTimeInterval:0.10
                                                        target:editor->controller
                                                      selector:@selector(timerFired:)
                                                      userInfo:nil
                                                       repeats:YES];
        refresh(editor);
        return editor;
    }
    catch (...)
    {
        destroyEditor(editor);
        return nullptr;
    }
}

void destroyEditor(Editor* editor) noexcept
{
    if (editor == nullptr)
        return;

    [editor->timer invalidate];
    editor->timer = nil;
    [editor->root removeFromSuperview];
    editor->root = nil;
    if (editor->controller != nil)
        editor->controller->editor = nullptr;
    editor->controller = nil;
    delete editor;
}

bool setParent(Editor* editor, void* nsView) noexcept
{
    if (editor == nullptr || editor->root == nil || nsView == nullptr)
        return false;

    NSView* parent = (__bridge NSView*)nsView;
    [editor->root removeFromSuperview];
    editor->root.frame = NSMakeRect(0.0, 0.0, editorWidth, editorHeight);
    [parent addSubview:editor->root];
    return true;
}

bool show(Editor* editor) noexcept
{
    if (editor == nullptr || editor->root == nil)
        return false;
    editor->root.hidden = NO;
    refresh(editor);
    return true;
}

bool hide(Editor* editor) noexcept
{
    if (editor == nullptr || editor->root == nil)
        return false;
    editor->root.hidden = YES;
    return true;
}

void refresh(Editor* editor) noexcept
{
    if (editor == nullptr || editor->engine == nullptr)
        return;

    for (const auto& binding : editor->bindings)
    {
        if (binding.control == nil)
            continue;
        const auto value = editor->engine->hostParameterValue(binding.parameterId);
        if (binding.toggle)
            ((NSButton*)binding.control).state = value >= 0.5f ? NSControlStateValueOn : NSControlStateValueOff;
        else
            binding.control.doubleValue = value;
    }
}
}
