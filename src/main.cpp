// https://jsfiddle.net/h1kmqz9f/2/
// https://jsfiddle.net/vwoeja89/13/

#include <cstdint>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <optional>

#include <SDL.h>

#include "images.h"

#define USE_GEOMETRY_FOR_NINEPATCH 1

float lerp(float a, float b, float t)
{
    return a * (1.0 - t) + b * t;
}

struct Color
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}
    
    static const Color BLACK;
    static const Color GRAY;
    static const Color WHITE;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
};
constexpr const Color Color::BLACK  {  0,   0,   0, 255};
constexpr const Color Color::GRAY   {127, 127, 127, 255};
constexpr const Color Color::WHITE  {255, 255, 255, 255};
constexpr const Color Color::RED    {255,   0,   0, 255};
constexpr const Color Color::GREEN  {  0, 255,   0, 255};
constexpr const Color Color::BLUE   {  0,   0, 255, 255};
constexpr const Color Color::YELLOW {255, 255,   0, 255};

struct Vec2
{
    float x = 0.0;
    float y = 0.0;
    
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    
    Vec2 operator*(const Vec2& other) const { return {x * other.x, y * other.y}; }
    Vec2 operator/(const Vec2& other) const { return {x / other.x, y / other.y}; }

    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }
    
    Vec2 & operator+=(const Vec2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Vec2 & operator-=(const Vec2& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    Vec2 & operator*=(const Vec2& other)
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }
    Vec2 & operator/=(const Vec2& other)
    {
        x /= other.x;
        y /= other.y;
        return *this;
    }

    Vec2 & operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    Vec2 & operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }
    
    Vec2 lerp(const Vec2& other, const Vec2& t) const
    {
        return Vec2 {
            lerp(x, other.x, t.x),
            lerp(y, other.y, t.y)
        };
    }
    Vec2 lerp(const Vec2& other, float t) const { return lerp(other, {t, t}); }
    Vec2 round() const { return {roundf(x), roundf(y)}; };
    
private:
    static float lerp(float a, float b, float t)
    {
        return a * (1.0 - t) + b * t;
    }
};

struct Rect2
{
    Vec2 pos;
    Vec2 size;
    Vec2 get_end() const
    {
        return pos + size;
    }
    void set_end(Vec2 end)
    {
        size = end - pos;
    }
    Rect2 round() const
    {
        auto end = get_end().round();
        auto ret = *this;
        ret.pos = pos.round();
        ret.set_end(end);
        return ret;
    }
    bool contains_point(Vec2 point)
    {
        auto end = get_end();
        return point.x >= pos.x && point.x <= end.x && point.y >= pos.y && point.y <= end.y;
    }
};

struct WaRenderAPI
{
    void (*draw_begin_frame)(void * userdata);
    void (*draw_finish_frame)(void * userdata);
    void (*draw_rect)(void * userdata, float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    
    void (*draw_texture_rect)(void * userdata,
        float x, float y, float w, float h,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
        uint64_t tex, float tex_x, float tex_y, float tex_w, float tex_h,
        uint32_t tex_size_w, uint32_t tex_size_h);
    
    // The given image data does not become owned by your application; you must copy it.
    uint64_t (*texture_create)(void * userdata, uint32_t w, uint32_t h, const unsigned char * data);
    
    void (*texture_destroy)(void * userdata, uint32_t texture_id);
};

struct WaEvent
{
    enum Type
    {
        NONE,
        ACTION,
        RESIZE,
        MOUSE_POSITION,
        MOUSE_BUTTON_PRESSED,
        MOUSE_BUTTON_RELEASED,
    };
    enum Action
    {
        INVALID,
        UP,
        DOWN,
        LEFT,
        RIGHT,
        NEXT,
        PREV,
        NEXT_ALT,
        PREV_ALT,
        PGUP,
        PGDN,
        CONFIRM,
        CANCEL,
        ESCAPE,
    };
    enum Button
    {
        M1,
        M2,
        M3,
        M4,
        M5,
        MWHEEL_UP,
        MWHEEL_DOWN,
    };
    
    int type = 0; // e.g. RESIZE, ACTION, MOUSE_POSITION
    int subtype = 0; // e.g. UP, DOWN, M1, etc
    
