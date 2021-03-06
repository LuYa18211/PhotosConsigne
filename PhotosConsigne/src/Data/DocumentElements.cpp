
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


/**
 * \file DocumentElements.cpp
 * \brief defines Consign/Header/Footer/PCSet/PCPage/PCPages
 * \author Florian Lance
 * \date 04/04/2017
 */

// local
#include "DocumentElements.hpp"

// Qt
#include <QDebug>

using namespace pc;

void pc::PCPage::compute_sizes(QRectF upperRect){

    rectOnPage = std::move(upperRect);

    MarginsSettings &margins        = settings.margins;
    MiscSettings &misc              = settings.misc;
    SetsPositionSettings &positions = settings.positions;

    // extern margins
    qreal heightTopMargin = 0., heightBottomMargin = 0., widthLeftMargin = 0., widthRightMargin = 0.;

    if(margins.exteriorMarginsEnabled){
        heightTopMargin     = rectOnPage.height() * margins.top;
        heightBottomMargin  = rectOnPage.height() * margins.bottom;
        widthLeftMargin     = rectOnPage.width()  * margins.left;
        widthRightMargin    = rectOnPage.width()  * margins.right;
    }

    // page minus extern margins
    pageMinusMarginsRect = QRectF(rectOnPage.x() + widthLeftMargin, rectOnPage.y() + heightTopMargin,
                                 rectOnPage.width() - widthLeftMargin - widthRightMargin, rectOnPage.height() - heightTopMargin - heightBottomMargin);

    qreal footerMarginHeight  = margins.footerHeaderMarginEnabled ? margins.footer * pageMinusMarginsRect.height() : 0.;
    qreal headerMarginHeight  = margins.footerHeaderMarginEnabled ? margins.header * pageMinusMarginsRect.height() : 0.;

    // header/footer ratios
    qreal headerFooterSetsHeight = pageMinusMarginsRect.height() - footerMarginHeight - headerMarginHeight;
    qreal footerRatio            = (footer->settings.enabled && !misc.doNotDisplayFooter) ? footer->settings.ratio : 0.;
    qreal headerRatio            = (header->settings.enabled && !misc.doNotDisplayHeader) ? header->settings.ratio : 0.;

    qreal sum = footerRatio + headerRatio;
    if(sum > 1.){        
        footerRatio -= (sum-1.) *0.5;
        headerRatio -= (sum-1.) *0.5;
    }

    // header/footer/sets heights
    qreal footerHeight = footerRatio * headerFooterSetsHeight;
    qreal headerHeight = headerRatio * headerFooterSetsHeight;
    qreal setsHeight   = headerFooterSetsHeight - footerHeight - headerHeight;

    // header/footer/sets/headerMargins/footerMargins rects
    QRectF headerRect = QRectF( pageMinusMarginsRect.x(),
                                pageMinusMarginsRect.y(),
                                pageMinusMarginsRect.width(),
                                headerHeight);

    marginHeaderRect  = QRectF(pageMinusMarginsRect.x(),
                               headerRect.y() + headerRect.height(),
                               pageMinusMarginsRect.width(),
                               headerMarginHeight);

    setsRect          = QRectF(pageMinusMarginsRect.x(),
                               marginHeaderRect.y() + marginHeaderRect.height(),
                               pageMinusMarginsRect.width(),
                               setsHeight);

    marginFooterRect  = QRectF(pageMinusMarginsRect.x(),
                               setsRect.y() + setsRect.height(),
                               pageMinusMarginsRect.width(),
                               footerMarginHeight);

    QRectF footerRect = QRectF(pageMinusMarginsRect.x(),
                               marginFooterRect.y() + marginFooterRect.height(),
                               pageMinusMarginsRect.width(),
                               footerHeight);

    header->compute_sizes(headerRect);
    footer->compute_sizes(footerRect);

    // intern margins
    int nbInterVMargins     = positions.nbPhotosV - 1;
    int nbInterHMargins     = positions.nbPhotosH - 1;
    qreal heightInterMargin = 0., widthInterMargin = 0.;
    if(margins.interiorMarginsEnabled){
        if(nbInterVMargins > 0){
            heightInterMargin = setsRect.height()/nbInterVMargins * margins.interHeight;
        }
        if(nbInterHMargins > 0){
            widthInterMargin  = setsRect.width()/nbInterHMargins  * margins.interWidth;
        }
    }

    // PC
    qreal widthSets   = (setsRect.width()  - nbInterHMargins * widthInterMargin);//  / positions.nbPhotosH;
    qreal heightSets  = (setsRect.height() - nbInterVMargins * heightInterMargin);// / positions.nbPhotosV;

    int currPC = 0;

    if(!positions.customMode){ // grid

        interMarginsRects.clear();
        interMarginsRects.reserve(sets.size());        
        qreal offsetV = setsRect.y();
        for(int ii = 0; ii < positions.nbPhotosV; ++ii){
            qreal offsetH = setsRect.x();
            qreal heightSet = heightSets*positions.linesHeight[ii];

            for(int jj = 0; jj < positions.nbPhotosH; ++jj){
                if(currPC >= sets.size())
                    break;

                qreal widthSet = widthSets*positions.columnsWidth[jj];
                sets[currPC]->compute_sizes(QRectF(offsetH, offsetV, widthSet, heightSet));
                interMarginsRects.push_back(QRectF(offsetH, offsetV,
                                                   (jj < positions.nbPhotosH-1) ? widthSet  + widthInterMargin  : widthSet,
                                                   (ii < positions.nbPhotosV-1) ? heightSet + heightInterMargin : heightSet));

                offsetH += widthSet + widthInterMargin;
                ++currPC;
            }

            offsetV += heightSet + heightInterMargin;
        }
    }else{ // custom mode

        interMarginsRects.clear();
        for(int ii = 0; ii < positions.relativePosCustom.size(); ++ii){
            if(currPC >= sets.size())
                break;

            QRectF &relRect = positions.relativePosCustom[ii];

            sets[currPC]->compute_sizes(QRectF(setsRect.x() + relRect.x()*setsRect.width(),
                                               setsRect.y() + relRect.y()*setsRect.height(),
                                               relRect.width()*setsRect.width(),
                                               relRect.height()*setsRect.height()));
            ++currPC;
        }
    }
}

