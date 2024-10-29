// https://jsfiddle.net/h1kmqz9f/2/
// https://jsfiddle.net/vwoeja89/13/

#include <unordered_map>
#include <thread>
#include <functional>
#include <cstdint>

#include "waui.hpp"

#include <SDL.h>

void feed_event_to_ui(SDL_Event event, WaUI & ui)
{
    if (event.type == SDL_WINDOWEVENT)
    {
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || 
            event.window.event == SDL_WINDOWEVENT_RESIZED)
            ui.feed_event(WaEvent{WaEvent::Type::RESIZE, 0, (float)event.window.data1, (float)event.window.data2});
    }
    else if (event.type == SDL_MOUSEMOTION)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_POSITION, 0, (float)event.motion.x, (float)event.motion.y});
    else if (event.type == SDL_MOUSEBUTTONDOWN)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_BUTTON_PRESSED, event.button.button, (float)event.button.x, (float)event.button.y});
    else if (event.type == SDL_MOUSEBUTTONUP)
        ui.feed_event(WaEvent{WaEvent::Type::MOUSE_BUTTON_RELEASED, event.button.button, (float)event.button.x, (float)event.button.y});
    else if (event.type == SDL_KEYDOWN or event.type == SDL_KEYUP)
    {
        auto wa_event = WaEvent{WaEvent::Type::ACTION, WaEvent::Action::INVALID};
        if (event.type == SDL_KEYUP)
            wa_event.type = WaEvent::Type::ACTION_RELEASED;
        
        auto mod = SDL_GetModState();
        if (mod & KMOD_SHIFT)
            wa_event.data |= WaEvent::ActionMod::SHIFT;
        
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
            if (event.type == SDL_KEYDOWN)
                ui.feed_event(WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, "\x08"});
            break;
        case SDLK_TAB:
            if (event.type == SDL_KEYDOWN)
                ui.feed_event(WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, "\x09"});
            break;
        case SDLK_RETURN:
            if (event.type == SDL_KEYDOWN)
                ui.feed_event(WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, "\x0A"});
            break;
        case SDLK_DELETE:
            if (event.type == SDL_KEYDOWN)
                ui.feed_event(WaEvent{WaEvent::Type::TEXT, 0, 0, 0, 0, "\x7F"});
            break;
        }
        
        if (event.key.keysym.sym == SDLK_c and (mod & KMOD_CTRL))
            wa_event.subtype = WaEvent::Action::COPY;
        else if (event.key.keysym.sym == SDLK_x and (mod & KMOD_CTRL))
            wa_event.subtype = WaEvent::Action::CUT;
        else if (event.key.keysym.sym == SDLK_v and (mod & KMOD_CTRL))
            wa_event.subtype = WaEvent::Action::PASTE;
        else if (event.key.keysym.sym == SDLK_v and (mod & KMOD_CTRL))
            wa_event.subtype = WaEvent::Action::PASTE;
        else if (event.key.keysym.sym == SDLK_a and (mod & KMOD_CTRL))
            wa_event.subtype = WaEvent::Action::SELECT_ALL;
        else if (event.key.keysym.sym == SDLK_z and (mod & KMOD_CTRL))
            wa_event.subtype = WaEvent::Action::UNDO;
        else if (event.key.keysym.sym == SDLK_y and (mod & KMOD_CTRL))
            wa_event.subtype = WaEvent::Action::REDO;
        
        if (event.key.keysym.sym == SDLK_z and (mod & KMOD_CTRL) and (mod & KMOD_SHIFT))
            wa_event.subtype = WaEvent::Action::REDO;
        
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
    std::mutex events_mutex;
    std::vector<SDL_Event> events;
    std::vector<std::function<void(void)>> render_commands;
    SDL_Renderer * renderer;
    bool ime_started = false;
    std::unordered_map<uint64_t, SDL_Texture *> textures;
    uint64_t next_id = 1;
};

bool dead = false;

