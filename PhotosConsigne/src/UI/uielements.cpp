

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
 * \file UIElements.cpp
 * \brief defines UIElements
 * \author Florian Lance
 * \date 04/04/2017
 */


// Qt
#include <QDesktopServices>
#include <QMessageBox>


// local
#include "UIElements.hpp"

using namespace pc;

pc::UIElements::UIElements(QMainWindow *parent) : m_parent(parent) {

    // use designer ui
    mainUI.setupUi(parent);

    // register new types
    qRegisterMetaType<std::shared_ptr<QString>>(        "std::shared_ptr<QString>");
    qRegisterMetaType<std::shared_ptr<QTextDocument>>(  "std::shared_ptr<QTextDocument>");

    // # preview display
    mainUI.hlPreview->addWidget(&previewW);

    // # photo display
    mainUI.hlSelectedPhoto->addWidget(&photoW);

    // # right settings
    mainUI.vlRightSettings->addWidget(&settingsW);

    // PCMainUI init uI
    // # definition
    PaperFormat pf(mainUI.cbDPI->currentText(),mainUI.cbFormat->currentText(), mainUI.cbOrientation->currentIndex() == 1);
    int currentDPI = mainUI.cbDPI->currentText().toInt();
    mainUI.laDefinition->setTextFormat(Qt::RichText);
    mainUI.laDefinition->setText("<p><b>Définition: </b> " + QString::number(pf.width_pixels(currentDPI)) + "x" + QString::number(pf.height_pixels(currentDPI)) + " pixels</p>");
    // # buttons
    mainUI.pbAdd->hide();
    // # list photos
    mainUI.twPhotosList->setTabEnabled(1,false); // list photos info tab
    // # list pages
    mainUI.twPagesList->setTabEnabled(1,false);  // list pages info tab
    // # tabs middles
    mainUI.twMiddle->setTabEnabled(0, false);

    mainUI.tabWDocument->setCurrentIndex(1);

    // mainUI connections
    connect(mainUI.pbDisplayOnlyPhotos, &QPushButton::clicked, this, [&]{
        Utility::safe_init_double_spinbox_value(settingsW.globalSetW.styleW.ui.dsbRatioPC, 1.);
        for(auto &&setW : settingsW.setsLoadedW){
            Utility::safe_init_double_spinbox_value(setW->styleW.ui.dsbRatioPC, 1.);
            Utility::safe_init_slider_value(setW->styleW.ui.hsRatioPC, 10000.);
        }
        ask_for_update(true);
    });
    connect(mainUI.pbDisplayOnlyTextes, &QPushButton::clicked, this, [&]{
        Utility::safe_init_double_spinbox_value(settingsW.globalSetW.styleW.ui.dsbRatioPC, 0.);
        for(auto &&setW : settingsW.setsLoadedW){
            Utility::safe_init_double_spinbox_value(setW->styleW.ui.dsbRatioPC, 0.);
            Utility::safe_init_slider_value(setW->styleW.ui.hsRatioPC, 0.);
        }
        ask_for_update(true);
    });

    settingsW.init_spinboxes_connections({mainUI.sbPreviewGridH,mainUI.sbPreviewGridV}, false);
    settingsW.init_checkboxes_connections({mainUI.cbDisplayPreviewGrid,mainUI.cbDoublePage}, false);
    // # misc
    connect(mainUI.cbSaveOnlyCurrentPage, &QCheckBox::clicked, this, [=]{
        ask_for_update(false);
    });

    connect(mainUI.cbOrientation, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]{
        ask_for_update(true);
    });

    // right tab
    connect(&settingsW, &RightSettingsW::settings_updated_signal,       this, &UIElements::ask_for_update);
    connect(&settingsW, &RightSettingsW::resource_added_signal,         this, &UIElements::resource_added_signal);
    connect(&settingsW, &RightSettingsW::set_progress_bar_state_signal, this, &UIElements::set_progress_bar_state_signal);
    connect(&settingsW, &RightSettingsW::set_progress_bar_text_signal,  this, &UIElements::set_progress_bar_text_signal);

    // main window
    // # progress
    connect(this, &UIElements::set_progress_bar_state_signal,   mainUI.progressBarLoading, &QProgressBar::setValue);
    connect(this, &UIElements::set_progress_bar_text_signal,    mainUI.laLoadingText, &QLabel::setText);
    // # buttons
    // ## donate
    connect(mainUI.pbDonate,    &QPushButton::clicked, this, &UIElements::display_donate_window);
    // ## help
    connect(mainUI.pbHelp,      &QPushButton::clicked, this, &UIElements::display_help_window);
}


