
/*******************************************************************************
** PhotosConsigne                                                             **
** MIT License                                                                **
** Copyright (c) [2016] [Florian Lance]                                       **
**                                                                            **
** Permission is hereby granted, free of charge, to any person obtaining a    **
** copy of this software and associated documentation files (the "Software"), **
** to deal in the Software without restriction, including without limitation  **
** the rights to use, copy, modify, merge, publish, distribute, sublicense,   **
** and/or sell copies of the Software, and to permit persons to whom the      **
** Software is furnished to do so, subject to the following conditions:       **
**                                                                            **
** The above copyright notice and this permission notice shall be included in **
** all copies or substantial portions of the Software.                        **
**                                                                            **
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR **
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   **
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    **
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER **
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    **
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        **
** DEALINGS IN THE SOFTWARE.                                                  **
**                                                                            **
********************************************************************************/


#pragma once


/**
 * \file DegradedW.hpp
 * \brief defines DegradedW/Ui::DegradedUI
 * \author Florian Lance
 * \date 04/04/2017
 */


// local
#include "SettingsW.hpp"
#include "PhotoW.hpp"

// generated
#include "ui_Degraded.h"

namespace Ui {
    class DegradedUI;
}


namespace pc {


class DegradedW : public SettingsW{


    Q_OBJECT

public:

    DegradedW(){
        ui.setupUi(this);
        ui.vlDrawZone->insertWidget(0, &m_degradedDraw);
        m_degradedDraw.setMinimumHeight(zoneLength);

        QLabel *laDegraded = new QLabel("Degradé");
        laDegraded->setStyleSheet(
        "QWidget {"
            "color : rgb(0,106,255);"
             "font:  9pt Calibri;"
         "}"
        "QWidget:!enabled {"
                "color : rgb(150,150,150);"
             "font:  9pt Calibri;"
         "}");


        laDegraded->setAlignment(Qt::AlignmentFlag::AlignHCenter);
        ui.vlDrawZone->insertWidget(1, laDegraded);

        color1.setAlpha(180);
        color2.setAlpha(180);
        init_radiobuttons_connections({ui.rbColor1, ui.rbColor2, ui.rbDegraded}, false);
        init_color_dialog_connections(ui.tbColor1, tr("Couleur 1"), tr("Choix de la couleur n°1"), &color1, QSize(50,50));
        init_color_dialog_connections(ui.tbColor2, tr("Couleur 2"), tr("Choix de la couleur n°2"), &color2, QSize(50,50));

        connect(&m_degradedDraw, &PhotoW::click_inside_signal, this, [&](QPointF posRel){

            if(isFirstPointTurn){
                start = posRel*zoneLength;
            }else{
                end = posRel*zoneLength;
            }
            isFirstPointTurn = !isFirstPointTurn;
            emit settings_updated_signal(false);

        });

        init_comboboxes_connections({ui.cbDegradedType}, false);

        connect(this, &DegradedW::settings_updated_signal, this, [&]{
            draw_color_zone();
        });

        draw_color_zone();
    }

    static void init_ui(DegradedW &p1, const DegradedW &p2){

        p1.color1 = p2.color1;
        p1.color2 = p2.color2;
        p1.isFirstPointTurn = p2.isFirstPointTurn;
        p1.start = p2.start;
        p1.end = p2.end;
        p1.ui.tbColor1->setIcon(p2.ui.tbColor1->icon());
        p1.ui.tbColor2->setIcon(p2.ui.tbColor2->icon());
        p1.m_currentColor = p2.m_currentColor;
        p1.m_degradedDraw.set_image(p1.m_currentColor);
        Utility::safe_init_combo_box_index(p1.ui.cbDegradedType, p2.ui.cbDegradedType->currentIndex());
        Utility::safe_init_radio_button_state(p1.ui.rbColor1, p2.ui.rbColor1->isChecked());
        Utility::safe_init_radio_button_state(p1.ui.rbColor2, p2.ui.rbColor2->isChecked());
        Utility::safe_init_radio_button_state(p1.ui.rbDegraded, p2.ui.rbDegraded->isChecked());
        p1.draw_color_zone();
    }

    void write_to_xml(QXmlStreamWriter &xml) const{
        xml.writeStartElement("Degraded");
        xml.writeAttribute("color1", StrUtility::to_str(color1));
        xml.writeAttribute("color2", StrUtility::to_str(color2));
        xml.writeAttribute("start", StrUtility::to_str(start));
        xml.writeAttribute("end", StrUtility::to_str(end));
        xml.writeAttribute("type", QString::number(ui.cbDegradedType->currentIndex()));
        xml.writeAttribute("choice", QString::number((ui.rbColor1->isChecked() ? 0 : (ui.rbColor2->isChecked() ? 1 : 2))));
        xml.writeEndElement();
    }

