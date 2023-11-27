// https://jsfiddle.net/h1kmqz9f/2/
// https://jsfiddle.net/vwoeja89/13/


/*
TODO:
- grid container (true and stretchy)
- scrolling container
- tab container

- checkbox/radio buttons
- dropdown menu
- label with line wrapping
- single-line text input 
- slider input
- number input
- knob input

- cached min size calculation

- multi-line text input (as an extension)

- text shaping callback system
*/

#include <unordered_map>
#include <cstdint>

#include "waui.hpp"

#include <SDL.h>

void feed_event_to_ui(SDL_Event event, WaUI & ui)
{
    if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            ui.feed_event(WaEvent{WaEvent::Type::RESIZE, 0, (float)event.window.data1, (float)event.window.data2});
    }
    else if (event.type == SDL_MOUSEMOTION)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_POSITION, 0, (float)event.motion.x, (float)event.motion.y});
    else if (event.type == SDL_MOUSEBUTTONDOWN)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_BUTTON_PRESSED, event.button.button, (float)event.button.x, (float)event.button.y});
    else if (event.type == SDL_MOUSEBUTTONUP)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_BUTTON_RELEASED, event.button.button, (float)event.button.x, (float)event.button.y});
    else if (event.type == SDL_KEYDOWN)
    {
        auto wa_event = WaEvent{WaEvent::Type::ACTION, WaEvent::Action::INVALID, 0, 0, 1};
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
        case SDLK_HOME:
            wa_event.subtype = WaEvent::Action::HOME;
            break;
        case SDLK_END:
            wa_event.subtype = WaEvent::Action::END;
            break;
        case SDLK_BACKSPACE:
            ui.feed_event(WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, "\x08"});
            break;
        case SDLK_TAB:
            ui.feed_event(WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, "\x09"});
            break;
        case SDLK_RETURN:
            ui.feed_event(WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, "\x0A"});
            break;
        case SDLK_DELETE:
            ui.feed_event(WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, "\x7F"});
            break;
        }
        if (wa_event.subtype != WaEvent::Action::INVALID)
            ui.feed_event(wa_event);
    }
    else if (event.type == SDL_TEXTEDITING)
    {
        auto wa_event = WaEvent{WaEvent::Type::TEXTEDIT};
        wa_event.x = event.edit.start;
        wa_event.y = event.edit.length;
        wa_event.ptr_data = event.edit.text;
        ui.feed_event(wa_event);
        
        printf("text edit: `%s` %d %d\n", event.edit.text, event.edit.start, event.edit.length);
    }
    else if (event.type == SDL_TEXTEDITING_EXT)
    {
        auto wa_event = WaEvent{WaEvent::Type::TEXTEDIT};
        wa_event.x = event.editExt.start;
        wa_event.y = event.editExt.length;
        wa_event.ptr_data = event.editExt.text;
        ui.feed_event(wa_event);
        
        printf("text edit extended: `%s` %d %d\n", event.editExt.text, event.editExt.start, event.editExt.length);
        //printf("text edit extended: %s\n", event.editExt.text);
        SDL_free(event.editExt.text);
    }
    else if (event.type == SDL_TEXTINPUT)
    {
        auto wa_event = WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, 0};
        wa_event.ptr_data = event.edit.text;
        ui.feed_event(wa_event);
        
        //printf("text input: %s\n", event.text.text);
    }
}

struct CallbackContext
{
    SDL_Renderer * renderer;
    bool ime_started = false;
    std::unordered_map<uint64_t, SDL_Texture *> textures;
    uint64_t next_id = 1;
};

