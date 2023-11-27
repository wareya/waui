#include <algorithm>
#include <memory>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>
#include <typeindex>

#include "images.h"
#include "unifont.h"

/*
const int font_char_w = 7;
const int font_char_h = 13;
const int font_cols = 32;
const int font_rows = 8;
const int font_bits = 8;

#define font_w font_image_width
#define font_h font_image_height
#define font_i font_image
*/

const int font_char_w = 16;
const int font_char_h = 16;
const int font_cols = 256;
const int font_rows = 256;
const int font_bits = 32;

#define font_w unifont_width
#define font_h unifont_height
#define font_i unifont

#define USE_GEOMETRY_FOR_NINEPATCH 1

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
    static const Color OFFWHITE;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
};
constexpr const Color Color::BLACK       {  0,   0,   0, 255};
constexpr const Color Color::GRAY        {127, 127, 127, 255};
constexpr const Color Color::WHITE       {255, 255, 255, 255};
constexpr const Color Color::OFFWHITE    {224, 224, 224, 255};
constexpr const Color Color::RED         {255,   0,   0, 255};
constexpr const Color Color::GREEN       {  0, 255,   0, 255};
constexpr const Color Color::BLUE        {  0,   0, 255, 255};
constexpr const Color Color::YELLOW      {255, 255,   0, 255};

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
    
    bool operator==(const Vec2& other) const
    {
        return x == other.x and y == other.y;
    }
    
    Vec2 lerp(const Vec2& other, const Vec2& t) const
    {
        return Vec2 {
            lerp(x, other.x, t.x),
            lerp(y, other.y, t.y)
        };
    }
    
    Vec2 max(const Vec2& other) const
    {
        return Vec2 {
            std::max(x, other.x),
            std::max(y, other.y)
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
    Rect2 clip(const Rect2 & other) const
    {
        auto ret = *this;
        ret.pos.x = std::max(pos.x, other.pos.x);
        ret.pos.y = std::max(pos.y, other.pos.y);
        auto end_a = get_end();
        auto end_b = other.get_end();
        end_a.x = std::min(end_a.x, end_b.x);
        end_a.y = std::min(end_a.y, end_b.y);
        ret.set_end(end_a);
        return ret;
    }
    bool contains_point(Vec2 point)
    {
        auto end = get_end();
        return point.x >= pos.x && point.x <= end.x && point.y >= pos.y && point.y <= end.y;
    }
    bool operator==(const Rect2& other) const
    {
        return pos == other.pos and size == other.size;
    }
};

struct Vec2x2
{
    Vec2 a;
    Vec2 b;
    Vec2x2 round() const
    {
        auto ret = *this;
        ret.a = a.round();
        ret.b = b.round();
        return ret;
    }
};

struct WaRenderAPI
{
    WaRenderAPI()
    {
        draw_begin_frame = nullptr;
        draw_finish_frame = nullptr;
        draw_rect = nullptr;
        draw_texture_rect = nullptr;
        draw_clip_rect_set = nullptr;
        draw_clip_rect_clear = nullptr;
        ime_rect_inform = nullptr;
        texture_create = nullptr;
        texture_destroy = nullptr;
    }
    
    void (*draw_begin_frame)(void * userdata);
    void (*draw_finish_frame)(void * userdata);
    void (*draw_rect)(void * userdata, float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    
    void (*draw_texture_rect)(void * userdata,
        float x, float y, float w, float h,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
        uint64_t tex, float tex_x, float tex_y, float tex_w, float tex_h,
        uint32_t tex_size_w, uint32_t tex_size_h);
    
    void (*draw_clip_rect_set)(void * userdata, float x, float y, float w, float h);
    void (*draw_clip_rect_clear)(void * userdata);
    
    void (*ime_rect_inform)(void * userdata, float x, float y, float w, float h);
    
    // The given image data does not become owned by your application; you must copy it.
    // The texture data is stored one pixel at a time (not a planar format), 8 bit per channel RGBA, and exactly (w * h * 4) bytes long.
    // Pixels are stored in row-major order, starting in the top left.
    uint64_t (*texture_create)(void * userdata, uint32_t w, uint32_t h, bool filter, const unsigned char * data);
    
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
        TEXT,
        TEXTEDIT,
        FOCUS, // Synthesized by the WaUI object. Do not costruct yourself.
        DEFOCUS, // Synthesized by the WaUI object. Do not costruct yourself.
    };
    enum Action
    {
        INVALID,
        UP,
        DOWN,
        LEFT,
        RIGHT,
        NEXT, // e.g. tab
        PREV, // e.g. shift-tab
        NEXT_ALT, // e.g. ctrl-tab
        PREV_ALT, // e.g. ctrl-shift-tab
        HOME, // e.g. same keyboard key
        END, // e.g. same keyboard key
        HOME_ALT, // e.g. same keyboard key but also with ctrl
        END_ALT, // e.g. same keyboard key but also with ctrl
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
    
    // The application is responsible for allocating and freeing this memory. WaUI will copy it, not hold onto it.
    const void * ptr_data = nullptr;
};

union WaSignalData
{
    uint8_t bytes[16];
    float floats[4];
    Vec2 vec2s[2];
};

struct WaSignal
{
    uint64_t control_id; // not necessary, but allows for cleaner signal-handling code
    uint64_t control_type; // because inheritance
    uint64_t type;
    WaSignalData data;
    void * extra_data;
};

struct WaUI;
struct WaControl;
struct WaControlAPI;

struct WaButton;

struct WaControlAPI
{
    WaControlAPI()
    {
        type_id = 0;
        
        data = nullptr;
        
        init = nullptr;
        destruct = nullptr;
        
        think = nullptr;
        handle_event = nullptr;
        reflow = nullptr;
        render = nullptr;
        get_min_size = nullptr;
        
        signal_destruct = nullptr;
    }
    
    uint64_t type_id;
    
    template<typename T>
    T * set_data(T * new_data)
    {
        data = (void *) new_data;
        data_type = std::type_index(typeid(T));
        return new_data;
    }
    
    template<typename T>
    void delete_data()
    {
        if (data != nullptr)
        {
            if (std::type_index(typeid(T)) == data_type)
                delete (T *)data;
            else
                raise(SIGSEGV);
        }
        
        data_type = std::type_index(typeid(void));
        data = nullptr;
    }
    
    template<typename T>
    T * get_data()
    {
        if (std::type_index(typeid(T)) == data_type)
            return (T *)data;
        return nullptr;
    }
    
    void (*init)(WaControl * control);
    void (*destruct)(WaControl * control);
    
    void (*think)(WaControl * control, WaUI * ui, uint64_t delta);
    bool (*handle_event)(WaControl * control, WaUI * ui, WaEvent event, Vec2 pos_offset);
    // returns whether the control handles reflowing at all
    // (return false if you want default reflow to happen)
    bool (*reflow)(WaControl * control, WaUI * ui);
    void (*render)(WaControl * control, WaUI * ui, WaRenderAPI * api);
    Vec2 (*get_min_size)(WaControl * control, WaUI * ui);
    
    void (*signal_destruct)(WaControl * control, WaUI * ui, WaSignal * signal);

private:
    std::type_index data_type = std::type_index(typeid(void));
    void * data;
};

struct WaControl
{
    friend WaUI;
    
    Rect2 rect;
    Rect2 anchor = {{0.0, 0.0}, {0.0, 0.0}};
    
    Vec2x2 padding = {{4, 4}, {4, 4}};
    Vec2x2 margin = {{0, 0}, {0, 0}};
    
    Vec2 min_size;
    
    Color bg_color = Color::WHITE;
    Color modulate = Color::WHITE;
    
    uint64_t type_id = 0;
    
    uint64_t id = 0;
    uint64_t parent_id = 0;
    
    bool visible = true;
    bool draw_bg = true;
    bool mouse_to_self = true;
    bool mouse_to_children = true;
    bool clip_children = true;
    
    bool focused = false;
    
    WaControlAPI type_info;
    std::vector<uint64_t> children;
    
    std::vector<WaSignal> signals;
    
    // returns true if the event has been "consumed"
    bool feed_event(WaUI * ui, WaEvent event, Vec2 pos_offset);
    
    void fit_rect(WaUI * ui, Rect2 rect);
    ptrdiff_t find_child(uint64_t id);
    
    void clear_all_signals(WaUI * ui);
    
    // methods with callbacks:
    
    void think(WaUI * ui, uint64_t delta);
    // returns true if the event has been "consumed"
    bool handle_event(WaUI * ui, WaEvent event, Vec2 pos_offset);
    void reflow(WaUI * ui);
    void render(WaUI * ui, WaRenderAPI * api);
    Vec2 get_min_size(WaUI * ui);
    void signal_destruct(WaUI * ui, WaSignal * signal);
};

// assumes that the text is valid utf-8
uint32_t next_codepoint_utf8(const char * & str)
{
    if (*str == 0)
        return 0;
    
    auto ret = 0;
    uint8_t c = *str++;
    if ((c & 0x80) == 0)
        ret = c;
    else if((c & 0xE0) == 0xC0)
    {
        ret = ((uint32_t) c & 0x1F);
        ret = (ret << 6) | (*str++ & 0x3F);
    }
    else if((c & 0xF0) == 0xE0)
    {
        ret = ((uint32_t) c & 0x0F);
        ret = (ret << 6) | (*str++ & 0x3F);
        ret = (ret << 6) | (*str++ & 0x3F);
    }
    else if((c & 0xF8) == 0xF0)
    {
        ret = ((uint32_t) c & 0x07);
        ret = (ret << 6) | (*str++ & 0x3F);
        ret = (ret << 6) | (*str++ & 0x3F);
        ret = (ret << 6) | (*str++ & 0x3F);
    }
    
    return ret;
}
int next_pos_utf8(const char * str, int pos)
{
    if (*str == 0)
        return 0;
    pos++;
    while (str[pos] != 0 and (str[pos] & 0xC0) == 0x80)
        pos++;
    return pos;
}
int prev_pos_utf8(const char * str, int pos)
{
    if (*str == 0 or pos == 0)
        return 0;
    pos--;
    while (str[pos] != 0 and (str[pos] & 0xC0) == 0x80 and pos > 0)
        pos--;
    return pos;
}

// assumes that the text is valid utf-8
size_t substr_len_utf8(const std::string & str, size_t start, size_t count)
{
    size_t i = start;
    size_t n = 0;
    while (n <= count and i < str.size())
    {
        auto c = str[i];
        // single-byte character
        if ((c & 0x80) == 0)
            n += 1;
        // first byte of multibyte
        else if ((c & 0xC0) != 0x80)
            n += 1;
        // otherwise continuation byte; just continue
        
        if (n <= count)
            i += 1;
    }
    return i - start;
}

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
    
    uint64_t control_type_add(const char * name, WaControlAPI api);
    
    // Returns 0 if the type does not exist.
    uint64_t control_type_get(const char * name);
    
    // The callee does not own the returned pointer and is not allowed to hold onto it.
    // It must be copied into a new buffer before calling any other UI functions.
    // Returns null if the type does not exist.
    const char * control_type_get_name(uint64_t type_id);
    
    uint64_t control_create(uint64_t type_id);
    uint64_t control_create_untyped();
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
    
    float string_get_height(const char * string);
    float string_get_width(const char * string);
    float string_get_cursor_location(const char * string, int cursor);
    
    void ime_rect_inform(Rect2 dest);
    
    void render_string(WaRenderAPI * api, float x, float y, const char * string, Color color);
    void render_char(WaRenderAPI * api, float x, float y, uint32_t c, Color color);
    void render_rect(WaRenderAPI * api, Rect2 dest, Color color);
    void render_ninepatch(WaRenderAPI * api, uint64_t texture, Rect2 dest, Rect2 src_outer, Rect2 src_inner, Vec2 texture_size, Color color);
    
    void clear_all_signals();
    
    uint64_t test_texture = 0;
    uint64_t panel_texture = 0;
    uint64_t micro_bg_texture = 0;
    uint64_t font_texture = 0;
    
    uint64_t focus_control_get();
    void focus_control_set(uint64_t new_control);
    
protected:
    uint64_t clicked_control = 0;
    uint64_t clicked_control_count = 0;
    
private:
    uint64_t focused_control = 0;
    
    template<typename T>
    uint64_t control_type_add_basis(const char * name);
    
    bool layout_dirty = true;
    
    Vec2 size = {800, 600};
    
    std::vector<int> font_advance;
    std::vector<int> font_offset;
    
    Vec2 rect_offset;
    Rect2 last_clip_rect;
    
    Rect2 ime_rect;
    Rect2 ime_rect_prev;
    
    uint64_t next_type_id = 1;
    std::unordered_map<uint64_t, WaControlAPI> control_types;
    std::unordered_map<uint64_t, std::string> control_type_names;
    std::unordered_map<std::string, uint64_t> control_type_ids;
    
    uint64_t next_id = 1;
    std::unordered_map<uint64_t, std::shared_ptr<WaControl>> controls;
    
    void render_control(WaRenderAPI * api, uint64_t id);
};

struct WaButtonData
{
    std::string label;
};
struct WaButton
{
    static void init(WaControl * control)
    {
        auto data = control->type_info.set_data(new WaButtonData());
        data->label = "no YOU are a button";
    }
    static void destruct(WaControl * control)
    {
        control->type_info.delete_data<WaButtonData>();
    }
    static bool handle_event(WaControl * control, WaUI * ui, WaEvent event, Vec2 pos_offset)
    {
        if (event.type == WaEvent::MOUSE_BUTTON_PRESSED)
        {
            control->signals.push_back(WaSignal {
                control->id,
                control->type_info.type_id,
                0,
                {},
                nullptr
            });
            //printf("Press detected on BUTTON %lld\n", control->id);
            return true;
        }
        if (event.type == WaEvent::MOUSE_BUTTON_RELEASED)
        {
            control->signals.push_back(WaSignal {
                control->id,
                control->type_info.type_id,
                1,
                {},
                nullptr
            });
            //printf("Release detected on BUTTON %lld\n", control->id);
            return true;
        }
        return false;
    }
    static void render(WaControl * control, WaUI * ui, WaRenderAPI * api)
    {
        auto data = control->type_info.get_data<WaButtonData>();
        auto str = data->label.data();
        
        auto available_size = control->rect.size;
        available_size -= control->padding.a;
        available_size -= control->padding.b;
        
        auto string_size = Vec2{ui->string_get_width(str), ui->string_get_height(str)};
        
        auto pad = (available_size - string_size) / 2 + control->padding.a;
        
        ui->render_string(api, pad.x, pad.y, str, control->modulate);
    }
    static Vec2 get_min_size(WaControl * control, WaUI * ui)
    {
        auto data = control->type_info.get_data<WaButtonData>();
        auto str = data->label.data();
        
        auto string_size = Vec2{ui->string_get_width(str), ui->string_get_height(str)};
        string_size += control->padding.a;
        string_size += control->padding.b;
        return string_size;
    }
};

struct WaLineEditData
{
    std::string text;
    std::string text_transient;
    int transient_length = 0;
    int cursor = 0;
    int selection_end = -1;
    
    int ime_cursor = 0;
    int ime_selection_end = -1;
};
struct WaLineEdit
{
    // TODO:
    // - "input changed" signal and enter key support
    // - IME placement callback
    // - text cursor blink and better text cursor
    static void init(WaControl * control)
    {
        control->modulate = Color::BLACK;
        
        auto data = control->type_info.set_data(new WaLineEditData());
        data->text = "";
        control->draw_bg = false;
    }
    static void destruct(WaControl * control)
    {
        control->type_info.delete_data<WaLineEditData>();
    }
    static bool handle_event(WaControl * control, WaUI * ui, WaEvent event, Vec2 pos_offset)
    {
        auto data = control->type_info.get_data<WaLineEditData>();
        
        if (event.type == WaEvent::DEFOCUS)
        {
            data->transient_length = 0;
            data->text = data->text_transient;
            return false;
        }
        if (event.type == WaEvent::ACTION)
        {
            if (data->text_transient.size() > 0)
            {
                auto text = data->text_transient.data();
                
                if (event.subtype == WaEvent::Action::LEFT)
                    data->cursor = std::clamp(prev_pos_utf8(text, data->cursor), 0, (int)data->text.size());
                else if (event.subtype == WaEvent::Action::RIGHT)
                    data->cursor = std::clamp(next_pos_utf8(text, data->cursor), 0, (int)data->text.size());
                else if (event.subtype == WaEvent::Action::HOME)
                    data->cursor = 0;
                else if (event.subtype == WaEvent::Action::END)
                    data->cursor = (int)data->text.size();
            }
        }
        if (event.type == WaEvent::TEXTEDIT)
        {
            auto text = (const char *) event.ptr_data;
            printf("got text edit... `%s`\n", text);
            
            if (*text == '\x09') // horizontal tab
                text = " ";
            if (*text == '\x0A') // enter key. not supported yet
                return false;
            
            data->text_transient = data->text;
            
            if (data->selection_end > data->cursor)
                data->text_transient.erase(data->cursor, data->selection_end - data->cursor);
            
            data->text_transient.insert(data->cursor, text);
            
            data->transient_length = strlen(text);
            
            data->ime_cursor = (int)event.x;
            data->ime_selection_end = (int)event.y;
            
            return true;
        }
        if (event.type == WaEvent::TEXT)
        {
            //data->text = data->text_transient;
            puts("got text");
            auto text = (const char *) event.ptr_data;
            
            if (*text == '\x09') // horizontal tab
                text = " ";
            if (*text == '\x0A') // enter key. not supported yet
                return false;
            
            if (*text == '\x7F' || *text == '\x08')
            { 
                if (data->selection_end > data->cursor)
                    data->text.erase(data->cursor, data->selection_end - data->cursor);
                // delete key
                else if (*text == '\x7F' and (size_t)data->cursor < data->text.size())
                {
                    auto next = std::clamp(next_pos_utf8(data->text.data(), data->cursor), 0, (int)data->text.size());
                    data->text.erase(data->cursor, next - data->cursor);
                }
                // backspace key
                else if (*text == '\x08' and data->cursor > 0)
                {
                    auto prev = std::clamp(prev_pos_utf8(data->text.data(), data->cursor), 0, (int)data->text.size());
                    data->text.erase(prev, data->cursor - prev);
                    data->cursor = prev;
                }
            }
            else
            {
                if (data->selection_end > data->cursor)
                    data->text.erase(data->cursor, data->selection_end - data->cursor);
                
                data->text.insert(data->cursor, text);
                data->cursor += strlen(text);
            }
            
            data->selection_end = -1;
            data->text_transient = data->text;
            data->transient_length = 0;
            
            return true;
        }
        return false;
    }
    static void render(WaControl * control, WaUI * ui, WaRenderAPI * api)
    {
        ui->render_ninepatch(api, ui->micro_bg_texture, {{0, 0}, control->rect.size}, {{1, 1}, {4, 4}}, {{2.5, 2.5}, {1, 1}}, {micro_bg_width, micro_bg_height}, Color::OFFWHITE);
        
        auto data = control->type_info.get_data<WaLineEditData>();
        auto str = data->text_transient.data();
        
        auto available_size = control->rect.size;
        available_size -= control->padding.a;
        available_size -= control->padding.b;
        
        auto string_size = Vec2{ui->string_get_width(str), ui->string_get_height(str)};
        
        auto pad = (available_size - string_size) / 2 + control->padding.a;
        pad.x = control->padding.a.x;
        
        auto cursor_x = pad.x + ui->string_get_cursor_location(str, data->cursor);
        
        if (data->transient_length > 0)
        {
            auto substring = data->text_transient.substr(data->cursor, data->transient_length);
            auto substring_size = Vec2{ui->string_get_width(substring.data()), ui->string_get_height(substring.data())};
            auto rect = Rect2{{cursor_x - 1, pad.y}, substring_size};
            auto rect2 = rect;
            
            //printf("%d\n", bytes);
            auto bytes = substr_len_utf8(substring, 0, data->ime_cursor);
            auto substring2 = substring.substr(0, bytes);
            cursor_x += ui->string_get_width(substring2.data());
            
            rect.pos.y += rect.size.y - 2;
            rect.size.y = 2;
            ui->render_rect(api, rect, Color{127, 160, 192, 128});
            //ui->render_rect(api, rect, Color{127, 192, 255, 64});
            
            rect2.size.y += 1;
            
            ui->ime_rect_inform(rect2);
        }
        else
        {
            if (control->focused)
                ui->ime_rect_inform({pad, {1, 1}});
        }
        
        ui->render_string(api, pad.x, pad.y, str, control->modulate);
        
        if (control->focused)
            ui->render_rect(api, {{cursor_x - 1, pad.y}, {1, ui->string_get_height("|")}}, control->modulate);
        
    }
    static Vec2 get_min_size(WaControl * control, WaUI * ui)
    {
        auto data = control->type_info.get_data<WaLineEditData>();
        auto str = data->text_transient.data();
        
        auto string_size = Vec2{ui->string_get_width(str), ui->string_get_height(str)};
        string_size += control->padding.a;
        string_size += control->padding.b;
        string_size.x = std::max(128.0f, string_size.x);
        return string_size;
    }
    static void signal_destruct(WaControl * control, WaUI * ui, WaSignal * signal)
    {
        
    }
};

struct WaListData
{
    bool is_horizontal = false;
};
struct WaList
{
    static void init(WaControl * control)
    {
        control->type_info.set_data(new WaListData());
    }
    static bool handle_event(WaControl * control, WaUI * ui, WaEvent event, Vec2 pos_offset)
    {
        return false;
    }
    static bool reflow(WaControl * control, WaUI * ui)
    {
        auto data = control->type_info.get_data<WaListData>();
        auto is_horizontal = data ? data->is_horizontal : false;
        
        auto padded_rect = Rect2{{0, 0}, control->rect.size};
        padded_rect.pos += control->padding.a;
        padded_rect.size -= control->padding.a + control->padding.b;
        
        auto extra_spacing = 4;
        
        for (auto child_id : control->children)
        {
            auto control = ui->get_control(child_id);
            control->fit_rect(ui, padded_rect);
            control->reflow(ui);
            
            if (is_horizontal)
            {
                padded_rect.pos.x += control->rect.size.x + extra_spacing;
                padded_rect.size.x -= control->rect.size.x + extra_spacing;
            }
            else
            {
                padded_rect.pos.y += control->rect.size.y + extra_spacing;
                padded_rect.size.y -= control->rect.size.y + extra_spacing;
            }
        }
        
        return true;
    }
    static Vec2 get_min_size(WaControl * control, WaUI * ui) // FIXME iterate over children?
    {
        return {0, 0};
    }
};

void WaControl::think(WaUI * ui, uint64_t delta)
{
    for (auto child_id : children)
    {
        auto control = ui->get_control(child_id);
        control->think(ui, delta);
    }
    if (type_info.think)
        type_info.think(this, ui, delta);
}
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
    auto may_handle = true;
    auto ret = [&]()
    {
        auto global_rect = rect;
        global_rect.pos += pos_offset;
        if (event.type == WaEvent::MOUSE_BUTTON_PRESSED)
        {
            if (!global_rect.contains_point({event.x, event.y}))
            {
                may_handle = false;
                return false;
            }
            ui->focus_control_set(id);
            printf("focusing %d\n", id);
            ui->clicked_control_count += 1;
            ui->clicked_control = id;
            return true;
        }
        if (event.type == WaEvent::MOUSE_BUTTON_RELEASED)
        {
            may_handle = ui->clicked_control == id;
            // return may_handle; // FIXME: might fix a bug but haven't found the bug yet
            if (!global_rect.contains_point({event.x, event.y}))
            {
                return ui->clicked_control == id;
            }
            return true;
        }
        
        return false;
    }();
    
    if (event.type == WaEvent::DEFOCUS)
        focused = false;
    if (event.type == WaEvent::FOCUS)
        focused = true;
    
    if (may_handle and type_info.handle_event)
        ret |= type_info.handle_event(this, ui, event, pos_offset);
    
    return ret;
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
    auto did_reflow = false;
    if (type_info.reflow)
        did_reflow = type_info.reflow(this, ui);
    
    if (!did_reflow)
    {
        auto padded_rect = Rect2{{0, 0}, rect.size};
        padded_rect.pos += padding.a;
        padded_rect.size -= padding.a + padding.b;
        
        for (auto child_id : children)
        {
            auto control = ui->get_control(child_id);
            control->fit_rect(ui, padded_rect);
            control->reflow(ui);
        }
    }
}