void pc::PCSet::compute_sizes(QRectF upperRect){
    rectOnPage = std::move(upperRect);

    //  text
    qreal heightConsigneV = rectOnPage.height() * (1. - settings.style.ratioTextPhoto);
    qreal widthConsigneV  = rectOnPage.width();
    qreal heightConsigneH = rectOnPage.height();
    qreal widthConsigneH  = rectOnPage.width() * (1. - settings.style.ratioTextPhoto);
    //  photo
    qreal heightPhotoV    = settings.style.ratioTextPhoto * rectOnPage.height();
    qreal widthPhotoV     = rectOnPage.width();
    qreal heightPhotoH    = rectOnPage.height();
    qreal widthPhotoH     = settings.style.ratioTextPhoto * rectOnPage.width();

    QRectF consignRect, photoRect;
    switch (settings.style.textPositionFromPhotos) {
    case Position::top:
        consignRect = QRectF(rectOnPage.x(), rectOnPage.y(), widthConsigneV, heightConsigneV);
        photoRect   = QRectF(rectOnPage.x(), rectOnPage.y() + heightConsigneV, widthPhotoV, heightPhotoV);
        break;
    case Position::left:
        consignRect = QRectF(rectOnPage.x(), rectOnPage.y(), widthConsigneH, heightConsigneH);
        photoRect   = QRectF(rectOnPage.x() + widthConsigneH, rectOnPage.y(), widthPhotoH, heightPhotoH);
        break;
    case Position::right:
        consignRect = QRectF(rectOnPage.x() + widthPhotoH, rectOnPage.y(), widthConsigneH, heightConsigneH);
        photoRect   = QRectF(rectOnPage.x(), rectOnPage.y(), widthPhotoH, heightPhotoH);
        break;
    case Position::bottom:
        consignRect = QRectF(rectOnPage.x(), rectOnPage.y() + heightPhotoV, widthConsigneV, heightConsigneV);
        photoRect   = QRectF(rectOnPage.x(), rectOnPage.y(), widthPhotoV, heightPhotoV);
        break;
    case Position::on:
        photoRect = consignRect = QRectF(rectOnPage.x(), rectOnPage.y(), rectOnPage.width(), rectOnPage.height());
        break;
    }

    text->compute_sizes(consignRect);
    photo->compute_sizes(photoRect);
}


void Header::compute_sizes(QRectF upperRect) {
    rectOnPage = std::move(upperRect);
}