    void load_from_xml(QXmlStreamReader &xml){

        QStringList col = xml.attributes().value("color1").toString().split(" ");
        color1 = QColor(col[0].toInt(),col[1].toInt(),col[2].toInt(), col[3].toInt());

        col = xml.attributes().value("color2").toString().split(" ");
        color2 = QColor(col[0].toInt(),col[1].toInt(),col[2].toInt(), col[3].toInt());
        QPixmap pix(50,50);
        pix.fill(color1);
        ui.tbColor1->actions()[0]->setIcon(pix);

        pix.fill(color2);
        ui.tbColor2->actions()[0]->setIcon(pix);

        QStringList pt =xml.attributes().value("start").toString().split(" ");
        start = QPointF(pt[0].toDouble(),pt[1].toDouble());
        pt =xml.attributes().value("end").toString().split(" ");
        end = QPointF(pt[0].toDouble(),pt[1].toDouble());

        Utility::safe_init_combo_box_index(ui.cbDegradedType, xml.attributes().value("type").toInt());
        Utility::safe_init_radio_button_state(ui.rbColor1, xml.attributes().value("choice").toInt() == 0);
        Utility::safe_init_radio_button_state(ui.rbColor2, xml.attributes().value("choice").toInt() == 1);
        Utility::safe_init_radio_button_state(ui.rbDegraded, xml.attributes().value("choice").toInt() == 2);
    }

    void update_settings(Settings &settings) const{

        ColorsSettings &colors = dynamic_cast<ColorsSettings&>(settings);
        if(ui.rbColor1->isChecked()){
            colors.type = ColorType::color1;
        }else if(ui.rbColor2->isChecked()){
            colors.type = ColorType::color2;
        }else{
            colors.type = ColorType::degraded;
        }

        colors.start = start/zoneLength;
        colors.end   = end/zoneLength;
        colors.degradedType = static_cast<DegradedType>(ui.cbDegradedType->currentIndex());
        colors.color1 = color1;
        colors.color2 = color2;
    }

    void draw_color_zone(){

        QPainter painter(&m_currentColor);        
        painter.fillRect(m_currentColor.rect(),QBrush(Qt::white));

        switch(static_cast<DegradedType>(ui.cbDegradedType->currentIndex())){
            case DegradedType::padSpread:{
                QLinearGradient grad(start, end);
                grad.setSpread(QGradient::PadSpread);
                grad.setColorAt(0, color1);
                grad.setColorAt(1, color2);
                painter.fillRect(m_currentColor.rect(),grad);
            }
            break;
//            case DegradedType::reflectSpread:{
//                QLinearGradient grad(start, end);
//                grad.setSpread(QGradient::ReflectSpread);
//                grad.setColorAt(0, color1);
//                grad.setColorAt(1, color2);
//                painter.fillRect(m_currentColor.rect(),grad);
//            }
//            break;
//            case DegradedType::repeatSpread:{
//                QLinearGradient grad(start, end);
//                grad.setSpread(QGradient::RepeatSpread);
//                grad.setColorAt(0, color1);
//                grad.setColorAt(1, color2);
//                painter.fillRect(m_currentColor.rect(),grad);
//                }
//                break;
            case DegradedType::radPadSpread:{
                QRadialGradient grad(start, static_cast<qreal>(QVector2D(start-end).length()));
                grad.setSpread(QGradient::PadSpread);
                grad.setColorAt(0, color1);
                grad.setColorAt(1, color2);
                painter.fillRect(m_currentColor.rect(),grad);
                }
                break;
//            case DegradedType::radReflectSpread:{
//                QRadialGradient grad(start, static_cast<qreal>(QVector2D(start-end).length()));
//                grad.setSpread(QGradient::ReflectSpread);
//                grad.setColorAt(0, color1);
//                grad.setColorAt(1, color2);
//                painter.fillRect(m_currentColor.rect(),grad);
//                }
//                break;
//            case DegradedType::radRepeatSpread:{
//                    QRadialGradient grad(start, static_cast<qreal>(QVector2D(start-end).length()));
//                    grad.setSpread(QGradient::RepeatSpread);
//                    grad.setColorAt(0, color1);
//                    grad.setColorAt(1, color2);
//                    painter.fillRect(m_currentColor.rect(),grad);
//                }
//                break;
            case DegradedType::conical:{
                    QConicalGradient grad(start, static_cast<qreal>(QVector2D(start-end).length()));
                    grad.setColorAt(0, color1);
                    grad.setColorAt(1, color2);
                    painter.fillRect(m_currentColor.rect(),grad);
                }
                break;
        }

        painter.setPen(Qt::black);
        painter.setBrush(QBrush(Qt::green));
        painter.drawEllipse(QRectF(start.x()-4.,start.y()-4, 8., 8.));
        painter.setBrush(QBrush(Qt::red));
        painter.drawEllipse(QRectF(end.x()-4.,end.y()-4, 8., 8.));
        m_degradedDraw.set_image(m_currentColor);
        m_degradedDraw.update();
    }


public :

private :

    const int zoneLength = 120;

    PhotoW m_degradedDraw;

    Ui::DegradedUI ui;

    bool isFirstPointTurn = true;
    QPointF start = QPointF(20.,60.);
    QPointF end= QPointF(100.,60.);

    QImage m_currentColor = QImage(zoneLength, zoneLength, QImage::Format_ARGB32);

    QColor color1 = qRgba(255,255,255,100);
    QColor color2 = qRgba(0,0,0,100);
};
}
