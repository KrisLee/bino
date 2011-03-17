/*
 * This file is part of bino, a 3D video player.
 *
 * Copyright (C) 2010-2011
 * Martin Lambers <marlam@marlam.de>
 * Alexey Osipov <lion-simba@pridelands.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include "config.h"

#include <vector>
#include <unistd.h>

#include "exc.h"
#include "str.h"
#include "msg.h"
#include "timer.h"

#include "controller.h"
#include "media_data.h"
#include "media_input.h"
#include "audio_output.h"
#include "video_output_qt.h"
#include "player.h"


player_init_data::player_init_data() :
    log_level(msg::INF),
    urls(),
    video_stream(0),
    audio_stream(0),
    subtitles_stream(0),
    benchmark(false),
    fullscreen(false),
    center(false),
    stereo_layout_override(false),
    stereo_layout(video_frame::mono),
    stereo_layout_swap(false),
    stereo_mode_override(false),
    stereo_mode(parameters::mono_left),
    stereo_mode_swap(false),
    params()
{
}

player_init_data::~player_init_data()
{
}

void player_init_data::save(std::ostream &os) const
{
    s11n::save(os, static_cast<int>(log_level));
    s11n::save(os, urls);
    s11n::save(os, video_stream);
    s11n::save(os, audio_stream);
    s11n::save(os, subtitles_stream);
    s11n::save(os, benchmark);
    s11n::save(os, fullscreen);
    s11n::save(os, center);
    s11n::save(os, stereo_layout_override);
    s11n::save(os, static_cast<int>(stereo_layout));
    s11n::save(os, stereo_layout_swap);
    s11n::save(os, stereo_mode_override);
    s11n::save(os, static_cast<int>(stereo_mode));
    s11n::save(os, stereo_mode_swap);
    s11n::save(os, params);
}

void player_init_data::load(std::istream &is)
{
    int x;
    s11n::load(is, x);
    log_level = static_cast<msg::level_t>(x);
    s11n::load(is, urls);
    s11n::load(is, video_stream);
    s11n::load(is, audio_stream);
    s11n::load(is, subtitles_stream);
    s11n::load(is, benchmark);
    s11n::load(is, fullscreen);
    s11n::load(is, center);
    s11n::load(is, stereo_layout_override);
    s11n::load(is, x);
    stereo_layout = static_cast<video_frame::stereo_layout_t>(x);
    s11n::load(is, stereo_layout_swap);
    s11n::load(is, stereo_mode_override);
    s11n::load(is, x);
    stereo_mode = static_cast<parameters::stereo_mode_t>(x);
    s11n::load(is, stereo_mode_swap);
    s11n::load(is, params);
}


// The single player instance
player *global_player = NULL;

// The registered controllers
std::vector<controller *> global_controllers;

player::player(type t) :
    _media_input(NULL), _audio_output(NULL), _video_output(NULL)
{
    if (t == master)
    {
        make_master();
    }
    reset_playstate();
}

player::~player()
{
    if (global_player == this)
    {
        global_player = NULL;
    }
    delete _media_input;
    delete _audio_output;
    delete _video_output;
}

float player::normalize_pos(int64_t pos)
{
    double pos_min = _start_pos + _media_input->initial_skip();
    double pos_max = pos_min + _media_input->duration();
    double npos = (pos_max > pos_min ? (static_cast<double>(pos) - pos_min) / (pos_max - pos_min) : 0.0f);
    return npos;
}

void player::reset_playstate()
{
    _running = false;
    _first_frame = false;
    _need_frame_now = false;
    _need_frame_soon = false;
    _drop_next_frame = false;
    _previous_frame_dropped = false;
    _in_pause = false;
    _quit_request = false;
    _pause_request = false;
    _seek_request = 0;
    _set_pos_request = -1.0f;
}

void player::stop_playback()
{
    if (_video_output)
    {
        _video_output->exit_fullscreen();
        _video_output->prepare_next_frame(video_frame());
        _video_output->activate_next_frame();
    }
    notify(notification::play, true, false);
}

video_output *player::create_video_output()
{
    return new video_output_qt(_benchmark);
}

audio_output *player::create_audio_output()
{
    return new audio_output();
}

void player::make_master()
{
    if (global_player)
    {
        throw exc("Cannot create a second master player.");
    }
    global_player = this;
}

void player::open(const player_init_data &init_data)
{
    // Initialize basics
    msg::set_level(init_data.log_level);
    _benchmark = init_data.benchmark;
    reset_playstate();

    // Create media input
    _media_input = new media_input();
    _media_input->open(init_data.urls);
    if (_media_input->video_streams() == 0)
    {
        throw exc("No video streams found.");
    }
    if (init_data.stereo_layout_override)
    {
        if (!_media_input->stereo_layout_is_supported(init_data.stereo_layout, init_data.stereo_layout_swap))
        {
            throw exc("Cannot set requested stereo layout: incompatible media.");
        }
        _media_input->set_stereo_layout(init_data.stereo_layout, init_data.stereo_layout_swap);
    }
    if (_media_input->video_streams() < init_data.video_stream + 1)
    {
        throw exc(str::asprintf("Video stream %d not found.", init_data.video_stream + 1));
    }
    _media_input->select_video_stream(init_data.video_stream);
    if (_media_input->audio_streams() > 0 && _media_input->audio_streams() < init_data.audio_stream + 1)
    {
        throw exc(str::asprintf("Audio stream %d not found.", init_data.audio_stream + 1));
    }
    if (_media_input->audio_streams() > 0)
    {
        _media_input->select_audio_stream(init_data.audio_stream);
    }

    // Create audio output
    if (_media_input->audio_streams() > 0 && !_benchmark)
    {
        _audio_output = create_audio_output();
    }
    if (_audio_output)
    {
        _audio_output->init();
    }

    // Create video output
    _video_output = create_video_output();
    if (_video_output)
    {
        _video_output->init();
    }

    // Initialize output parameters
    _params = init_data.params;
    _params.set_defaults();
    if (init_data.stereo_mode_override)
    {
        _params.stereo_mode = init_data.stereo_mode;
        _params.stereo_mode_swap = init_data.stereo_mode_swap;
    }
    else
    {
        if (_media_input->video_frame_template().stereo_layout == video_frame::mono)
        {
            _params.stereo_mode = parameters::mono_left;
        }
        else if (_video_output && _video_output->supports_stereo())
        {
            _params.stereo_mode = parameters::stereo;
        }
        else
        {
            _params.stereo_mode = parameters::red_cyan_dubois;
        }
        _params.stereo_mode_swap = false;
    }

    // Set initial parameters
    if (_video_output)
    {
        _video_output->set_parameters(_params);
        _video_output->set_suitable_size(
                _media_input->video_frame_template().width,
                _media_input->video_frame_template().height,
                _media_input->video_frame_template().aspect_ratio,
                _params.stereo_mode);
        if (init_data.fullscreen)
        {
            _video_output->enter_fullscreen();
        }
        if (init_data.center)
        {
            _video_output->center();
        }
        _video_output->process_events();
    }
}

int64_t player::step(bool *more_steps, int64_t *seek_to, bool *prep_frame, bool *drop_frame, bool *display_frame)
{
    *more_steps = false;
    *seek_to = -1;
    *prep_frame = false;
    *drop_frame = false;
    *display_frame = false;

    if (_quit_request)
    {
        stop_playback();
        return 0;
    }
    else if (!_running)
    {
        // Read initial data and start output
        _media_input->start_video_frame_read();
        _video_frame = _media_input->finish_video_frame_read();
        if (!_video_frame.is_valid())
        {
            msg::dbg("Empty video input.");
            stop_playback();
            return 0;
        }
        _video_pos = _video_frame.presentation_time;
        if (_audio_output)
        {
            _media_input->start_audio_blob_read(_audio_output->required_initial_data_size());
            audio_blob blob = _media_input->finish_audio_blob_read();
            if (!blob.is_valid())
            {
                msg::dbg("Empty audio input.");
                stop_playback();
                return 0;
            }
            _audio_pos = blob.presentation_time;
            _audio_output->data(blob);
            _media_input->start_audio_blob_read(_audio_output->required_update_data_size());
            _master_time_start = _audio_output->start();
            _master_time_pos = _audio_pos;
            _current_pos = _audio_pos;
        }
        else
        {
            _master_time_start = timer::get_microseconds(timer::monotonic);
            _master_time_pos = _video_pos;
            _current_pos = _video_pos;
        }
        _start_pos = _current_pos;
        _fps_mark_time = timer::get_microseconds(timer::monotonic);
        _frames_shown = 0;
        _running = true;
        if (_media_input->initial_skip() > 0)
        {
            _seek_request = _media_input->initial_skip();
        }
        else
        {
            _need_frame_now = false;
            _need_frame_soon = true;
            _first_frame = true;
            *more_steps = true;
            *prep_frame = true;
            return 0;
        }
    }
    if (_seek_request != 0 || _set_pos_request >= 0.0f)
    {
        int64_t old_pos = _current_pos;
        if (_set_pos_request >= 0.0f)
        {
            int64_t dest_pos_min = _start_pos + _media_input->initial_skip();
            int64_t dest_pos_max = dest_pos_min + _media_input->duration() - 2000000;
            if (dest_pos_max <= dest_pos_min)
            {
                *seek_to = _current_pos;
            }
            else
            {
                *seek_to = static_cast<double>(_set_pos_request) * dest_pos_max
                    + (1.0 - static_cast<double>(_set_pos_request)) * dest_pos_min;
            }
        }
        else
        {
            if (_current_pos + _seek_request < _start_pos + _media_input->initial_skip())
            {
                _seek_request = _start_pos + _media_input->initial_skip() - _current_pos;
            }
            if (_media_input->duration() > 0)
            {
                if (_seek_request > 0 && _current_pos + _seek_request >= std::max(_start_pos + _media_input->duration() - 2000000, static_cast<int64_t>(0)))
                {
                    _seek_request = std::max(_start_pos + _media_input->duration() - 2000000 - _current_pos, static_cast<int64_t>(0));
                }
            }
            *seek_to = _current_pos + _seek_request;
        }
        _seek_request = 0;
        _set_pos_request = -1.0f;
        _media_input->seek(*seek_to);

        _media_input->start_video_frame_read();
        _video_frame = _media_input->finish_video_frame_read();
        if (!_video_frame.is_valid())
        {
            msg::wrn("Seeked to end of video?!");
            stop_playback();
            return 0;
        }
        _video_pos = _video_frame.presentation_time;
        if (_audio_output)
        {
            _audio_output->stop();
            _media_input->start_audio_blob_read(_audio_output->required_initial_data_size());
            audio_blob blob = _media_input->finish_audio_blob_read();
            if (!blob.is_valid())
            {
                msg::wrn("Seeked to end of audio?!");
                stop_playback();
                return 0;
            }
            _audio_pos = blob.presentation_time;
            _audio_output->data(blob);
            _media_input->start_audio_blob_read(_audio_output->required_update_data_size());
            _master_time_start = _audio_output->start();
            _master_time_pos = _audio_pos;
            _current_pos = _audio_pos;
        }
        else
        {
            _master_time_start = timer::get_microseconds(timer::monotonic);
            _master_time_pos = _video_pos;
            _current_pos = _video_pos;
        }
        notify(notification::pos, normalize_pos(old_pos), normalize_pos(_current_pos));
        _need_frame_now = false;
        _need_frame_soon = true;
        _previous_frame_dropped = false;
        *more_steps = true;
        *prep_frame = true;
        return 0;
    }
    else if (_pause_request)
    {
        if (!_in_pause)
        {
            if (_audio_output)
            {
                _audio_output->pause();
            }
            else
            {
                _pause_start = timer::get_microseconds(timer::monotonic);
            }
            _in_pause = true;
            notify(notification::pause, false, true);
        }
        *more_steps = true;
        return 0;
    }
    else if (_need_frame_now)
    {
        _video_frame = _media_input->finish_video_frame_read();
        if (!_video_frame.is_valid())
        {
            if (_first_frame)
            {
                msg::dbg("Single-frame video input: going into pause mode.");
                _pause_request = true;
            }
            else
            {
                msg::dbg("End of video stream.");
                stop_playback();
                return 0;
            }
        }
        else
        {
            _first_frame = false;
        }
        _video_pos = _video_frame.presentation_time;
        if (!_audio_output)
        {
            _master_time_start += (_video_pos - _master_time_pos);
            _master_time_pos = _video_pos;
            _current_pos = _video_pos;
            notify(notification::pos, normalize_pos(_current_pos), normalize_pos(_current_pos));
        }
        _need_frame_now = false;
        _need_frame_soon = true;
        if (_drop_next_frame)
        {
            *drop_frame = true;
        }
        else if (!_pause_request)
        {
            *prep_frame = true;
        }
        *more_steps = true;
        return 0;
    }
    else if (_need_frame_soon)
    {
        _media_input->start_video_frame_read();
        _need_frame_soon = false;
        *more_steps = true;
        return 0;
    }
    else
    {
        if (_in_pause)
        {
            if (_audio_output)
            {
                _audio_output->unpause();
            }
            else
            {
                _master_time_start += timer::get_microseconds(timer::monotonic) - _pause_start;
            }
            _in_pause = false;
            notify(notification::pause, true, false);
        }

        if (_audio_output)
        {
            // Check if audio needs more data, and get audio time
            bool need_audio_data;
            _master_time_current = _audio_output->status(&need_audio_data) - _master_time_start + _master_time_pos;
            // Output requested audio data
            if (need_audio_data)
            {
                audio_blob blob = _media_input->finish_audio_blob_read();
                if (!blob.is_valid())
                {
                    msg::dbg("End of audio stream.");
                    stop_playback();
                    return 0;
                }
                _audio_pos = blob.presentation_time;
                _master_time_start += (_audio_pos - _master_time_pos);
                _master_time_pos = _audio_pos;
                _audio_output->data(blob);
                _media_input->start_audio_blob_read(_audio_output->required_update_data_size());
                _current_pos = _audio_pos;
                notify(notification::pos, normalize_pos(_current_pos), normalize_pos(_current_pos));
            }
        }
        else
        {
            // Use our own timer
            _master_time_current = timer::get_microseconds(timer::monotonic) - _master_time_start
                + _master_time_pos;
        }

        int64_t allowable_sleep = 0;
        if (_master_time_current >= _video_pos || _benchmark)
        {
            // Output current video frame
            _drop_next_frame = false;
            if (_master_time_current - _video_pos > _media_input->video_frame_duration() * 75 / 100 && !_benchmark)
            {
                msg::wrn("Video: delay %g seconds; dropping next frame.", (_master_time_current - _video_pos) / 1e6f);
                _drop_next_frame = true;
            }
            if (!_previous_frame_dropped)
            {
                *display_frame = true;
                if (_benchmark)
                {
                    _frames_shown++;
                    if (_frames_shown == 100)   //show fps each 100 frames
                    {
                        int64_t now = timer::get_microseconds(timer::monotonic);
                        msg::inf("FPS: %.2f", static_cast<float>(_frames_shown) / ((now - _fps_mark_time) / 1e6f));
                        _fps_mark_time = now;
                        _frames_shown = 0;
                    }
                }
            }
            _need_frame_now = true;
            _need_frame_soon = false;
            _previous_frame_dropped = _drop_next_frame;
        }
        else
        {
            allowable_sleep = _video_pos - _master_time_current;
            if (allowable_sleep < 100)
            {
                allowable_sleep = 0;
            }
            else if (allowable_sleep > 1100)
            {
                allowable_sleep = 1100;
            }
            allowable_sleep -= 100;
        }
        *more_steps = true;
        return allowable_sleep;
    }
}

bool player::run_step()
{
    bool more_steps;
    int64_t seek_to;
    bool prep_frame;
    bool drop_frame;
    bool display_frame;
    int64_t allowed_sleep;

    allowed_sleep = step(&more_steps, &seek_to, &prep_frame, &drop_frame, &display_frame);

    if (!more_steps)
    {
        return false;
    }
    if (prep_frame)
    {
        _video_output->prepare_next_frame(_video_frame);
    }
    else if (drop_frame)
    {
    }
    else if (display_frame)
    {
        _video_output->activate_next_frame();
    }

    if (_video_output->has_events())
    {
        _video_output->process_events();
    }
    else if (allowed_sleep > 0)
    {
        usleep(allowed_sleep);
    }

    return true;
}

void player::run()
{
    while (run_step());
}

void player::close()
{
    reset_playstate();
    if (_audio_output)
    {
        try { _audio_output->deinit(); } catch (...) {}
        delete _audio_output;
        _audio_output = NULL;
    }
    if (_video_output)
    {
        try { _video_output->deinit(); } catch (...) {}
        delete _video_output;
        _video_output = NULL;
    }
    if (_media_input)
    {
        try { _media_input->close(); } catch (...) {}
        delete _media_input;
        _media_input = NULL;
    }
}

void player::receive_cmd(const command &cmd)
{
    std::istringstream p(cmd.param);
    bool flag;
    float oldval;
    float param;

    bool parameters_changed = false;

    switch (cmd.type)
    {
    case command::toggle_play:
        _quit_request = true;
        /* notify when request is fulfilled */
        break;
    case command::toggle_stereo_mode_swap:
        _params.stereo_mode_swap = !_params.stereo_mode_swap;
        parameters_changed = true;
        notify(notification::stereo_mode_swap, !_params.stereo_mode_swap, _params.stereo_mode_swap);
        break;
    case command::cycle_video_stream:
        if (_media_input->video_streams() > 1
                && _media_input->video_frame_template().stereo_layout != video_frame::separate)
        {
            int oldstream = _media_input->selected_video_stream();
            int newstream = oldstream + 1;
            if (newstream >= _media_input->video_streams())
            {
                newstream = 0;
            }
            _media_input->select_video_stream(newstream);
            notify(notification::video_stream, oldstream, newstream);
            _seek_request = -1;         // Get position right
        }
        break;
    case command::set_video_stream:
        if (_media_input->video_streams() > 1
                && _media_input->video_frame_template().stereo_layout != video_frame::separate)
        {
            int oldstream = _media_input->selected_video_stream();
            int newstream;
            s11n::load(p, newstream);
            if (newstream < 0 || newstream >= _media_input->video_streams())
            {
                newstream = 0;
            }
            if (newstream != oldstream)
            {
                _media_input->select_video_stream(newstream);
                notify(notification::video_stream, oldstream, newstream);
                _seek_request = -1;     // Get position right
            }
        }
        break;
    case command::cycle_audio_stream:
        if (_media_input->audio_streams() > 1)
        {
            int oldstream = _media_input->selected_audio_stream();
            int newstream = oldstream + 1;
            if (newstream >= _media_input->audio_streams())
            {
                newstream = 0;
            }
            _media_input->select_audio_stream(newstream);
            notify(notification::audio_stream, oldstream, newstream);
            _seek_request = -1;         // Get position right
        }
        break;
    case command::set_audio_stream:
        if (_media_input->audio_streams() > 1)
        {
            int oldstream = _media_input->selected_audio_stream();
            int newstream;
            s11n::load(p, newstream);
            if (newstream < 0 || newstream >= _media_input->audio_streams())
            {
                newstream = 0;
            }
            if (newstream != oldstream)
            {
                _media_input->select_audio_stream(newstream);
                notify(notification::audio_stream, oldstream, newstream);
                _seek_request = -1;     // Get position right
            }
        }
        break;
    case command::cycle_subtitles_stream:
        if (_media_input->subtitles_streams() > 1)
        {
            int oldstream = _media_input->selected_subtitles_stream();
            int newstream = oldstream + 1;
            if (newstream >= _media_input->subtitles_streams())
            {
                newstream = 0;
            }
            _media_input->select_subtitles_stream(newstream);
            notify(notification::subtitles_stream, oldstream, newstream);
            _seek_request = -1;         // Get position right
        }
        break;
    case command::set_subtitles_stream:
        if (_media_input->subtitles_streams() > 1)
        {
            int oldstream = _media_input->selected_subtitles_stream();
            int newstream;
            s11n::load(p, newstream);
            if (newstream < 0 || newstream >= _media_input->subtitles_streams())
            {
                newstream = 0;
            }
            if (newstream != oldstream)
            {
                _media_input->select_subtitles_stream(newstream);
                notify(notification::subtitles_stream, oldstream, newstream);
                _seek_request = -1;     // Get position right
            }
        }
        break;
    case command::set_stereo_layout:
        {
            int stereo_layout;
            bool stereo_layout_swap;
            s11n::load(p, stereo_layout);
            s11n::load(p, stereo_layout_swap);
            _media_input->set_stereo_layout(static_cast<video_frame::stereo_layout_t>(stereo_layout), stereo_layout_swap);
            if (stereo_layout == video_frame::separate)
            {
                _seek_request = -1;     // Get position of both streams right
            }
        }
        break;
    case command::set_stereo_mode:
        {
            int stereo_mode;
            bool stereo_mode_swap;
            s11n::load(p, stereo_mode);
            s11n::load(p, stereo_mode_swap);
            _params.stereo_mode = static_cast<parameters::stereo_mode_t>(stereo_mode);
            _params.stereo_mode_swap = stereo_mode_swap;
            parameters_changed = true;
        }
        break;
    case command::toggle_fullscreen:
        flag = false;
        if (_video_output)
        {
            flag = _video_output->toggle_fullscreen();
        }
        notify(notification::fullscreen, flag, !flag);
        break;
    case command::center:
        if (_video_output)
        {
            _video_output->center();
        }
        notify(notification::center);
        break;
    case command::toggle_pause:
        _pause_request = !_pause_request;
        /* notify when request is fulfilled */
        break;
    case command::adjust_contrast:
        s11n::load(p, param);
        oldval = _params.contrast;
        _params.contrast = std::max(std::min(_params.contrast + param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::contrast, oldval, _params.contrast);
        break;
    case command::set_contrast:
        s11n::load(p, param);
        oldval = _params.contrast;
        _params.contrast = std::max(std::min(param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::contrast, oldval, _params.contrast);
        break;
    case command::adjust_brightness:
        s11n::load(p, param);
        oldval = _params.brightness;
        _params.brightness = std::max(std::min(_params.brightness + param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::brightness, oldval, _params.brightness);
        break;
    case command::set_brightness:
        s11n::load(p, param);
        oldval = _params.brightness;
        _params.brightness = std::max(std::min(param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::brightness, oldval, _params.brightness);
        break;
    case command::adjust_hue:
        s11n::load(p, param);
        oldval = _params.hue;
        _params.hue = std::max(std::min(_params.hue + param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::hue, oldval, _params.hue);
        break;
    case command::set_hue:
        s11n::load(p, param);
        oldval = _params.hue;
        _params.hue = std::max(std::min(param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::hue, oldval, _params.hue);
        break;
    case command::adjust_saturation:
        s11n::load(p, param);
        oldval = _params.saturation;
        _params.saturation = std::max(std::min(_params.saturation + param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::saturation, oldval, _params.saturation);
        break;
    case command::set_saturation:
        s11n::load(p, param);
        oldval = _params.saturation;
        _params.saturation = std::max(std::min(param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::saturation, oldval, _params.saturation);
        break;
    case command::seek:
        s11n::load(p, param);
        _seek_request = param * 1e6f;
        /* notify when request is fulfilled */
        break;
    case command::set_pos:
        s11n::load(p, param);
        _set_pos_request = param;
        /* notify when request is fulfilled */
        break;
    case command::adjust_parallax:
        s11n::load(p, param);
        oldval = _params.parallax;
        _params.parallax = std::max(std::min(_params.parallax + param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::parallax, oldval, _params.parallax);
        break;
    case command::set_parallax:
        s11n::load(p, param);
        oldval = _params.parallax;
        _params.parallax = std::max(std::min(param, 1.0f), -1.0f);
        parameters_changed = true;
        notify(notification::parallax, oldval, _params.parallax);
        break;
    case command::set_crosstalk:
        {
            float r, g, b;
            std::ostringstream oldval, newval;
            s11n::load(p, r);
            s11n::load(p, g);
            s11n::load(p, b);
            s11n::save(oldval, _params.crosstalk_r);
            s11n::save(oldval, _params.crosstalk_g);
            s11n::save(oldval, _params.crosstalk_b);
            _params.crosstalk_r = std::max(std::min(r, 1.0f), 0.0f);
            _params.crosstalk_g = std::max(std::min(g, 1.0f), 0.0f);
            _params.crosstalk_b = std::max(std::min(b, 1.0f), 0.0f);
            s11n::save(newval, _params.crosstalk_r);
            s11n::save(newval, _params.crosstalk_g);
            s11n::save(newval, _params.crosstalk_b);
            parameters_changed = true;
            notify(notification::crosstalk, oldval.str(), newval.str());
        }
        break;
    case command::adjust_ghostbust:
        s11n::load(p, param);
        oldval = _params.ghostbust;
        _params.ghostbust = std::max(std::min(_params.ghostbust + param, 1.0f), 0.0f);
        parameters_changed = true;
        notify(notification::ghostbust, oldval, _params.ghostbust);
        break;
    case command::set_ghostbust:
        s11n::load(p, param);
        oldval = _params.ghostbust;
        _params.ghostbust = std::max(std::min(param, 1.0f), 0.0f);
        parameters_changed = true;
        notify(notification::ghostbust, oldval, _params.ghostbust);
        break;
    case command::set_subtitles_font:
        {
            std::string old_file;
            old_file = _params.subtitles_font;
            _params.subtitles_font = cmd.param;
            parameters_changed = true;
            notify(notification::subtitles_font, old_file, _params.subtitles_font);
            break;
        }
    case command::set_subtitles_encoding:
        {
            std::string old_val = _params.subtitles_encoding;
            _params.subtitles_encoding = cmd.param;
            parameters_changed = true;
            notify(notification::subtitles_encoding, old_val, _params.subtitles_encoding);
            break;
        }
    case command::set_subtitles_color:
        {
            int val, old_val;
            s11n::load(p, val);
            old_val = _params.subtitles_color;
            _params.subtitles_color = val & 0x00FFFFFF;
            parameters_changed = true;
            notify(notification::subtitles_color, old_val, _params.subtitles_color);
            break;
        }
    }

    if (parameters_changed && _video_output)
    {
        _video_output->set_parameters(_params);
    }
}

void player::notify(const notification &note)
{
    // Notify all controllers
    for (size_t i = 0; i < global_controllers.size(); i++)
    {
        global_controllers[i]->receive_notification(note);
    }
}
