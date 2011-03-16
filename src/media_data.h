/*
 * This file is part of bino, a 3D video player.
 *
 * Copyright (C) 2010-2011
 * Martin Lambers <marlam@marlam.de>
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

#ifndef MEDIA_DATA_H
#define MEDIA_DATA_H

#include <string>
#include <stdint.h>

#include "s11n.h"


class subtitles_list
{
public:
   // Sample format
   typedef enum
   {
      text,
      ssa,              // subtitles stored as vector of strings
      image,            // subtitles stored as stream of images
   } format_t;
   
   struct subtitle
   {
      std::string text;            // subtitle text
      int64_t start_time;          // Presentation timestamp
      int64_t end_time;
      
      const void * image_data;           // Buffer holding image data
      int image_width;             // Width of subtitle image
      int image_height;            // Height of subtitle image      
      
      subtitle(const char * t = ""):text(t),start_time(0),end_time(0),image_data(NULL),image_height(0),image_width(0){};
   };
   
   format_t format;      // Subtitles format
   std::string lang;     // Subtitles language
   
   typedef std::vector<subtitle> subtitles;
   subtitles  data;
   size_t current;
   
   // Return a string describing the format
   std::string format_info() const;    // Human readable information
   std::string format_name() const;    // Short code
   
   // Constructor
   subtitles_list();
   
   // Returns pointer to actual subtitle or NULL if none should be diplayed
   subtitle * get_current_subtitle(int64_t time);
   static const char* extract_text_from_ssa(const char* text);
   size_t find_next_subtitle(int64_t time);
};

class video_frame
{
public:
    // Data layout
    typedef enum
    {
        bgra32,         // Single plane: BGRABGRABGRA....
        yuv444p,        // Three planes, Y/U/V, all with the same size
        yuv422p,        // Three planes, U and V with half width: one U/V pair for 2x1 Y values
        yuv420p,        // Three planes, U and V with half width and half height: one U/V pair for 2x2 Y values
    } layout_t;

    // Color space
    typedef enum
    {
        srgb,           // SRGB color space
        yuv601,         // YUV according to ITU.BT-601
        yuv709,         // YUV according to ITU.BT-709
    } color_space_t;

    // Value range
    typedef enum
    {
        u8_full,        // 0-255 for all components
        u8_mpeg         // 16-235 for Y, 16-240 for U and V
    } value_range_t;

    // Location of chroma samples (only relevant for chroma subsampling layouts)
    typedef enum
    {
        center,         // U/V at center of the corresponding Y locations
        left,           // U/V vertically at the center, horizontally at the left Y locations
        topleft         // U/V at the corresponding top left Y location
    } chroma_location_t;

    // Stereo layout: describes how left and right view are stored
    typedef enum
    {
        mono,           // 1 video source: center view
        separate,       // 2 video sources: left and right view independent
        top_bottom,     // 1 video source: left view top, right view bottom, both with full size
        top_bottom_half,// 1 video source: left view top, right view bottom, both with half size
        left_right,     // 1 video source: left view left, right view right, both with full size
        left_right_half,// 1 video source: left view left, right view right, both with half size
        even_odd_rows   // 1 video source: left view even lines, right view odd lines
    } stereo_layout_t;

    int raw_width;                      // Width of the data in pixels
    int raw_height;                     // Height of the data in pixels
    float raw_aspect_ratio;             // Aspect ratio of the data
    int width;                          // Width of one view in pixels
    int height;                         // Height of one view in pixels
    float aspect_ratio;                 // Aspect ratio of one view when displayed
    layout_t layout;                    // Data layout
    color_space_t color_space;          // Color space
    value_range_t value_range;          // Value range
    chroma_location_t chroma_location;  // Chroma sample location
    stereo_layout_t stereo_layout;      // Stereo layout
    bool stereo_layout_swap;            // Whether the stereo layout needs to swap left and right view
    // The data. Note that a frame does not own the data stored in these pointers,
    // so it does not free them on destruction.
    void *data[2][3];                   // Data pointer for 1-3 planes in 1-2 views. NULL if unused.
    size_t line_size[2][3];             // Line size for 1-3 planes in 1-2 views. 0 if unused.
    subtitles_list::subtitle *subtitle;     // Associated subtitles with frame

    int64_t presentation_time;          // Presentation timestamp

    // Constructor
    video_frame();

    // Set width/height/ar from raw width/height/ar according to stereo layout
    void set_view_dimensions();

    // Does this frame contain valid data?
    bool is_valid() const
    {
        return (raw_width > 0 && raw_height > 0);
    }

    // Copy the data of the given view (0=left, 1=right) and the given plane (see layout)
    // to the given destination.
    void copy_plane(int view, int plane, void *dst) const;

    // Return a string describing the format (layout, color space, value range, chroma location)
    std::string format_info() const;    // Human readable information
    std::string format_name() const;    // Short code

    // Convert the stereo layout to and from a string representation
    static std::string stereo_layout_to_string(stereo_layout_t stereo_layout, bool stereo_layout_swap);
    static void stereo_layout_from_string(const std::string &s, stereo_layout_t &stereo_layout, bool &stereo_layout_swap);
};

class audio_blob
{
public:
    // Sample format
    typedef enum
    {
        u8,             // uint8_t
        s16,            // int16_t
        f32,            // float
        d64             // double
    } sample_format_t;

    int channels;                       // 1 (mono), 2 (stereo), 4 (quad), 6 (5:1), 7 (6:1), or 8 (7:1)
    int rate;                           // Samples per second
    sample_format_t sample_format;      // Sample format
    // The data. Note that an audio blob does not own the data stored in these pointers,
    // so it does not free them on destruction.
    void *data;                         // Pointer to the data
    size_t size;                        // Data size in bytes

    int64_t presentation_time;          // Presentation timestamp

    // Constructor
    audio_blob();

    // Does this blob contain valid data?
    bool is_valid() const
    {
        return (channels > 0 && rate > 0);
    }

    // Return a string describing the format
    std::string format_info() const;    // Human readable information
    std::string format_name() const;    // Short code

    // Return the number of bits the sample format
    int sample_bits() const;
};

class parameters : public s11n
{
public:
    // Do not change these enum values without a good reason, because the GUI saves them to a file.
    typedef enum
    {
        stereo,                         // OpenGL quad buffered stereo
        mono_left,                      // Left view only
        mono_right,                     // Right view only
        top_bottom,                     // Left view top, right view bottom
        top_bottom_half,                // Left view top, right view bottom, half height
        left_right,                     // Left view left, right view right
        left_right_half,                // Left view left, right view right, half width
        even_odd_rows,                  // Left view even rows, right view odd rows
        even_odd_columns,               // Left view even columns, right view odd columns
        checkerboard,                   // Checkerboard pattern
        red_cyan_monochrome,            // Red/cyan anaglyph, monochrome method
        red_cyan_half_color,            // Red/cyan anaglyph, half color method
        red_cyan_full_color,            // Red/cyan anaglyph, full color method
        red_cyan_dubois,                // Red/cyan anaglyph, high quality Dubois method
        green_magenta_monochrome,       // Green/magenta anaglyph, monochrome method
        green_magenta_half_color,       // Green/magenta anaglyph, half color method
        green_magenta_full_color,       // Green/magenta anaglyph, full color method
        green_magenta_dubois,           // Green/magenta anaglyph, high quality Dubois method
        amber_blue_monochrome,          // Amber/blue anaglyph, monochrome method
        amber_blue_half_color,          // Amber/blue anaglyph, half color method
        amber_blue_full_color,          // Amber/blue anaglyph, full color method
        amber_blue_dubois,              // Amber/blue anaglyph, high quality Dubois method
        red_green_monochrome,           // Red/green anaglyph, monochrome method
        red_blue_monochrome,            // Red/blue anaglyph, monochrome method
    } stereo_mode_t;

    stereo_mode_t stereo_mode;          // Stereo mode
    bool stereo_mode_swap;              // Swap left and right view
    float parallax;                     // Parallax adjustment, -1 .. +1
    float crosstalk_r;                  // Crosstalk level for red, 0 .. 1
    float crosstalk_g;                  // Crosstalk level for green, 0 .. 1
    float crosstalk_b;                  // Crosstalk level for blue, 0 .. 1
    float ghostbust;                    // Amount of crosstalk ghostbusting, 0 .. 1
    float contrast;                     // Contrast adjustment, -1 .. +1
    float brightness;                   // Brightness adjustment, -1 .. +1
    float hue;                          // Hue adjustment, -1 .. +1
    float saturation;                   // Saturation adjustment, -1 .. +1
    
    int subtitles_color;                // RGB color of subtitles
    std::string subtitles_font;         // Filepath of font for rendering
    int subtitles_size;            // Size of subtitles

    // Constructor
    parameters();

    // Set all uninitialised values to their defaults
    void set_defaults();
    
    // Convert the stereo mode to and from a string representation
    static std::string stereo_mode_to_string(stereo_mode_t stereo_mode, bool stereo_mode_swap);
    static void stereo_mode_from_string(const std::string &s, stereo_mode_t &stereo_mode, bool &stereo_mode_swap);

    // Serialization
    void save(std::ostream &os) const;
    void load(std::istream &is);
};

#endif