int application_main(CallbackContext * context)
{
    auto ui = WaUI();
    ui.userdata = context;
    printf("---------%p\n", context);
    fflush(stdout);
    
    auto sdl_begin_frame = [](void * userdata)
    {
        auto context = (CallbackContext *) userdata;
        context->events_mutex.lock();
        context->render_commands.clear();
        context->render_commands.push_back([=]()
        {
            auto renderer = context->renderer;
            SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
            SDL_RenderClear(renderer);
        });
    };
    auto sdl_finish_frame = [](void * userdata)
    {
        auto context = (CallbackContext *) userdata;
        context->render_commands.push_back([=]()
        {
            auto renderer = context->renderer;
            SDL_RenderPresent(renderer);
        });
        context->events_mutex.unlock();
    };
    auto sdl_draw_rect = [](void * userdata, float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        auto context = (CallbackContext *) userdata;
        ((CallbackContext *) userdata)->render_commands.push_back([=]()
        {
            auto renderer = context->renderer;
            SDL_Rect rect;
            rect.x = x;
            rect.y = y;
            rect.w = w;
            rect.h = h;
            SDL_SetRenderDrawColor(renderer, r, g, b, a);
            SDL_RenderFillRect(renderer, &rect);
        });
    };
    auto sdl_draw_texture_rect = [](void * userdata,
        float x, float y, float w, float h,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a,
        uint64_t tex, float tex_x, float tex_y, float tex_w, float tex_h,
        uint32_t tex_size_w, uint32_t tex_size_h)
    {
        auto context = (CallbackContext *) userdata;
        ((CallbackContext *) userdata)->render_commands.push_back([=]()
        {
            auto renderer = context->renderer;
            auto & textures = context->textures;
            
            auto texture = textures[tex];
            
            SDL_Vertex verts[4] = {
                {{x    , y    }, {r, g, b, a}, {(tex_x        ) / tex_size_w, (tex_y        ) / tex_size_h}},
                {{x + w, y    }, {r, g, b, a}, {(tex_x + tex_w) / tex_size_w, (tex_y        ) / tex_size_h}},
                {{x    , y + h}, {r, g, b, a}, {(tex_x        ) / tex_size_w, (tex_y + tex_h) / tex_size_h}},
                {{x + w, y + h}, {r, g, b, a}, {(tex_x + tex_w) / tex_size_w, (tex_y + tex_h) / tex_size_h}},
            };
            
            int indexes[6] = {0, 1, 2, 2, 1, 3};
            
            SDL_RenderGeometry(renderer, texture, verts, 4, indexes, 6);
        });
    };
    
    auto sdl_clip_rect_set = [](void * userdata, float x, float y, float w, float h)
    {
        auto context = (CallbackContext *) userdata;
        ((CallbackContext *) userdata)->render_commands.push_back([=]()
        {
            auto renderer = context->renderer;
            SDL_Rect rect;
            rect.x = (int)x;
            rect.y = (int)y;
            rect.w = (int)w;
            rect.h = (int)h;
            
            // for debugging
            //SDL_RenderSetClipRect(renderer, nullptr);
            //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            //SDL_RenderDrawRect(renderer, &rect);
            
            SDL_RenderSetClipRect(renderer, &rect);
        });
    };
    auto sdl_clip_rect_clear = [](void * userdata)
    {
        auto context = (CallbackContext *) userdata;
        ((CallbackContext *) userdata)->render_commands.push_back([=]()
        {
            auto renderer = context->renderer;
            SDL_RenderSetClipRect(renderer, nullptr);
        });
    };
    
    auto sdl_ime_rect_inform = [](void * userdata, float x, float y, float w, float h)
    {
        auto context = (CallbackContext *) userdata;
        context->events_mutex.lock();
        context->render_commands.push_back([=]()
        {
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
                    //puts("called SDL_StartTextInput");
                }
                context->ime_started = true;
                //printf("setting ime rect to %d %d %d %d\n", rect.x, rect.y, rect.w, rect.h);
            }
            else
            {
                SDL_SetTextInputRect(nullptr);
                if (context->ime_started)
                {
                    SDL_StopTextInput();
                    //puts("called SDL_StopTextInput");
                }
                context->ime_started = false;
                //puts("clearing ime rect");
            }
        });
        context->events_mutex.unlock();
    };
    
    auto sdl_texture_create = [](void * userdata, uint32_t w, uint32_t h, bool filter, uint8_t bytes_per_pixel, const unsigned char * data)
    {
        auto context = (CallbackContext *) userdata;
        context->events_mutex.lock();
        
        auto texture_id = context->next_id;
        context->next_id += 1;
        
        const unsigned char * safe_data = (unsigned char *)malloc(w * h * bytes_per_pixel);
        if (!safe_data)
            throw;
        memcpy((void *)safe_data, (void *)data, w * h * bytes_per_pixel);
        
        context->render_commands.push_back([=]()
        {
            auto renderer = context->renderer;
            auto & textures = context->textures;
            
            if (filter)
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
            else
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
            
            auto format = SDL_PIXELFORMAT_RGBA32;
            if (bytes_per_pixel == 3)
                format = SDL_PIXELFORMAT_RGB24;
            
            auto texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STATIC, w, h);
            textures.insert({texture_id, texture});
            
            SDL_UpdateTexture(texture, NULL, safe_data, w * 4);
            free((void *)safe_data);
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        });
        context->events_mutex.unlock();
        return texture_id;
    };
    auto sdl_texture_destroy = [](void * userdata, uint32_t texture_id)
    {
        auto context = (CallbackContext *) userdata;
        context->events_mutex.lock();
        context->render_commands.push_back([=]()
        {
            auto & textures = context->textures;
            auto texture = textures[texture_id];
            textures.erase(texture_id);
            SDL_DestroyTexture(texture);
        });
        context->events_mutex.unlock();
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
    
    auto sdl_clipboard_text_get = [](void * userdata) -> char *
    {
        return SDL_GetClipboardText();
    };
    auto sdl_clipboard_text_free = [](void * userdata, char * str)
    {
        SDL_free(str);
    };
    auto sdl_clipboard_text_set = [](void * userdata, const char * str)
    {
        SDL_SetClipboardText(str);
    };
    
    auto sys_api = WaSystemAPI();
    sys_api.clipboard_text_get = sdl_clipboard_text_get;
    sys_api.clipboard_text_free = sdl_clipboard_text_free;
    sys_api.clipboard_text_set = sdl_clipboard_text_set;
    
    ui.init(sys_api, &api);
    
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
    
    while (1)
    {
        uint64_t new_time = SDL_GetTicks64();
        
        // waui event phase
        
        context->events_mutex.lock();
        for (auto & event : context->events)
        {
            if (event.type == SDL_QUIT)
            {
                context->events_mutex.unlock();
                dead = true;
                goto exit;
            }
            feed_event_to_ui(event, ui);
        }
        context->events.clear();
        context->events_mutex.unlock();
        
        // application logic phase
        
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
        
        // waui logic phase
        
        ui.think(new_time - time);
        ui.clear_all_signals();
        
        // waui render phase
        
        ui.render_scene(&api);
        
        time = new_time;
        SDL_Delay(1);
    }
    exit:
    
    context->events_mutex.lock();
    context->render_commands.clear();
    context->events_mutex.unlock();
    
    ui.clean_up(&api);

    return 0;
}