int main()
{
    setbuf(stdout, nullptr);
    
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return fprintf(stderr, "failed to initialize SDL"), -1;
    
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");
    
    auto window = SDL_CreateWindow("WaUI Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window)
        return fprintf(stderr, "failed to open SDL window"), -1;
    
    SDL_SetWindowResizable(window, SDL_TRUE);
    
    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
        return fprintf(stderr, "failed to open SDL renderer"), -1;
    
    auto ui = WaUI();
    
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
    
    auto sdl_clip_rect_set = [](void * userdata, float x, float y, float w, float h)
    {
        auto context = (CallbackContext *) userdata;
        auto renderer = context->renderer;
        
        SDL_Rect rect;
        rect.x = (int)x;
        rect.y = (int)y;
        rect.w = (int)w;
        rect.h = (int)h;
        
        //SDL_RenderSetClipRect(renderer, nullptr);
        //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        //SDL_RenderDrawRect(renderer, &rect);
        
        SDL_RenderSetClipRect(renderer, &rect);
    };
    auto sdl_clip_rect_clear = [](void * userdata)
    {
        auto context = (CallbackContext *) userdata;
        auto renderer = context->renderer;
        SDL_RenderSetClipRect(renderer, nullptr);
    };
    
    auto sdl_ime_rect_inform = [](void * userdata, float x, float y, float w, float h)
    {
        auto context = (CallbackContext *) userdata;
        
        SDL_Rect rect;
        rect.x = (int)x;
        rect.y = (int)y;
        rect.w = (int)w;
        rect.h = (int)h;
        if (rect.w * rect.h > 0)
        {
            SDL_SetTextInputRect(&rect);
            if (!context->ime_started)
            {
                SDL_StartTextInput();
                puts("called SDL_StartTextInput");
            }
            context->ime_started = true;
            printf("setting ime rect to %d %d %d %d\n", rect.x, rect.y, rect.w, rect.h);
        }
        else
        {
            SDL_SetTextInputRect(nullptr);
            if (context->ime_started)
            {
                SDL_StopTextInput();
                puts("called SDL_StopTextInput");
            }
            context->ime_started = false;
            puts("clearing ime rect");
        }
    };
    
    auto sdl_texture_create = [](void * userdata, uint32_t w, uint32_t h, bool filter, const unsigned char * data)
    {
        auto context = (CallbackContext *) userdata;
        auto renderer = context->renderer;
        auto & textures = context->textures;
        
        if (filter)
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        else
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        
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
    
    auto api = WaRenderAPI();
    api.draw_begin_frame = sdl_begin_frame;
    api.draw_finish_frame = sdl_finish_frame;
    api.draw_rect = sdl_draw_rect;
    api.draw_texture_rect = sdl_draw_texture_rect;
    api.draw_clip_rect_set = sdl_clip_rect_set;
    api.draw_clip_rect_clear = sdl_clip_rect_clear;
    api.ime_rect_inform = sdl_ime_rect_inform;
    api.texture_create = sdl_texture_create;
    api.texture_destroy = sdl_texture_destroy;
    
    ui.init(&api);
    
    ui.get_control(ui.root_control_id)->anchor = {{0.1, 0.1}, {0.9, 0.9}};
    
    auto list_type_id = ui.control_type_get("List");
    auto list_id = ui.control_create(list_type_id);
    ui.control_add_child(ui.root_control_id, list_id);
    
    ui.get_control(list_id)->anchor = {{0.1, 0.1}, {0.9, 0.9}};
    
    auto button_type_id = ui.control_type_get("Button");
    auto button_1_id = ui.control_create(button_type_id);
    auto button_2_id = ui.control_create(button_type_id);
    ui.control_add_child(list_id, button_1_id);
    ui.control_add_child(list_id, button_2_id);
    
    ui.get_control(button_1_id)->bg_color = Color{64, 128, 192, 255};
    ui.get_control(button_2_id)->modulate = Color::BLACK;
    
    auto line_edit_type_id = ui.control_type_get("LineEdit");
    auto line_edit_1_id = ui.control_create(line_edit_type_id);
    ui.control_add_child(list_id, line_edit_1_id);
    
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
        
        auto button_1 = ui.get_control(button_1_id);
        for (auto & signal : button_1->signals)
        {
            if (signal.type == 0)
                printf("Pressed on button 1!\n");
            else if (signal.type == 1)
                printf("Released on button 1!\n");
        }
        auto button_2 = ui.get_control(button_2_id);
        for (auto & signal : button_2->signals)
        {
            if (signal.type == 0)
                printf("Ah, yes, button 2. It just got pressed.\n");
            else if (signal.type == 1)
                printf("Ah, yes, button 2. It just got released!\n");
        }
        
        ui.think(new_time - time);
        
        ui.clear_all_signals();
        
        ui.render_scene(&api);
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