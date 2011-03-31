/*
 * This file is part of bino, a 3D video player.
 *
 * Copyright (C) 2010-2011
 * Martin Lambers <marlam@marlam.de>
 * Stefan Eilemann <eile@eyescale.ch>
 * Frédéric Devernay <Frederic.Devernay@inrialpes.fr>
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

#include <cstring>

#include "dbg.h"
#include "msg.h"
#include "opt.h"

#include "player.h"
#include "player_qt.h"
#if HAVE_LIBEQUALIZER
# include "player_equalizer.h"
#endif
#include "qt_app.h"
#include "lib_versions.h"


int main(int argc, char *argv[])
{
    /* Initialization */
    char *program_name = strrchr(argv[0], '/');
    program_name = program_name ? program_name + 1 : argv[0];
    msg::set_level(msg::INF);
    msg::set_program_name(program_name);
    msg::set_columns_from_env();
    dbg::init_crashhandler();

    /* Command line handling */
    std::vector<opt::option *> options;
    opt::info help("help", '\0', opt::optional);
    options.push_back(&help);
    opt::info version("version", '\0', opt::optional);
    options.push_back(&version);
    opt::flag no_gui("no-gui", 'n', opt::optional);
    options.push_back(&no_gui);
    std::vector<std::string> log_levels;
    log_levels.push_back("debug");
    log_levels.push_back("info");
    log_levels.push_back("warning");
    log_levels.push_back("error");
    log_levels.push_back("quiet");
    opt::val<std::string> log_level("log-level", 'l', opt::optional, log_levels, "");
    options.push_back(&log_level);
    std::vector<std::string> input_modes;
    input_modes.push_back("mono");
    input_modes.push_back("separate-left-right");
    input_modes.push_back("separate-right-left");
    input_modes.push_back("top-bottom");
    input_modes.push_back("top-bottom-half");
    input_modes.push_back("bottom-top");
    input_modes.push_back("bottom-top-half");
    input_modes.push_back("left-right");
    input_modes.push_back("left-right-half");
    input_modes.push_back("right-left");
    input_modes.push_back("right-left-half");
    input_modes.push_back("even-odd-rows");
    input_modes.push_back("odd-even-rows");
    opt::val<std::string> input_mode("input", 'i', opt::optional, input_modes, "");
    options.push_back(&input_mode);
    opt::val<int> video("video", 'v', opt::optional, 1, 999, 1);
    options.push_back(&video);
    opt::val<int> audio("audio", 'a', opt::optional, 1, 999, 1);
    options.push_back(&audio);
    opt::val<int> subtitle("subtitle", 's', opt::optional, 1, 999, 0);
    options.push_back(&subtitle);
    std::vector<std::string> video_output_modes;
    video_output_modes.push_back("mono-left");
    video_output_modes.push_back("mono-right");
    video_output_modes.push_back("top-bottom");
    video_output_modes.push_back("top-bottom-half");
    video_output_modes.push_back("left-right");
    video_output_modes.push_back("left-right-half");
    video_output_modes.push_back("even-odd-rows");
    video_output_modes.push_back("even-odd-columns");
    video_output_modes.push_back("checkerboard");
    video_output_modes.push_back("red-cyan-monochrome");
    video_output_modes.push_back("red-cyan-half-color");
    video_output_modes.push_back("red-cyan-full-color");
    video_output_modes.push_back("red-cyan-dubois");
    video_output_modes.push_back("green-magenta-monochrome");
    video_output_modes.push_back("green-magenta-half-color");
    video_output_modes.push_back("green-magenta-full-color");
    video_output_modes.push_back("green-magenta-dubois");
    video_output_modes.push_back("amber-blue-monochrome");
    video_output_modes.push_back("amber-blue-half-color");
    video_output_modes.push_back("amber-blue-full-color");
    video_output_modes.push_back("amber-blue-dubois");
    video_output_modes.push_back("red-green-monochrome");
    video_output_modes.push_back("red-blue-monochrome");
    video_output_modes.push_back("stereo");
    video_output_modes.push_back("equalizer");
    video_output_modes.push_back("equalizer-3d");
    opt::val<std::string> video_output_mode("output", 'o', opt::optional, video_output_modes, "");
    options.push_back(&video_output_mode);
    opt::flag fullscreen("fullscreen", 'f', opt::optional);
    options.push_back(&fullscreen);
    opt::flag center("center", 'c', opt::optional);
    options.push_back(&center);
    opt::flag swap_eyes("swap-eyes", 'S', opt::optional);
    options.push_back(&swap_eyes);
    opt::flag benchmark("benchmark", 'b', opt::optional);
    options.push_back(&benchmark);
    opt::val<float> parallax("parallax", 'P', opt::optional, -1.0f, +1.0f, parameters().parallax);
    options.push_back(&parallax);
    opt::tuple<float> crosstalk("crosstalk", 'C', opt::optional, 0.0f, 1.0f, std::vector<float>(3, parameters().crosstalk_r), 3);
    options.push_back(&crosstalk);
    opt::val<float> ghostbust("ghostbust", 'G', opt::optional, 0.0f, 1.0f, parameters().ghostbust);
    options.push_back(&ghostbust);
    // Accept some Equalizer options. These are passed to Equalizer for interpretation.
    opt::val<std::string> eq_server("eq-server", '\0', opt::optional);
    options.push_back(&eq_server);
    opt::val<std::string> eq_config("eq-config", '\0', opt::optional);
    options.push_back(&eq_config);
    opt::val<std::string> eq_listen("eq-listen", '\0', opt::optional);
    options.push_back(&eq_listen);
    opt::val<std::string> eq_logfile("eq-logfile", '\0', opt::optional);
    options.push_back(&eq_logfile);
    opt::val<std::string> eq_render_client("eq-render-client", '\0', opt::optional);
    options.push_back(&eq_render_client);

    std::vector<std::string> arguments;