void UIElements::ask_for_update(bool displayZones){

    if(displayZones){
        zonesTimer.start(1000);
    }
    emit settings_updated_signal();
}

void UIElements::write_to_xml(QXmlStreamWriter &xml) const{

    xml.writeStartElement("Document");
    xml.writeAttribute("orientation", QString::number(mainUI.cbOrientation->currentIndex()));
    xml.writeAttribute("format", QString::number(mainUI.cbFormat->currentIndex()));
    xml.writeAttribute("nbPages", QString::number(mainUI.sbNbPages->value()));
    xml.writeAttribute("doublePage", QString::number(mainUI.cbDoublePage->isChecked() ? 1 : 0));
    xml.writeAttribute("dpi", QString::number(mainUI.cbDPI->currentIndex()));
    xml.writeAttribute("onlyCurrPage", QString::number(mainUI.cbSaveOnlyCurrentPage->isChecked() ? 1 : 0));
    xml.writeAttribute("blackAndWhite", QString::number(mainUI.cbBAndW->isChecked() ? 1 : 0));
    settingsW.write_to_xml(xml);
    xml.writeEndElement();
}


void UIElements::load_from_xml(QXmlStreamReader &xml, bool firstPart){

    if(xml.name() == "Document"){

        if(firstPart){ // read only arguments
            Utility::safe_init_combo_box_index(mainUI.cbOrientation, xml.attributes().value("orientation").toInt());
            Utility::safe_init_combo_box_index(mainUI.cbFormat, xml.attributes().value("format").toInt());
            Utility::safe_init_combo_box_index(mainUI.cbDPI, xml.attributes().value("dpi").toInt());
            Utility::safe_init_spinbox_value(mainUI.sbNbPages, xml.attributes().value("nbPages").toInt());
            Utility::safe_init_checkboxe_checked_state(mainUI.cbDoublePage, xml.attributes().value("doublePage").toInt() == 1);
            Utility::safe_init_checkboxe_checked_state(mainUI.cbSaveOnlyCurrentPage, xml.attributes().value("onlyCurrPage").toInt() == 1);
            Utility::safe_init_checkboxe_checked_state(mainUI.cbBAndW, xml.attributes().value("blackAndWhite").toInt() == 1);
        }else{
            settingsW.load_from_xml(xml);
        }
    }
}

void UIElements::display_donate_window(){

    supportW = std::make_unique<QWidget>();
    supportW->setWindowIcon(QIcon(":/images/icon"));
    supportW->setWindowModality(Qt::ApplicationModal);
    supportW->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    Ui::SupportUI support;
    support.setupUi(supportW.get());

    delete support.laPatreon;
    delete support.pbPatreon;
    delete support.hlPatreon;
    connect(support.pbPaypal, &QPushButton::clicked, this, [=]{
        if(!QDesktopServices::openUrl(QUrl("https://www.paypal.me/PhotosConsigne", QUrl::TolerantMode))){
            QMessageBox::warning(m_parent, tr("Avertissement"), tr("Le site internet du tutoriel n'a pu être lancé, aucun logiciel de navigation web n'a été trouvé.' .\n"),QMessageBox::Ok);
        }
    });
    connect(support.pbMail, &QPushButton::clicked, this, [=]{
        if(!QDesktopServices::openUrl(QUrl("mailto:?to=lance.florian@protonmail.com&subject=PhotosConsigne&body=", QUrl::TolerantMode))){}
    });
    connect(support.pbReturn, &QPushButton::clicked, this, [=]{
        supportW->hide();
    });

    supportW->show();
}