void WaControl::clear_all_signals(WaUI * ui)
{
    for (auto & signal : signals)
        signal_destruct(ui, &signal);
    signals.clear();
    
    for (auto child_id : children)
    {
        auto control = ui->get_control(child_id);
        control->clear_all_signals(ui);
    }
}

void WaControl::render(WaUI * ui, WaRenderAPI * api)
{
    if (draw_bg)
        ui->render_ninepatch(api, ui->panel_texture, {{0, 0}, rect.size}, {{1, 1}, {14, 14}}, {{3.5, 3.5}, {9, 9}}, {panel_image_width, panel_image_height}, bg_color);
    
    if (type_info.render)
        type_info.render(this, ui, api);
}

Vec2 WaControl::get_min_size(WaUI * ui)
{
    auto ret = min_size;
    if (type_info.get_min_size)
        ret = ret.max(type_info.get_min_size(this, ui));
    return ret;
}

void WaControl::signal_destruct(WaUI * ui, WaSignal * signal)
{
    if (type_info.signal_destruct)
        type_info.signal_destruct(this, ui, signal);
}

void WaControl::fit_rect(WaUI * ui, Rect2 parent_rect)
{
    rect.pos = parent_rect.pos.lerp(parent_rect.size, anchor.pos);
    rect.set_end(parent_rect.pos.lerp(parent_rect.size, anchor.size));
    rect.size = rect.size.max(get_min_size(ui));
}