#ifdef __APPLE__
    // strip -psn* option when launching from the Finder on mac
    if (argc > 1 && strncmp("-psn", argv[1], 4) == 0)
    {
        argc--;
        argv++;
    }
#endif
    if (!opt::parse(argc, argv, options, 0, -1, arguments))
    {
        return 1;
    }
    set_qt_argv(argc, argv);
    if (version.value())
    {
        msg::req("%s version %s", PACKAGE_NAME, VERSION);
        msg::req("    Copyright (C) 2011 the Bino developers.");
        msg::req("    This is free software. You may redistribute copies of it");
        msg::req("    under the terms of the GNU General Public License.");
        msg::req("    There is NO WARRANTY, to the extent permitted by law.");
        msg::req("Platform:");
        msg::req("    %s", PLATFORM);
        msg::req("Libraries used:");
        std::vector<std::string> v = lib_versions(false);
        for (size_t i = 0; i < v.size(); i++)
        {
            msg::req("    %s", v[i].c_str());
        }
    }
    if (help.value())
    {
        msg::req_txt(
                "Usage:\n"
                "  %s [option...] [file0] [file1] [file2]\n"
                "\n"
                "Options:\n"
                "  --help                   Print help.\n"
                "  --version                Print version.\n"
                "  -n|--no-gui              Do not use the GUI, just show a plain window.\n"
                "  -l|--log-level=LEVEL     Set log level (debug/info/warning/error/quiet).\n"
                "  -v|--video=STREAM        Select video stream (1-n, depending on input).\n"
                "  -a|--audio=STREAM        Select audio stream (1-n, depending on input).\n"
                "  -s|--subtitle=STREAM     Select subtitle stream (1-n, depending on input).\n"
                "  -i|--input=TYPE          Select input type (default autodetect):\n"
                "    mono                     Single view.\n"
                "    separate-left-right      Left/right separate streams, left first.\n"
                "    separate-right-left      Left/right separate streams, right first.\n"
                "    top-bottom               Left top, right bottom.\n"
                "    top-bottom-half          Left top, right bottom, half height.\n"
                "    bottom-top               Left bottom, right top.\n"
                "    bottom-top-half          Left bottom, right top, half height.\n"
                "    left-right               Left left, right right.\n"
                "    left-right-half          Left left, right right, half width.\n"
                "    right-left               Left right, right left.\n"
                "    right-left-half          Left right, right left, half width.\n"
                "    even-odd-rows            Left even rows, right odd rows.\n"
                "    odd-even-rows            Left odd rows, right even rows.\n"
                "  -o|--output=TYPE         Select output type:\n"
                "    mono-left                Only left.\n"
                "    mono-right               Only right.\n"
                "    top-bottom               Left top, right bottom.\n"
                "    top-bottom-half          Left top, right bottom, half height.\n"
                "    left-right               Left left, right right.\n"
                "    left-right-half          Left left, right right, half width.\n"
                "    even-odd-rows            Left even rows, right odd rows.\n"
                "    even-odd-columns         Left even columns, right odd columns.\n"
                "    checkerboard             Left and right in checkerboard pattern.\n"
                "    red-cyan-monochrome      Red/cyan anaglyph, monochrome method.\n"
                "    red-cyan-half-color      Red/cyan anaglyph, half color method.\n"
                "    red-cyan-full-color      Red/cyan anaglyph, full color method.\n"
                "    red-cyan-dubois          Red/cyan anaglyph, Dubois method (default).\n"
                "    green-magenta-monochrome Green/magenta anaglyph, monochrome method.\n"
                "    green-magenta-half-color Green/magenta anaglyph, half color method.\n"
                "    green-magenta-full-color Green/magenta anaglyph, full color method.\n"
                "    green-magenta-dubois     Green/magenta anaglyph, Dubois method.\n"
                "    amber-blue-monochrome    Amber/blue anaglyph, monochrome method.\n"
                "    amber-blue-half-color    Amber/blue anaglyph, half color method.\n"
                "    amber-blue-full-color    Amber/blue anaglyph, full color method.\n"
                "    amber-blue-dubois        Amber/blue anaglyph, Dubois method.\n"
                "    red-green-monochrome     Red/green anaglyph, monochrome method.\n"
                "    red-blue-monochrome      Red/blue anaglyph, monochrome method.\n"
                "    stereo                   OpenGL quad-buffered stereo.\n"
                "    equalizer                Multi-display via Equalizer (2D setup).\n"
                "    equalizer-3d             Multi-display via Equalizer (3D setup).\n"
                "  -S|--swap-eyes           Swap left/right view.\n"
                "  -f|--fullscreen          Fullscreen.\n"
                "  -c|--center              Center window on screen.\n"
                "  -P|--parallax=VAL        Parallax adjustment (-1 to +1).\n"
                "  -C|--crosstalk=VAL       Crosstalk leak level (0 to 1); comma-separated\n"
                "                           values for the R,G,B channels.\n"
                "  -G|--ghostbust=VAL       Amount of ghostbusting to apply (0 to 1).\n"
                "  -b|--benchmark           Benchmark mode (no audio, show fps).\n"
                "\n"
                "Interactive control:\n"
                "  q or ESC                 Quit.\n"
                "  p or SPACE               Pause / unpause.\n"
                "  f                        Toggle fullscreen.\n"
                "  c                        Center window.\n"
                "  s                        Swap left/right view.\n"
                "  v                        Cycle through available video streams.\n"
                "  a                        Cycle through available audio streams.\n"
                "  1, 2                     Adjust contrast.\n"
                "  3, 4                     Adjust brightness.\n"
                "  5, 6                     Adjust hue.\n"
                "  7, 8                     Adjust saturation.\n"
                "  <, >                     Adjust parallax.\n"
                "  (, )                     Adjust ghostbusting.\n"
                "  left, right              Seek 10 seconds backward / forward.\n"
                "  up, down                 Seek 1 minute backward / forward.\n"
                "  page up, page down       Seek 10 minutes backward / forward.\n"
                "  Mouse click              Seek according to horizontal click position.",
                program_name);
    }
    if (version.value() || help.value())
    {
        return 0;
    }

