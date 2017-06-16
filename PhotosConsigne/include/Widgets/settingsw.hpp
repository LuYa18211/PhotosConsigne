
#pragma once

// Qt
// # widgets
#include <QCheckBox>
#include <QComboBox>
#include <QToolButton>
#include <QSpinBox>
#include <QPushButton>
#include <QToolBox>
#include <QRadioButton>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
// # dialogs
#include <QColorDialog>
#include <QFileDialog>
// # misc
#include <QAction>

// local
// # data
#include "Settings.hpp"

namespace pc {

struct Utility{

    static int photo_alignment_from_comboBox(QComboBox *cb);
    static PhotoAdjust photo_adjust_from_comboBox(QComboBox *cb);
    static qreal borders_line_width_from_comboBox(QComboBox *cb);
    static Qt::PenJoinStyle borders_join_style_from_comboBox(QComboBox *cb);
    static void borders_line_style_from_comboBox(QComboBox *cb, QPen &pen);
    static Qt::BrushStyle pattern_style_comboBox(QComboBox *cb);
    static void associate_double_spinbox_with_slider(QDoubleSpinBox *sb, QSlider *slider);
    static void associate_double_spinbox_with_slider(QDoubleSpinBox *sb1, QSlider *slider1, QDoubleSpinBox *sb2, QSlider *slider2);
    static void checkbox_enable_UI(QCheckBox *cb, QVector<QWidget *> widgets, bool inverted = false);

    static void safe_init_push_button_enabled_state(QPushButton* button, bool state);
    static void safe_init_tool_button_enabled_state(QToolButton* button, bool state);
    static void safe_init_radio_button_state(QRadioButton *rb, bool state);
    static void safe_init_checkboxe_checked_state(QCheckBox *cb, bool state);
    static void safe_init_spinbox_value(QSpinBox *sb, int value);
    static void safe_init_double_spinbox_value(QDoubleSpinBox *dsb, qreal value);
    static void safe_init_slider_value(QSlider *slider, int value);
    static void safe_init_combo_box_index(QComboBox *cb, int index);    
    static void safe_init_tab_widget_index(QTabWidget *tw, int index);
    static void safe_init_tool_box_index(QToolBox *tb, int index);

};


struct SettingsW : public QWidget {

    Q_OBJECT

public :

    SettingsW() : QWidget(nullptr){}

    // connections
    void init_checkboxes_connections(QVector<QCheckBox*> checkBoxes, bool displayZones = false);
    void init_comboboxes_connections(QVector<QComboBox*> comboBox, bool displayZones);
    void init_color_dialog_connections(QToolButton *tb, QString actionToolTip, QString dialogColorText, QColor *color, QSize iconSize, bool alpha = true);
    void init_image_dialog_connections(QToolButton *tb, QString actionToolTip, QString dialogText, SPhoto &photo, QSize iconeSIze);
    void init_pushbuttons_connections(QVector<QPushButton*> buttons, bool displayZones = false);
    void init_sliders_connections(QVector<QSlider*> sliders, bool displayZones = false);
    void init_spinboxes_connections(QVector<QSpinBox*> spinBoxes, bool displayZones = false);
    void init_doublespinboxes_connections(QVector<QDoubleSpinBox*> dSpinBoxes, bool displayZones);
    void init_radiobuttons_connections(QVector<QRadioButton*> buttons, bool displayZones);

signals:

    void settings_updated_signal(bool displayZones);
    void resource_added_signal(QUrl url, QImage image);
    void set_progress_bar_state_signal(int state);
    void set_progress_bar_text_signal(QString text);
};}