WaUI::WaUI()
{
    root_control_id = control_create_untyped();
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
                //printf("reset clicked control... was %lld\n", clicked_control);
                clicked_control = 0;
            }
        }
    }
    else
    {
        controls[root_control_id]->feed_event(this, event, {0, 0});
    }
}
uint64_t WaUI::control_create(uint64_t type_id)
{
    auto id = control_create_untyped();
    auto control = controls[id];
    
    control->type_id = type_id;
    control->type_info.type_id = type_id;
    
    if (control_types.count(type_id) > 0)
        control->type_info = control_types[type_id];
    if (control->type_info.init)
        control->type_info.init(&*control);
    
    return id;
}
uint64_t WaUI::control_create_untyped()
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

void WaUI::clear_all_signals()
{
    controls[root_control_id]->clear_all_signals(this);
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
        control->fit_rect(this, {{0, 0}, {size}});
        control->reflow(this);
        
        layout_dirty = false;
    }
}
void WaUI::render_scene(WaRenderAPI * api)
{
    api->draw_begin_frame(userdata);
    
    if (ime_rect != ime_rect_prev)
        api->ime_rect_inform(userdata, ime_rect.pos.x, ime_rect.pos.y, ime_rect.size.x, ime_rect.size.y);
    ime_rect_prev = ime_rect;
    
    rect_offset = {0, 0};
    
    api->draw_clip_rect_clear(userdata);
    
    last_clip_rect = {{0, 0}, size};
    api->draw_clip_rect_set(userdata, last_clip_rect.pos.x, last_clip_rect.pos.y, last_clip_rect.size.x, last_clip_rect.size.y);
    
    render_control(api, root_control_id);
    
    //render_string(api, 100, 100, "Drawing random text to the screen!!!");
    
    api->draw_finish_frame(userdata);
}