#if HAVE_LIBEQUALIZER
    if (arguments.size() > 0 && arguments[0] == "--eq-client")
    {
        try
        {
            player_equalizer *player = new player_equalizer(&argc, argv, true);
            delete player;
        }
        catch (std::exception &e)
        {
            msg::err("%s", e.what());
            return 1;
        }
        return 0;
    }
#endif

    bool equalizer = false;
    bool equalizer_flat_screen = true;
    player_init_data init_data;
    init_data.log_level = msg::level();
    if (log_level.value() == "")
    {
        init_data.log_level = msg::INF;
    }
    else if (log_level.value() == "debug")
    {
        init_data.log_level = msg::DBG;
    }
    else if (log_level.value() == "info")
    {
        init_data.log_level = msg::INF;
    }
    else if (log_level.value() == "warning")
    {
        init_data.log_level = msg::WRN;
    }
    else if (log_level.value() == "error")
    {
        init_data.log_level = msg::ERR;
    }
    else if (log_level.value() == "quiet")
    {
        init_data.log_level = msg::REQ;
    }
    init_data.urls = arguments;
    init_data.video_stream = video.value() - 1;
    init_data.audio_stream = audio.value() - 1;
    init_data.subtitle_stream = subtitle.value() - 1;
    if (input_mode.value() == "")
    {
        init_data.stereo_layout_override = false;
    }
    else
    {
        init_data.stereo_layout_override = true;
        video_frame::stereo_layout_from_string(input_mode.value(), init_data.stereo_layout, init_data.stereo_layout_swap);
    }
    if (video_output_mode.value() == "")
    {
        init_data.stereo_mode_override = false;
    }
    else if (video_output_mode.value() == "equalizer")
    {
        equalizer = true;
        equalizer_flat_screen = true;
        init_data.stereo_mode_override = true;
        init_data.stereo_mode = parameters::mono_left;
        init_data.stereo_mode_swap = false;
    }
    else if (video_output_mode.value() == "equalizer-3d")
    {
        equalizer = true;
        equalizer_flat_screen = false;
        init_data.stereo_mode_override = true;
        init_data.stereo_mode = parameters::mono_left;
        init_data.stereo_mode_swap = false;
    }
    else
    {
        init_data.stereo_mode_override = true;
        parameters::stereo_mode_from_string(video_output_mode.value(), init_data.stereo_mode, init_data.stereo_mode_swap);
        init_data.stereo_mode_swap = swap_eyes.value();
    }
    init_data.fullscreen = fullscreen.value();
    init_data.center = center.value();
    init_data.benchmark = benchmark.value();
    if (init_data.benchmark)
    {
        msg::inf("Benchmark mode: audio and time synchronization disabled.");
    }
    init_data.params.parallax = parallax.value();
    init_data.params.crosstalk_r = crosstalk.value()[0];
    init_data.params.crosstalk_g = crosstalk.value()[1];
    init_data.params.crosstalk_b = crosstalk.value()[2];
    init_data.params.ghostbust = ghostbust.value();
    init_data.params.stereo_mode_swap = swap_eyes.value();

    int retval = 0;
    player *player = NULL;
    try
    {
        if (equalizer)
        {
#if HAVE_LIBEQUALIZER
            player = new class player_equalizer(&argc, argv, equalizer_flat_screen);
#else
            throw exc("This version of Bino was compiled without support for Equalizer.");
#endif
        }
        else if (!no_gui.value())
        {
            if (log_level.value() == "")
            {
                init_data.log_level = msg::WRN;         // Be silent by default when the GUI is used
            }
            init_data.fullscreen = false;               // GUI overrides fullscreen setting
            init_data.center = false;                   // GUI overrides center flag
            player = new class player_qt();
        }
        else
        {
            if (arguments.size() == 0)
            {
                throw exc("No video to play.");
            }
            player = new class player();
        }
        player->open(init_data);
        player->run();
    }
    catch (std::exception &e)
    {
        msg::err("%s", e.what());
        retval = 1;
    }
    if (player)
    {
        try { player->close(); } catch (...) {}
        delete player;
    }

    return retval;
}