    float x = 0.0;
    float y = 0.0;
    uint64_t data = 0; // e.g. pressed / released
};

struct WaUI;
struct WaControl;
struct WaControlAPI;

struct WaControlAPI
{
    void (*think)(WaControl * control, WaUI * ui, uint64_t delta);
    void (*handle_event)(WaControl * control, WaUI * ui, uint64_t delta);
    void (*reflow)(WaControl * control, WaUI * ui, uint64_t delta);
    void (*render)(WaControl * control, WaUI * ui, WaRenderAPI * api);
};

struct WaControl
{
    Rect2 rect;
    Rect2 anchor = {{0.1, 0.1}, {0.9, 0.9}};
    
    Vec2 min_size;
    
    Color bg_color = Color::WHITE;
    Color modulate = Color::WHITE;
    
    uint64_t id = 0;
    uint64_t parent_id = 0;
    
    std::vector<uint64_t> children;
    
    bool visible = true;
    bool draw_bg = true;
    bool mouse_to_self = true;
    bool mouse_to_children = true;
    
    void think(WaUI * ui, uint64_t delta) {}
    
    // returns whether the event has been "consumed"
    bool feed_event(WaUI * ui, WaEvent event, Vec2 pos_offset);
    // same
    bool handle_event(WaUI * ui, WaEvent event, Vec2 pos_offset);
    
    ptrdiff_t find_child(uint64_t id);
    void reflow(WaUI * ui);
    void render(WaUI * ui, WaRenderAPI * api);
    
    void fit_rect(Rect2 rect);
};

struct WaUI
{
    friend WaControl;
    
    void * userdata;
    uint64_t root_control_id = 0;
    uint64_t id = 0;
    
    WaUI();
    
    void init(WaRenderAPI * api);
    void clean_up(WaRenderAPI * api);
    
    void render_scene(WaRenderAPI * api);
    void think(uint64_t delta);
    void feed_event(WaEvent event);
    
    uint64_t control_create();
    void control_destroy(uint64_t id);
    void control_add_child(uint64_t parent_id, uint64_t child_id);
    void control_remove_child(uint64_t parent_id, uint64_t child_id);
    uint64_t control_get_parent(uint64_t id);
    // Should only be used temporarily to modify the properties of the control.
    // The tree should not be modified while the control is borrowed.
    // It is not instant UB to do so, but it may leave WaUI in an invalid state and trigger UB later, or cause memory leaks.
    std::shared_ptr<WaControl> get_control(uint64_t id);
    std::vector<uint64_t> get_children(uint64_t id);
    
    bool px_snapped_rendering = true;
    
protected:
    uint64_t clicked_control = 0;
    uint64_t clicked_control_count = 0;
    
    uint64_t test_texture = 0;
    uint64_t font_texture = 0;
    
private:
    bool layout_dirty = true;
    
    Vec2 size = {800, 600};
    
    std::vector<int> font_advance;
    std::vector<int> font_offset;
    
    Vec2 draw_offset;
    
    uint64_t next_id = 1;
    std::unordered_map<uint64_t, std::shared_ptr<WaControl>> controls;
    
    void render_string(WaRenderAPI * api, float x, float y, const char * string);
    void render_ascii_char(WaRenderAPI * api, float x, float y, uint8_t c);
    void render_control(WaRenderAPI * api, uint64_t id);
    