uint64_t WaUI::control_type_add(const char * name, WaControlAPI api)
{
    auto type_id = next_type_id;
    next_type_id += 1;
    
    std::string strname(name);
    control_types.insert({type_id, api});
    control_type_names.insert({type_id, strname});
    control_type_ids.insert({strname, type_id});
    
    return type_id;
}
template<typename T>
uint64_t WaUI::control_type_add_basis(const char * name)
{
    WaControlAPI api;
    if constexpr (requires { T::init; })
        api.init = T::init;
    if constexpr (requires { T::destruct; })
        api.destruct = T::destruct;
    if constexpr (requires { T::think; })
        api.think = T::think;
    if constexpr (requires { T::handle_event; })
        api.handle_event = T::handle_event;
    if constexpr (requires { T::reflow; })
        api.reflow = T::reflow;
    if constexpr (requires { T::render; })
        api.render = T::render;
    if constexpr (requires { T::get_min_size; })
        api.get_min_size = T::get_min_size;
    if constexpr (requires { T::signal_destruct; })
        api.signal_destruct = T::signal_destruct;
    
    return control_type_add(name, api);
}
uint64_t WaUI::control_type_get(const char * name)
{
    std::string strname(name);
    if (control_type_ids.count(strname) > 0)
        return control_type_ids[strname];
    return 0;
}
const char * WaUI::control_type_get_name(uint64_t type_id)
{
    if (control_type_names.count(type_id) > 0)
        return control_type_names[type_id].data();
    return nullptr;
}