void UIElements::display_help_window(){

    helpW = std::make_unique<QWidget>();
    helpW->setWindowIcon(QIcon(":/images/icon"));
    helpW->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    Ui::HelpUI help;
    help.setupUi(helpW.get());

    connect(help.pbFAQ, &QPushButton::clicked, this, [=]{
        if(!QDesktopServices::openUrl(QUrl("https://github.com/FlorianLance/PhotosConsigne/wiki/FAQ:-Foire-aux-questions", QUrl::TolerantMode))){
            QMessageBox::warning(m_parent, tr("Avertissement"), tr("Le site internet du tutoriel n'a pu être lancé, aucun logiciel de navigation web n'a été trouvé.' .\n"),QMessageBox::Ok);
        }
    });
    connect(help.pbVideosTutorials, &QPushButton::clicked, this, [=]{
        if(!QDesktopServices::openUrl(QUrl("https://github.com/FlorianLance/PhotosConsigne/wiki/Tutoriels", QUrl::TolerantMode))){
            QMessageBox::warning(m_parent, tr("Avertissement"), tr("Le site internet du tutoriel n'a pu être lancé, aucun logiciel de navigation web n'a été trouvé.' .\n"),QMessageBox::Ok);
        }
    });

    connect(help.pbReturn, &QPushButton::clicked, this, [=]{
        helpW->hide();
    });

    helpW->show();
}

void UIElements::update_global_settings(GlobalSettings &settings) const{

    // # document
    settings.document.paperFormat           = PaperFormat(mainUI.cbDPI->currentText(), mainUI.cbFormat->currentText(), mainUI.cbOrientation->currentIndex() == 1, mainUI.cbDoublePage->isChecked());
    settings.document.saveOnlyCurrentPage   = mainUI.cbSaveOnlyCurrentPage->isChecked();
    settings.document.displayPreviewGrid    = mainUI.cbDisplayPreviewGrid->isChecked();
    settings.document.nbHoriPreviewGridLine = mainUI.sbPreviewGridH->value();
    settings.document.nbVertPreviewGridLine = mainUI.sbPreviewGridV->value();
    qDebug() << "update_global_settings " << settings.document.displayPreviewGrid;

    // # set
    settingsW.globalSetW.update_settings(settings.sets);

    // # page
    settingsW.globalPageW.update_settings(settings.pages);
    settings.pages.nb = mainUI.sbNbPages->value();

    // # header
    settingsW.headerW.update_settings(settings.header);

    // # footer
    settingsW.footerW.update_settings(settings.footer);
}


void UIElements::set_ui_state_for_adding_photos(bool state){

    if(!state){
        mainUI.pbAddPhotosDIrectory->setIcon(QIcon(":/images/photos/cancel"));
        mainUI.pbAddPhotosDIrectory->setText("Stop");
    }else{
        mainUI.pbAddPhotosDIrectory->setIcon(QIcon(":/images/dirImages"));
        mainUI.pbAddPhotosDIrectory->setText(tr("Ajouter un dossier"));
    }

    // tabs
    // # middle
    mainUI.twMiddle->setEnabled(state);
    // # right
    settingsW.ui.tbRight->setEnabled(state);
    // # action
    // ## photos
    mainUI.pbInsertNewImage->setEnabled(state);
    mainUI.pbInsertNewWhiteSpace->setEnabled(state);
    mainUI.pbRemoveAllPhoto->setEnabled(state);
    // ## pdf
    mainUI.pbSavePDF->setEnabled(state);
    // # photos list
    mainUI.lwPhotosList->setEnabled(state);
    // # pages list
    mainUI.lwPagesList->setEnabled(state);
    // # document
    mainUI.tabWDocument->setEnabled(state);

    if( mainUI.lwPhotosList->count()> 0){
        mainUI.twMiddle->setTabEnabled(0, true);
    }
}