int main()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return fprintf(stderr, "failed to initialize SDL"), -1;
    
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");
    
    SDL_Window * window = SDL_CreateWindow("WaUI Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window)
        return fprintf(stderr, "failed to open SDL window"), -1;
    
    SDL_SetWindowResizable(window, SDL_TRUE);
    
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
        return fprintf(stderr, "failed to open SDL renderer"), -1;
    
    auto context = CallbackContext();
    context.renderer = renderer;
    
    std::thread app_thread(application_main, &context);
    
    auto event_pumper = [](void * userdata, SDL_Event * event) -> int
    {
        auto context = (CallbackContext *)userdata;
        context->events_mutex.lock();
        
        if (context->render_commands.size() > 0)
        {
            auto render_commands = std::move(context->render_commands);
            context->render_commands = {};
            context->events_mutex.unlock();
            
            for (auto & c : render_commands)
                c();
            render_commands.clear();
        }
        else
            context->events_mutex.unlock();
        
        context->events_mutex.lock();
        if (event->type == SDL_POLLSENTINEL)
        {
            context->events_mutex.unlock();
            return 0;
        }
        context->events.push_back(*event);
        context->events_mutex.unlock();
        
        return 0;
    };
    
    SDL_AddEventWatch((SDL_EventFilter)event_pumper, (void *)&context);
    
    while (!dead)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event));
        SDL_Delay(1);
    }
    
    app_thread.join();
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    SDL_Quit();

    return 0;
}