void WaUI::init(WaRenderAPI * api)
{
    test_texture = api->texture_create(userdata, test_image_width, test_image_height, true, test_image);
    panel_texture = api->texture_create(userdata, panel_image_width, panel_image_height, true, panel_image);
    micro_bg_texture = api->texture_create(userdata, micro_bg_width, micro_bg_height, true, micro_bg_image);
    
    auto font_data = (unsigned char *)malloc(font_w * font_h * 4);
    auto i = 0;
    auto j = 0;
    auto bit = 0;
    for (unsigned long int y = 0; y < font_h; y++)
    {
        for (unsigned long int x = 0; x < font_w; x++)
        {
            font_data[i++] = 255;
            font_data[i++] = 255;
            font_data[i++] = 255;
            font_data[i++] = ((font_i[j] >> ((font_bits - 1) - bit)) & 1) ? 255 : 0;
            bit += 1;
            if (bit >= font_bits)
            {
                bit -= font_bits;
                j += 1;
            }
        }
    }
    
    for (uint32_t y0 = 0; y0 < font_rows*font_char_h; y0 += font_char_h)
    {
        for (uint32_t x0 = 0; x0 < font_cols*font_char_w; x0 += font_char_w)
        {
            int32_t left = font_char_w;
            int32_t right = 0;
            for (uint32_t y = y0; y < y0 + font_char_h; y += 1)
            {
                for (uint32_t x = x0; x < x0 + font_char_w; x += 1)
                {
                    size_t index = (y * font_w + x) * 4 + 3;
                    auto c = font_data[index];
                    if (c > 127)
                    {
                        left = std::min(int32_t(x - x0), left);
                        right = std::max(int32_t(x - x0), right);
                    }
                }
            }
            
            auto n = font_advance.size() + 1;
            if (
                (n >= 0x2E80 and n <= 0x30FF) or
                (n >= 0x3130 and n <= 0x319F) or
                (n >= 0x31C0 and n <= 0x9FFF) or
                (n >= 0xF900 and n <= 0xFAFF) or
                (n >= 0xFF00 and n <= 0xFF60) or
                (n >= 0xFFE0 and n <= 0xFFE6)
            )
            {
                left = 1;
                right = font_char_w - 1;
            }
            
            if (left <= right)
            {
                font_advance.push_back(right - left + 2);
                font_offset.push_back(left);
            }
            else
            {
                font_advance.push_back(5);
                font_offset.push_back(0);
            }
        }
    }
    
    font_texture = api->texture_create(userdata, font_w, font_h, true, font_data);
    free(font_data);
    
    control_type_add_basis<WaList>("List");
    control_type_add_basis<WaButton>("Button");
    control_type_add_basis<WaLineEdit>("LineEdit");
};
void WaUI::clean_up(WaRenderAPI * api)
{
    api->texture_destroy(userdata, test_texture);
    test_texture = 0;
    api->texture_destroy(userdata, panel_texture);
    panel_texture = 0;
    api->texture_destroy(userdata, font_texture);
    font_texture = 0;
};
float WaUI::string_get_height(const char * string)
{
    return font_char_h;
}
float WaUI::string_get_width(const char * string)
{
    auto n = 0;
    while (*string != 0)
    {
        auto c = next_codepoint_utf8(string);
        n += font_advance[c];
        if (c == 0)
            break;
    }
    return n;
}
float WaUI::string_get_cursor_location(const char * string, int cursor)
{
    auto n = 0;
    while (*string != 0 and cursor > 0)
    {
        auto old_string = string;
        auto c = next_codepoint_utf8(string);
        n += font_advance[c];
        cursor -= string - old_string;
        if (c == 0)
            break;
    }
    return n;
}
void WaUI::render_string(WaRenderAPI * api, float x, float y, const char * string, Color color)
{
    x += 0;
    while (*string != 0)
    {
        //render_ascii_char(api, x - offset, y, *string, color);
        //x += font_advance[(uint8_t)*string];
        //string += 1;
        auto c = next_codepoint_utf8(string);
        if (c == 0)
            break;
        auto offset = font_offset[c];
        render_char(api, x - offset, y, c, color);
        x += font_advance[c];
    }
}
void WaUI::render_char(WaRenderAPI * api, float x, float y, uint32_t c, Color color)
{
    x += rect_offset.x;
    y += rect_offset.y;
    if (px_snapped_rendering)
    {
        x = round(x);
        y = round(y);
    }
    
    float c_x = (c % font_cols) * font_char_w;
    float c_y = (c / font_cols) * font_char_h;
    api->draw_texture_rect(userdata,
        x, y, font_char_w, font_char_h,
        color.r, color.g, color.b, color.a,
        font_texture, c_x, c_y, font_char_w, font_char_h,
        font_w, font_h
    );
}