void UIElements::set_ui_state_for_generating_pdf(bool state){
    mainUI.tabActions->setEnabled(state);
}

void UIElements::set_ui_state_for_loading_work(bool state){
    mainUI.tabActions->setEnabled(state);

    // tabs
    // # middle
    mainUI.twMiddle->setEnabled(state);
    // # right
    settingsW.ui.tbRight->setEnabled(state);
    // # action
    // ## photos
    mainUI.pbInsertNewImage->setEnabled(state);
    mainUI.pbInsertNewWhiteSpace->setEnabled(state);
    mainUI.pbRemoveAllPhoto->setEnabled(state);
    // ## pdf
    mainUI.pbSavePDF->setEnabled(state);
    // # photos list
    mainUI.lwPhotosList->setEnabled(state);
    // # pages list
    mainUI.lwPagesList->setEnabled(state);
    // # document
    mainUI.tabWDocument->setEnabled(state);

    if( mainUI.lwPhotosList->count()> 0){
        mainUI.twMiddle->setTabEnabled(0, true);
    }
}

void UIElements::update_photos_list(const GlobalSettings &settings){

    mainUI.lwPhotosList->blockSignals(true);

    SPhotos photos = settings.photos.loaded;

    mainUI.lwPhotosList->clear();
    mainUI.twPhotosList->setTabText(1, QString::number(photos->size()));

    for(int ii = 0; ii < photos->size(); ++ii){

        QString ext = photos->at(ii)->isADuplicate ? " (copie)" : "";
        mainUI.lwPhotosList->addItem(QString::number(ii+1) + ". " + photos->at(ii)->namePhoto + ext + ((photos->at(ii)->pageId > -1) ? " (p" + QString::number(photos->at(ii)->pageId) + ")" : ""));

        bool individual = settingsW.setsLoadedW[ii]->ui.cbEnableIndividualConsign->isChecked();
        QBrush brush = mainUI.lwPhotosList->item(ii)->foreground();
        SPhoto photo = photos->at(ii);

        if(!photo->isOnDocument && !photo->isRemoved){
            brush.setColor(QRgb(qRgb(255,0,0)));
        }else if(!photo->isOnDocument){
            brush.setColor(QRgb(qRgb(255,120,120)));
        }else if(photo->isRemoved){
            brush.setColor(QRgb(qRgb(127,180,255)));
        }else if(individual){
            brush.setColor(QRgb(qRgb(0,0,255)));
        }else{
            brush.setColor(QRgb(qRgb(0,106,255)));
        }

        if(ii == settings.photos.currentId){
            if(photo->isRemoved){
                mainUI.pbRemove->hide();
                mainUI.pbAdd->show();
            }else{
                mainUI.pbRemove->show();
                mainUI.pbAdd->hide();
            }
        }

        mainUI.lwPhotosList->item(ii)->setForeground(brush);
    }

    mainUI.lwPhotosList->setCurrentRow(settings.photos.currentId);
    mainUI.lwPhotosList->blockSignals(false);
}

