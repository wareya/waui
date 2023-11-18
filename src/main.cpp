// https://jsfiddle.net/h1kmqz9f/2/
// https://jsfiddle.net/vwoeja89/13/

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <optional>

#include <SDL.h>

#include "images.h"

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
};
constexpr const Color Color::BLACK{0, 0, 0, 255};
constexpr const Color Color::GRAY{127, 127, 127, 255};
constexpr const Color Color::WHITE{255, 255, 255, 255};

struct Vec2
{
    float x = 0.0;
    float y = 0.0;
    
    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    
    Vec2 operator*(const Vec2& other) const { return {x * other.x, y * other.y}; }
    Vec2 operator/(const Vec2& other) const { return {x / other.x, y / other.y}; }

    Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Vec2 operator/(float divisor) const { return {x / divisor, y / divisor}; }
    
    Vec2 lerp(const Vec2& other, const Vec2& t) const
    {
        return Vec2 {
            lerp(x, other.x, t.x),
            lerp(y, other.y, t.y)
        };
    }
    Vec2 lerp(const Vec2& other, float t) const { return lerp(other, {t, t}); }
    
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
};

struct WaRenderAPI
{
    void (*draw_begin_frame)(void * userdata);
    void (*draw_finish_frame)(void * userdata);
    void (*draw_rect)(void * userdata, float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    
    void (*draw_texture_rect)(void * userdata,
        float x, float y, float w, float h,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
        uint64_t tex, float tex_x, float tex_y, float tex_w, float tex_h);
    
    // The given image data does not become owned by your application; you must copy it.
    uint64_t (*draw_texture_create)(void * userdata, uint32_t w, uint32_t h, const unsigned char * data);
    
    void (*draw_texture_destroy)(void * userdata, uint32_t texture_id);
};

struct WaEvent
{
    enum Type
    {
        ACTION,
        RESIZE,
        //MOUSE_MOTION,
        MOUSE_POSITION,
        MOUSE_BUTTON_PRESSED,
        MOUSE_BUTTON_RELEASED,
        NONE,
    };
    enum Action
    {
        UP,
        DOWN,
        LEFT,
        RIGHT,
        NEXT,
        PREV,
        NEXT_ALT,
        PREV_ALT,
        CONFIRM,
        CANCEL,
        ESCAPE,
        INVALID,
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

struct WaUI
{
    void * userdata;
    uint64_t root_control_id = 0;
    
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
    // It is not instant UB to do so, but it may leave WaUI in an incoherent state and trigger UB later.
    std::shared_ptr<WaControl> get_control(uint64_t id);
    std::vector<uint64_t> get_children(uint64_t id);
    
private:
    uint64_t test_texture = 0;
    uint64_t font_texture = 0;
    
    uint64_t next_id = 1;
    std::unordered_map<uint64_t, std::shared_ptr<WaControl>> controls;
    
    void render_string(WaRenderAPI * api, float x, float y, const char * string);
    void render_ascii_char(WaRenderAPI * api, float x, float y, uint8_t c);
    void render_control(WaRenderAPI * api, uint64_t id);
};

struct WaControl
{
    Rect2 rect;
    
    float anchor_left = 0;
    float anchor_right = 1;
    float anchor_top = 0;
    float anchor_bottom = 1;
    
    Color bg_color = Color::GRAY;
    Color modulate = Color::WHITE;
    
    uint64_t parent_id = 0;
    
    std::vector<uint64_t> children;
    
    bool stop_mouse = false;
    bool ignore_mouse = false;
    
    ptrdiff_t find_child(uint64_t id);
};

ptrdiff_t WaControl::find_child(uint64_t id)
{
    for (size_t index = 0; index < children.size(); index++)
    {
        if (children[index] == id)
            return index;
    }
    return -1;
}

WaUI::WaUI()
{
    root_control_id = control_create();
}
void WaUI::feed_event(WaEvent event)
{
    printf("%d\n", event.type);
}
uint64_t WaUI::control_create()
{
    auto id = next_id;
    next_id += 1;
    
    controls.insert({id, std::make_shared<WaControl>()});
    
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
}
void WaUI::render_scene(WaRenderAPI * api)
{
    api->draw_begin_frame(userdata);
    render_control(api, root_control_id);
    
    //render_string(api, 100, 100, "Drawing random text to the screen!!!");
    
    api->draw_finish_frame(userdata);
}

void WaUI::init(WaRenderAPI * api)
{
    test_texture = api->draw_texture_create(userdata, test_image_width, test_image_height, test_image);
    
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
    font_texture = api->draw_texture_create(userdata, font_image_width, font_image_height, font_data);
    
    free(font_data);
};
void WaUI::clean_up(WaRenderAPI * api)
{
    api->draw_texture_destroy(userdata, test_texture);
    test_texture = 0;
    api->draw_texture_destroy(userdata, font_texture);
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
        font_texture, c_x, c_y, 7, 13
    );
}

void WaUI::render_control(WaRenderAPI * api, uint64_t id)
{
    if (controls.count(id) == 0)
        return;
    
    //printf("rendering %d\n", id);
    
    auto & control = controls[id];
    auto pos = control->rect.pos;
    auto size = control->rect.size;
    auto c = control->bg_color;
    api->draw_rect(userdata, pos.x, pos.y, size.x, size.y, c.r, c.g, c.b, c.a);
    
    for (auto child_id : get_children(id))
    {
        render_control(api, child_id);
    }
};

struct CallbackContext
{
    SDL_Renderer * renderer;
    std::unordered_map<uint64_t, SDL_Texture *> textures;
    uint64_t next_id = 1;
};


void feed_event_to_ui(SDL_Event event, WaUI & ui)
{
    if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            ui.feed_event(WaEvent{WaEvent::Type::RESIZE, 0, (float)event.window.data1, (float)event.window.data2, 0});
    }
    else if (event.type == SDL_MOUSEMOTION)
    {
        //ui.feed_event(WaEvent{WaEvent::Type::MOUSE_MOTION, 0, (float)event.window.xrel, (float)event.window.yrel, 0});
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_POSITION, 0, (float)event.motion.x, (float)event.motion.y, 0});
    }
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
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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
        uint64_t tex, float tex_x, float tex_y, float tex_w, float tex_h)
    {
        auto context = (CallbackContext *) userdata;
        auto renderer = context->renderer;
        auto & textures = context->textures;
        
        SDL_Rect dest;
        dest.x = x;
        dest.y = y;
        dest.w = w;
        dest.h = h;
        
        SDL_Rect src;
        src.x = tex_x;
        src.y = tex_y;
        src.w = tex_w;
        src.h = tex_h;
        
        auto texture = textures[tex];
        
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderCopy(renderer, texture, &src, &dest);
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
    }
    exit:
    
    ui.clean_up(&api);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}