void WaUI::render_control(WaRenderAPI * api, uint64_t id)
{
    if (controls.count(id) == 0)
        return;
    
    auto & control = controls[id];
    
    if (!control->visible)
        return;
    
    auto pos = control->rect.pos + rect_offset;
    auto prev_rect_offset = rect_offset;
    
    if (control->clip_children)
    {
        last_clip_rect = last_clip_rect.clip({pos, control->rect.size});
        last_clip_rect = last_clip_rect.round();
        api->draw_clip_rect_set(userdata, last_clip_rect.pos.x, last_clip_rect.pos.y, last_clip_rect.size.x, last_clip_rect.size.y);
    }
    
    rect_offset = pos;
    
    control->render(this, api);
    //printf("rendering %d (%f %f %f %f)\n", id, pos.x, pos.y, size.x, size.y);
    
    auto clip_at_start = last_clip_rect;
    
    for (auto child_id : get_children(id))
    {
        render_control(api, child_id);
        last_clip_rect = clip_at_start;
        api->draw_clip_rect_set(userdata, last_clip_rect.pos.x, last_clip_rect.pos.y, last_clip_rect.size.x, last_clip_rect.size.y);
    }
    
    rect_offset = prev_rect_offset;
};

void WaUI::render_ninepatch(WaRenderAPI * api, uint64_t texture, Rect2 dest, Rect2 src_outer, Rect2 src_inner, Vec2 texture_size, Color color)
{
    dest.pos += rect_offset;
    
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

void WaUI::render_rect(WaRenderAPI * api, Rect2 rect, Color color)
{
    rect.pos += rect_offset;
    
    if (px_snapped_rendering)
        rect = rect.round();
    
    api->draw_rect(userdata,
        rect.pos.x, rect.pos.y, rect.size.x, rect.size.y,
        color.r, color.g, color.b, color.a
    );
}

void WaUI::ime_rect_inform(Rect2 rect)
{
    rect.pos += rect_offset;
    rect = rect.round();
    ime_rect = rect;
}

uint64_t WaUI::focus_control_get()
{
    return focused_control;
}
void WaUI::focus_control_set(uint64_t new_control)
{
    if (focused_control == new_control)
        return;
    
    if (focused_control != 0 and controls.count(focused_control) != 0)
        controls[focused_control]->handle_event(this, WaEvent{WaEvent::Type::DEFOCUS}, {0, 0});
    
    ime_rect_inform({{0, 0}, {0, 0}});
    printf("%f %f\n", ime_rect.size.x, ime_rect.size.y);
    
    focused_control = new_control;
    controls[focused_control]->handle_event(this, WaEvent{WaEvent::Type::FOCUS}, {0, 0});
}