    void render_ninepatch(WaRenderAPI * api, uint64_t texture, Rect2 dest, Rect2 src_outer, Rect2 src_inner, Vec2 texture_size, Color color);
};

bool WaControl::feed_event(WaUI * ui, WaEvent event, Vec2 pos_offset)
{
    if (!visible)
        return false;
    
    for (size_t i = children.size() - 1; i < children.size(); i -= 1)
    {
        auto control = ui->get_control(children[i]);
        if (control->feed_event(ui, event, pos_offset + rect.pos))
            return true;
    }
    return handle_event(ui, event, pos_offset);
}
bool WaControl::handle_event(WaUI * ui, WaEvent event, Vec2 pos_offset)
{
    auto global_rect = rect;
    global_rect.pos += pos_offset;
    if (event.type == WaEvent::MOUSE_BUTTON_PRESSED)
    {
        if (!global_rect.contains_point({event.x, event.y}))
            return false;
        printf("Press detected on control %lld\n", id);
        ui->clicked_control_count += 1;
        ui->clicked_control = id;
        return true;
    }
    if (event.type == WaEvent::MOUSE_BUTTON_RELEASED)
    {
        if (!global_rect.contains_point({event.x, event.y}))
        {
            if (ui->clicked_control == id)
            {
                printf("release detected off of control %lld\n", id);
                return true;
            }
            else
                return false;
        }
        printf("release detected on control %lld\n", id);
        return true;
    }
    
    return false;
}

ptrdiff_t WaControl::find_child(uint64_t id)
{
    for (size_t index = 0; index < children.size(); index++)
    {
        if (children[index] == id)
            return index;
    }
    return -1;
}

void WaControl::reflow(WaUI * ui)
{
    for (auto child_id : children)
    {
        auto control = ui->get_control(child_id);
        control->fit_rect({{0, 0}, rect.size});
        control->reflow(ui);
    }
}

void WaControl::render(WaUI * ui, WaRenderAPI * api)
{
    Vec2 pos = {0, 0};
    Vec2 size = rect.size;
    
    if (draw_bg)
        ui->render_ninepatch(api, ui->test_texture, {pos, size}, {{0, 0}, {8, 8}}, {{4, 4}, {0, 0}}, {test_image_width, test_image_height}, bg_color);
}

void WaControl::fit_rect(Rect2 parent_rect)
{
    rect.pos = parent_rect.pos.lerp(parent_rect.size, anchor.pos);
    rect.set_end(parent_rect.pos.lerp(parent_rect.size, anchor.size));
}

WaUI::WaUI()
{
    root_control_id = control_create();
}
void WaUI::feed_event(WaEvent event)
{
    if (event.type == WaEvent::Type::RESIZE)
    {
        size.x = event.x;
        size.y = event.y;
        layout_dirty = true;
    }
    // TODO: separate "gui" events from "global" events
    if ((event.type == WaEvent::Type::MOUSE_BUTTON_RELEASED
            || event.type == WaEvent::Type::MOUSE_BUTTON_PRESSED
            || event.type == WaEvent::Type::MOUSE_POSITION)
        and clicked_control != 0 and controls.count(clicked_control) != 0)
    {
        auto control = controls[clicked_control];
        // calculate global position of parent
        Vec2 offset = {0, 0};
        auto parent_id = control->parent_id;
        while (parent_id != 0 and controls.count(parent_id) != 0)
        {
            offset += controls[parent_id]->rect.pos;
            parent_id = controls[parent_id]->parent_id;
        }
        // handle clicked-on-specific-element event weirdness
        if (!control->handle_event(this, event, offset) and event.type == WaEvent::Type::MOUSE_BUTTON_PRESSED)
            clicked_control_count += 1;
        if (event.type == WaEvent::Type::MOUSE_BUTTON_RELEASED)
        {
            clicked_control_count -= 1;
            if (clicked_control_count == 0)
            {
                printf("reset clicked control... was %lld\n", clicked_control);
                clicked_control = 0;
            }
        }
    }
    else
    {
        controls[root_control_id]->feed_event(this, event, {0, 0});
    }
}
uint64_t WaUI::control_create()
{
    auto id = next_id;
    next_id += 1;
    auto control = std::make_shared<WaControl>();
    control->id = id;
    
    controls.insert({id, control});
    
    return id;
}
void WaUI::control_destroy(uint64_t id)
{
    if (controls.count(id) == 0)
        return;
    
    auto & child = controls[id];
    if (child->parent_id != 0)
        control_remove_child(child->parent_id, id);
    
    controls.erase(id);
}
void WaUI::control_add_child(uint64_t parent_id, uint64_t child_id)
{
    if (controls.count(parent_id) == 0 || controls.count(child_id) == 0)
        return;
    
    auto & child = controls[child_id];
    if (child->parent_id != 0)
        control_remove_child(child->parent_id, child_id);
    
    auto & parent = controls[parent_id];
    parent->children.push_back(child_id);
    if (child->parent_id == parent_id)
        child->parent_id = 0;
}
void WaUI::control_remove_child(uint64_t parent_id, uint64_t child_id)
{
    if (controls.count(parent_id) == 0 || controls.count(child_id) == 0)
        return;
    
    auto & parent = controls[parent_id];
    auto & child = controls[child_id];
    auto index = parent->find_child(child_id);
    if (index >= 0)
        parent->children.erase(parent->children.begin() + index);
    child->parent_id = parent_id;
}
uint64_t WaUI::control_get_parent(uint64_t id)
{
    if (controls.count(id) == 0)
        return 0;
    
    return controls[id]->parent_id;
}
// Should only be used temporarily to modify the properties of the control.
// The tree should not be modified while the control is borrowed.
// It is not instant UB to do so, but it may leave WaUI in an incoherent state and trigger UB later.
std::shared_ptr<WaControl> WaUI::get_control(uint64_t id)
{
    if (controls.count(id) == 0)
        return nullptr;
    return controls[id];
}

std::vector<uint64_t> WaUI::get_children(uint64_t id)
{
    if (id == 0)
    {
        std::vector<uint64_t> ret;
        for (auto & [id, _] : controls)
            ret.push_back(id);
        return ret;
    }
    else if (controls.count(id) == 1)
    {
        return controls[id]->children;
    }
    return {};
}

void WaUI::think(uint64_t delta)
{
    delta = delta;
    
    if (controls.count(root_control_id) == 0)
        return;
    
    if (layout_dirty)
    {
        auto & control = controls[root_control_id];
        //auto & control_rect = control->rect;
        
        //printf("resizing %d (via %f %f)\n", root_control_id, control->anchor.size.x, control->anchor.size.y);
        control->fit_rect({{0, 0}, {size}});
        control->reflow(this);
        
        layout_dirty = false;
    }
}
void WaUI::render_scene(WaRenderAPI * api)
{
    api->draw_begin_frame(userdata);
    
    draw_offset = {0, 0};
    render_control(api, root_control_id);
    
    //render_string(api, 100, 100, "Drawing random text to the screen!!!");
    
    api->draw_finish_frame(userdata);
}

void WaUI::init(WaRenderAPI * api)
{
    test_texture = api->texture_create(userdata, test_image_width, test_image_height, test_image);
    
    auto font_data = (unsigned char *)malloc(font_image_width * font_image_height * 4);
    auto i = 0;
    auto j = 0;
    auto bit = 0;
    for (unsigned long int y = 0; y < font_image_height; y++)
    {
        for (unsigned long int x = 0; x < font_image_width; x++)
        {
            font_data[i++] = 255;
            font_data[i++] = 255;
            font_data[i++] = 255;
            font_data[i++] = ((font_image[j] >> (7 - bit)) & 1) ? 255 : 0;
            bit += 1;
            if (bit >= 8)
            {
                bit -= 8;
                j += 1;
            }
        }
    }
    font_texture = api->texture_create(userdata, font_image_width, font_image_height, font_data);
    
    free(font_data);
};
void WaUI::clean_up(WaRenderAPI * api)
{
    api->texture_destroy(userdata, test_texture);
    test_texture = 0;
    api->texture_destroy(userdata, font_texture);
    font_texture = 0;
};
void WaUI::render_string(WaRenderAPI * api, float x, float y, const char * string)
{
    while (*string != 0)
    {
        render_ascii_char(api, x, y, *string);
        string += 1;
        x += 7;
    }
}
void WaUI::render_ascii_char(WaRenderAPI * api, float x, float y, uint8_t c)
{
    float c_x = (c % 32) * 7;
    float c_y = (c / 32) * 13;
    api->draw_texture_rect(userdata,
        x, y, 7, 13,
        255, 255, 255, 255,
        font_texture, c_x, c_y, 7, 13,
        font_image_width, font_image_height
    );
}

void WaUI::render_control(WaRenderAPI * api, uint64_t id)
{
    if (controls.count(id) == 0)
        return;
    
    auto & control = controls[id];
    
    if (!control->visible)
        return;
    
    auto pos = control->rect.pos + draw_offset;
    auto prev_draw_offset = draw_offset;
    
    draw_offset = pos;
    
    control->render(this, api);
    //printf("rendering %d (%f %f %f %f)\n", id, pos.x, pos.y, size.x, size.y);
    
    for (auto child_id : get_children(id))
    {
        render_control(api, child_id);
    }
    
    draw_offset = prev_draw_offset;
};

void WaUI::render_ninepatch(WaRenderAPI * api, uint64_t texture, Rect2 dest, Rect2 src_outer, Rect2 src_inner, Vec2 texture_size, Color color)
{
    dest.pos += draw_offset;
    
    if (px_snapped_rendering)
        dest = dest.round();
    
    auto corner_a = src_inner.pos - src_outer.pos;
    auto corner_b = dest.size - (src_outer.get_end() - src_inner.get_end());
    
    auto rect_a = Rect2{dest.pos, corner_a};
    auto rect_b = Rect2{dest.pos + corner_a, corner_b - corner_a};
    auto rect_c = Rect2{dest.pos + corner_b, dest.size - corner_b};
    
    auto src_a = Rect2{src_outer.pos, src_inner.pos - src_outer.pos};
    auto src_b = Rect2{src_inner.pos, src_inner.size};
    auto src_c = Rect2{src_inner.get_end(), src_outer.get_end() - src_inner.get_end()};
    
    Rect2 dests[3] = {rect_a, rect_b, rect_c};
    Rect2 srces[3] = {src_a, src_b, src_c};
    
    for (int ix = 0; ix < 3; ix++)
    {
        for (int iy = 0; iy < 3; iy++)
        {
            api->draw_texture_rect(userdata,
                dests[ix].pos.x, dests[iy].pos.y, dests[ix].size.x, dests[iy].size.y, color.r, color.g, color.b, color.a,
                texture, srces[ix].pos.x, srces[iy].pos.y, srces[ix].size.x, srces[iy].size.y,
                texture_size.x, texture_size.y
            );
        }
    }
}

void feed_event_to_ui(SDL_Event event, WaUI & ui)
{
    if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            ui.feed_event(WaEvent{WaEvent::Type::RESIZE, 0, (float)event.window.data1, (float)event.window.data2, 0});
    }
    else if (event.type == SDL_MOUSEMOTION)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_POSITION, 0, (float)event.motion.x, (float)event.motion.y, 0});
    else if (event.type == SDL_MOUSEBUTTONDOWN)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_BUTTON_PRESSED, event.button.button, (float)event.button.x, (float)event.button.y, 0});
    else if (event.type == SDL_MOUSEBUTTONUP)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_BUTTON_RELEASED, event.button.button, (float)event.button.x, (float)event.button.y, 0});
    else if (event.type == SDL_KEYDOWN)
    {
        auto wa_event = WaEvent{WaEvent::Type::ACTION, 0, 0, 0, 1};
        switch (event.key.keysym.sym)
        {
        case SDLK_UP:
            wa_event.subtype = WaEvent::Action::UP;
            break;
        case SDLK_DOWN:
            wa_event.subtype = WaEvent::Action::DOWN;
            break;
        case SDLK_LEFT:
            wa_event.subtype = WaEvent::Action::LEFT;
            break;
        case SDLK_RIGHT:
            wa_event.subtype = WaEvent::Action::RIGHT;
            break;
        default:
            wa_event.subtype = WaEvent::Action::INVALID;
        }
        if (wa_event.subtype != WaEvent::Action::INVALID)
            ui.feed_event(wa_event);
    }
}