void UIElements::update_pages_list(const PCPages &pcPages){

    mainUI.lwPagesList->blockSignals(true);

    int currentIndex = mainUI.lwPagesList->currentRow();
    mainUI.lwPagesList->clear();
    mainUI.twPagesList->setTabText(1, QString::number(pcPages.pages.size()));

    for(int ii = 0; ii < pcPages.pages.size(); ++ii){

        mainUI.lwPagesList->addItem(QString::number(ii+1) + ". " + QString::number(pcPages.pages[ii]->sets.size()) + ((pcPages.pages[ii]->sets.size() > 1 ) ? " ensembles." : " ensemble.") );

        bool individual = settingsW.pagesW[ii]->ui.cbEnableIndividualPage->isChecked();
        QBrush brush =  mainUI.lwPagesList->item(ii)->foreground();
        if(pcPages.pages[ii]->sets.size() == 0){  // invalid
            brush.setColor(QRgb(qRgb(255,106,0)));
        }else if(!individual){                  // global
            brush.setColor(QRgb(qRgb(0,106,255)));
        }else{                                  // individual
            brush.setColor(QRgb(qRgb(0,0,255)));
        }

        mainUI.lwPagesList->item(ii)->setForeground(brush);
    }

    if(currentIndex >= pcPages.pages.size() && currentIndex != 0){
        currentIndex = pcPages.pages.size()-1;
    }
    mainUI.lwPagesList->setCurrentRow(currentIndex);
    mainUI.lwPagesList->blockSignals(false);
}

void UIElements::update_UI(const GlobalSettings &settings){

    // update defition/dim labels
    int currentDPI = mainUI.cbDPI->currentText().toInt();
    mainUI.laDefinition->setText("<p><b>Définition: </b> " + QString::number(settings.document.paperFormat .width_pixels(currentDPI))+ "x" +
                                    QString::number(settings.document.paperFormat .height_pixels(currentDPI)) +
                                    " pixels <br /><b>Dimensions: </b> " +
                                    QString::number(settings.document.paperFormat.sizeMM.width()) + "x" +
                                    QString::number(settings.document.paperFormat.sizeMM.height()) + " mm</p>");

    // update size margins mm
    settingsW.globalPageW.marginsW.update_mm_values(settings.document.paperFormat);
    for(auto &&pageW : settingsW.pagesW){
        pageW->marginsW.update_mm_values(settings.document.paperFormat);
    }

    if(settings.photos.loaded->size() > 0){
        SPhoto currentDisplayedPhoto = settings.photos.loaded->at(settings.photos.currentId);


        QStringList split = currentDisplayedPhoto->pathPhoto.split("/");
        QString path = "";
        for(int ii = 0; ii < split.size(); ++ii){
            path += split[ii] + "/";
            if(ii== split.size()/2 && currentDisplayedPhoto->pathPhoto.size() > 45){
                path += "<br />";
            }
        }

        QString textInfo = "<p><b>Chemin:</b> "  + path + "<br /><b>Taille photo:</b> " +
                QString::number(currentDisplayedPhoto->info.size()*0.000001, 'f', 2 ) + "Mo - <b>Dimensions:</b> " +
                QString::number(currentDisplayedPhoto->size().width())  + "x" +
                QString::number(currentDisplayedPhoto->size().height()) + "</p>";
        mainUI.laPhotoInfo->setTextFormat(Qt::RichText);
        mainUI.laPhotoInfo->setText(textInfo);
    }

    // header
    settingsW.headerW.ui.frameHeader->setEnabled(settingsW.headerW.ui.cbEnableHeader->isChecked());
    // footer
    settingsW.footerW.ui.frameFooter->setEnabled(settingsW.footerW.ui.cbEnableFooter->isChecked());


    // pages
    // # global
    settingsW.globalPageW.pageSetsW.customW.update_format(settings.document.paperFormat);    
    // # individual
    for(auto &&pageW : settingsW.pagesW){
        pageW->pageSetsW.customW.update_format(settings.document.paperFormat);
        pageW->ui.framePage->setEnabled(pageW->ui.cbEnableIndividualPage->isChecked());
    }

    // sets
    for(auto &&setW : settingsW.setsLoadedW){
        setW->ui.frameSet->setEnabled(setW->ui.cbEnableIndividualConsign->isChecked());
    }

    Utility::safe_init_spinbox_value(mainUI.sbOrder, settings.photos.currentId);

    if(settings.photos.loaded->size() > 0){
        mainUI.sbOrder->setMaximum(settings.photos.loaded->size()-1);
    }

    // display current dynamic ui
    if(settings.photos.loaded->size() > 0){
        display_current_individual_set_ui(settings);
        display_current_individual_page_ui(settings);
    }
}

