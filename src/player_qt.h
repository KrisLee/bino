/*
 * This file is part of bino, a 3D video player.
 *
 * Copyright (C) 2010-2011
 * Martin Lambers <marlam@marlam.de>
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

#ifndef PLAYER_GUI_H
#define PLAYER_GUI_H

#include <QMainWindow>
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QSettings>
#include <QDialog>
#include <QLineEdit>
#include <QFontDialog>

#include "controller.h"
#include "video_output_qt.h"
#include "player.h"
#include <QColorDialog>


class player_qt_internal : public player, public controller
{
private:
    bool _benchmark;
    bool _playing;
    video_container_widget *_container_widget;
    video_output_qt *_video_output;

protected:
    virtual video_output *create_video_output();

public:
    player_qt_internal(bool benchmark, video_container_widget *widget);
    virtual ~player_qt_internal();

    virtual void receive_cmd(const command &cmd);

    virtual void receive_notification(const notification &note);

    const video_output_qt *get_video_output() const;
    bool playloop_step();
    void force_stop();
    void move_event();
};

class in_out_widget : public QWidget, public controller
{
    Q_OBJECT

private:
    QSettings *_settings;
    const player_qt_internal *_player;
    QComboBox *_video_combobox;
    QComboBox *_audio_combobox;
    QComboBox *_input_combobox;
    QComboBox *_output_combobox;
    QCheckBox *_swap_checkbox;
    bool _lock;

    void set_stereo_layout(video_frame::stereo_layout_t stereo_layout, bool stereo_layout_swap);
    void set_stereo_mode(parameters::stereo_mode_t stereo_mode, bool stereo_mode_swap);

private slots:
    void video_changed();
    void audio_changed();
    void input_changed();
    void output_changed();
    void swap_changed();

public:
    in_out_widget(QSettings *settings, const player_qt_internal *player, QWidget *parent);
    virtual ~in_out_widget();

    void update(const player_init_data &init_data, bool have_valid_input, bool playing);

    int get_video_stream();
    int get_audio_stream();
    void get_stereo_layout(video_frame::stereo_layout_t &stereo_layout, bool &stereo_layout_swap);
    void get_stereo_mode(parameters::stereo_mode_t &stereo_mode, bool &stereo_mode_swap);

    virtual void receive_notification(const notification &note);
};

class controls_widget : public QWidget, public controller
{
    Q_OBJECT

private:
    bool _lock;
    QSettings *_settings;
    QPushButton *_play_button;
    QPushButton *_pause_button;
    QPushButton *_stop_button;
    QPushButton *_fullscreen_button;
    QPushButton *_center_button;
    QPushButton *_bbb_button;
    QPushButton *_bb_button;
    QPushButton *_b_button;
    QPushButton *_f_button;
    QPushButton *_ff_button;
    QPushButton *_fff_button;
    QSlider *_seek_slider;
    bool _playing;

private slots:
    void play_pressed();
    void pause_pressed();
    void stop_pressed();
    void fullscreen_pressed();
    void center_pressed();
    void bbb_pressed();
    void bb_pressed();
    void b_pressed();
    void f_pressed();
    void ff_pressed();
    void fff_pressed();
    void seek_slider_changed();

public:
    controls_widget(QSettings *settings, QWidget *parent);
    virtual ~controls_widget();

    void update(const player_init_data &init_data, bool have_valid_input, bool playing);
    virtual void receive_notification(const notification &note);
};

class color_dialog : public QDialog, public controller
{
    Q_OBJECT

private:
    bool _lock;
    QDoubleSpinBox *_c_spinbox;
    QSlider *_c_slider;
    QDoubleSpinBox *_b_spinbox;
    QSlider *_b_slider;
    QDoubleSpinBox *_h_spinbox;
    QSlider *_h_slider;
    QDoubleSpinBox *_s_spinbox;
    QSlider *_s_slider;

private slots:
    void c_slider_changed(int val);
    void c_spinbox_changed(double val);
    void b_slider_changed(int val);
    void b_spinbox_changed(double val);
    void h_slider_changed(int val);
    void h_spinbox_changed(double val);
    void s_slider_changed(int val);
    void s_spinbox_changed(double val);

public:
    color_dialog(const parameters &params, QWidget *parent);

    virtual void receive_notification(const notification &note);
};

class crosstalk_dialog : public QDialog, public controller
{
    Q_OBJECT

private:
    bool _lock;
    QDoubleSpinBox *_r_spinbox;
    QDoubleSpinBox *_g_spinbox;
    QDoubleSpinBox *_b_spinbox;
    parameters *_params;

private slots:
    void spinbox_changed();

public:
    crosstalk_dialog(parameters *params, QWidget *parent);

    virtual void receive_notification(const notification &note);
};

class stereoscopic_dialog : public QDialog, public controller
{
    Q_OBJECT

private:
    bool _lock;
    
    QDoubleSpinBox *_p_spinbox;
    QSlider *_p_slider;
    QDoubleSpinBox *_g_spinbox;
    QSlider *_g_slider;

private slots:
    void p_slider_changed(int val);
    void p_spinbox_changed(double val);
    void g_slider_changed(int val);
    void g_spinbox_changed(double val);

public:
    stereoscopic_dialog(const parameters &params, QWidget *parent);

    virtual void receive_notification(const notification &note);
};

class subtitles_dialog : public QDialog, public controller
{
   Q_OBJECT
   
private:
   bool _lock;
   QLineEdit *_font_label;
   QPushButton * _font_button;
   QColorDialog * _color_dialog;
   QSpinBox * _font_size_spinbox;
   QLabel * _color_box;
   QPushButton * _color_button;
   
private slots:
   void font_button_pushed();
   void color_button_pushed();
   void font_size_changed(int value);
   
private:
   void set_font_color(int rgb);
   
public:
   subtitles_dialog(const parameters &params, QWidget *parent);
   
   virtual void receive_notification(const notification &note);
};


class main_window : public QMainWindow, public controller
{
    Q_OBJECT

private:
    QSettings *_settings;
    video_container_widget *_video_container_widget;
    in_out_widget *_in_out_widget;
    controls_widget *_controls_widget;
    color_dialog *_color_dialog;
    crosstalk_dialog *_crosstalk_dialog;
    stereoscopic_dialog *_stereoscopic_dialog;
    subtitles_dialog *_subtitles_dialog;
    player_qt_internal *_player;
    QTimer *_timer;
    player_init_data _init_data;
    const player_init_data _init_data_template;
    bool _stop_request;

    QString current_file_hash();
    bool open_player();
    void open(QStringList urls);

private slots:
    void move_event();
    void playloop_step();
    void file_open();
    void file_open_url();
    void preferences_colors();
    void preferences_crosstalk();
    void preferences_stereoscopic();
    void preferences_subtitles();
    void help_manual();
    void help_website();
    void help_keyboard();
    void help_about();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void moveEvent(QMoveEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

public:
    main_window(QSettings *settings, const player_init_data &init_data);
    virtual ~main_window();

    virtual void receive_notification(const notification &note);
};

class player_qt : public player
{
private:
    bool _qt_app_owner;
    main_window *_main_window;
    QSettings *_settings;

public:
    player_qt();
    virtual ~player_qt();

    QSettings *settings()
    {
        return _settings;
    }

    virtual void open(const player_init_data &init_data);
    virtual void run();
    virtual void close();
};

#endif