struct CallbackContext
{
    SDL_Renderer * renderer;
    std::unordered_map<uint64_t, SDL_Texture *> textures;
    uint64_t next_id = 1;
};

int main()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return fprintf(stderr, "failed to initialize SDL"), -1;
    
    auto window = SDL_CreateWindow("WaUI Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window)
        return fprintf(stderr, "failed to open SDL window"), -1;
    
    SDL_SetWindowResizable(window, SDL_TRUE);
    
    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
        return fprintf(stderr, "failed to open SDL renderer"), -1;
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    
    auto ui = WaUI();
    
    auto c_id = ui.control_create();
    ui.control_add_child(ui.root_control_id, c_id);
    
    auto control = ui.get_control(c_id);
    if (control)
    {
        control->rect.pos = {4, 52};
        control->rect.size = {48, 32};
        control->bg_color = Color::RED;
        printf("%f %f\n", control->rect.pos.x, control->rect.pos.y);
    }
    else
    {
        printf("AAIAWE???\n");
    }
    
    auto context = CallbackContext();
    context.renderer = renderer;
    
    ui.userdata = &context;
    
    auto sdl_begin_frame = [](void * userdata)
    {
        auto renderer = ((CallbackContext *) userdata)->renderer;
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_RenderClear(renderer);
    };
    auto sdl_finish_frame = [](void * userdata)
    {
        auto renderer = ((CallbackContext *) userdata)->renderer;
        SDL_RenderPresent(renderer);
    };
    auto sdl_draw_rect = [](void * userdata, float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        auto renderer = ((CallbackContext *) userdata)->renderer;
        SDL_Rect rect;
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderFillRect(renderer, &rect);
    };
    auto sdl_draw_texture_rect = [](void * userdata,
        float x, float y, float w, float h,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
        uint64_t tex, float tex_x, float tex_y, float tex_w, float tex_h,
        uint32_t tex_size_w, uint32_t tex_size_h)
    {
        auto context = (CallbackContext *) userdata;
        auto renderer = context->renderer;
        auto & textures = context->textures;
        
        auto texture = textures[tex];
        
#if !USE_GEOMETRY_FOR_NINEPATCH
        SDL_Rect src = {(int)tex_x, (int)tex_y, (int)tex_w, (int)tex_h};
        SDL_Rect dest = {(int)x, (int)y, (int)w, (int)h};
        
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderCopy(renderer, texture, &src, &dest);
#else // !USE_GEOMETRY_FOR_NINEPATCH
        
        SDL_Vertex verts[4] = {
            {{x    , y    }, {r, g, b, a}, {(tex_x        ) / tex_size_w, (tex_y        ) / tex_size_h}},
            {{x + w, y    }, {r, g, b, a}, {(tex_x + tex_w) / tex_size_w, (tex_y        ) / tex_size_h}},
            {{x    , y + h}, {r, g, b, a}, {(tex_x        ) / tex_size_w, (tex_y + tex_h) / tex_size_h}},
            {{x + w, y + h}, {r, g, b, a}, {(tex_x + tex_w) / tex_size_w, (tex_y + tex_h) / tex_size_h}},
        };
        
        int indexes[6] = {0, 1, 2, 2, 1, 3};
        
        SDL_RenderGeometry(renderer, texture, verts, 4, indexes, 6);
#endif // else of #if !USE_GEOMETRY_FOR_NINEPATCH
    };
    
    auto sdl_texture_create = [](void * userdata, uint32_t w, uint32_t h, const unsigned char * data)
    {
        auto context = (CallbackContext *) userdata;
        auto renderer = context->renderer;
        auto & textures = context->textures;
        
        auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
        auto texture_id = context->next_id;
        context->next_id += 1;
        textures.insert({texture_id, texture});
        
        SDL_UpdateTexture(texture, NULL, data, w * 4);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        
        return texture_id;
    };
    auto sdl_texture_destroy = [](void * userdata, uint32_t texture_id)
    {
        auto & textures = ((CallbackContext *) userdata)->textures;
        auto texture = textures[texture_id];
        textures.erase(texture_id);
        SDL_DestroyTexture(texture);
    };
    
    auto api = WaRenderAPI
    {
        sdl_begin_frame,
        sdl_finish_frame,
        sdl_draw_rect,
        sdl_draw_texture_rect,
        sdl_texture_create,
        sdl_texture_destroy,
    };
    
    ui.init(&api);
    
    uint64_t time = SDL_GetTicks64();
    
    while (true)
    {
        uint64_t new_time = SDL_GetTicks64();
        
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                goto exit;
            feed_event_to_ui(event, ui);
        }
        
        ui.render_scene(&api);
        ui.think(new_time - time);
        time = new_time;
        
        SDL_Delay(1);
    }
    exit:
    
    ui.clean_up(&api);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}