void UIElements::add_resource_from_xml(QUrl url, QImage image){

    settingsW.headerW.richTextW.textEdit()->document()->addResource(QTextDocument::ImageResource, url, image);
    settingsW.footerW.richTextW.textEdit()->document()->addResource(QTextDocument::ImageResource, url, image);

    for(auto &&set : settingsW.setsLoadedW){
        set->textW.textEdit()->document()->addResource(QTextDocument::ImageResource, url, image);
    }
    settingsW.globalSetW.textW.textEdit()->document()->addResource(QTextDocument::ImageResource, url, image);
}

void UIElements::display_current_individual_set_ui(const GlobalSettings &settings){

    if(settingsW.setsValidedW.size() == 0){ // empty
        return;
    }

    if(settings.sets.currentId >= settingsW.setsValidedW.size()){ // id too big
        qWarning() << "-ERROR: display_current_individual_set_ui  " << settings.sets.currentId;
        return;
    }

    int currentVisiblesW = 0;
    for(int ii = 0; ii < settingsW.setsLoadedW.size(); ++ii){
        if(!settingsW.setsLoadedW[ii]->isHidden()){
            ++currentVisiblesW;
        }
    }

    if(currentVisiblesW > 1){
        for(auto &&w : settingsW.setsLoadedW){
            w->hide();
        }
    }

    if(currentVisiblesW == 1){

        int currentShownW = 0;
        int curretShownId = 0;
        for(int ii = 0; ii < settingsW.setsLoadedW.size(); ++ii){
            if(!settingsW.setsLoadedW[ii]->isHidden()){
                currentShownW = settingsW.setsLoadedW[ii]->id;
                curretShownId = ii;
                break;
            }
        }

        if(settingsW.setsValidedW[settings.sets.currentId]->id == currentShownW){
            return;
        }

        settingsW.setsLoadedW[curretShownId]->hide();
        settingsW.setsValidedW[settings.sets.currentId]->show();
    }else{
        settingsW.setsValidedW[settings.sets.currentId]->show();
    }

    settingsW.setsValidedW[settings.sets.currentId]->show();
    settingsW.ui.tbRight->setItemText(3, "ENSEMBLE N°" + QString::number(settings.sets.currentId+1));
}

void UIElements::display_current_individual_page_ui(const GlobalSettings &settings){

    if(settingsW.pagesW.size() == 0){ // empty
        return;
    }

    if(settings.pages.currentId >= settingsW.pagesW.size()){ // id too big
        qWarning() << "-ERROR: display_current_individual_page_ui  " << settings.pages.currentId;
        return;
    }

    mainUI.lwPagesList->blockSignals(true);
    mainUI.lwPagesList->setCurrentRow(settings.pages.currentId);
    mainUI.lwPagesList->blockSignals(false);

    for(int ii = 0; ii < settingsW.pagesW.size(); ++ii){
        if(ii == settings.pages.currentId){
            settingsW.pagesW[ii]->show();
        }else{
            settingsW.pagesW[ii]->hide();
        }
    }

    settingsW.pagesW[settings.pages.currentId]->show();
    settingsW.ui.vlSelectedPage->setAlignment(Qt::AlignmentFlag::AlignTop);

    settingsW.ui.tbRight->setItemText(1, "PAGE N°" + QString::number(settings.pages.currentId+1));
}


void UIElements::display_photo_panel(){
    Utility::safe_init_tab_widget_index(mainUI.twMiddle, 0);
}

void UIElements::display_preview_panel(){
    Utility::safe_init_tab_widget_index(mainUI.twMiddle, 1);
}




