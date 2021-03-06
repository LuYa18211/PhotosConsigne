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
 * \file RightSettingsW.hpp
 * \brief defines pc::RightSettingsW/Ui::RightSettingsUI
 * \author Florian Lance
 * \date 17/05/2017
 */

// local
#include "PageW.hpp"
#include "SetW.hpp"
#include "HeaderW.hpp"
#include "FooterW.hpp"

// generated ui
#include "ui_RightSettings.h"

namespace Ui {
    class RightSettingsUI;
}

namespace pc{


    struct RightSettingsW : public SettingsW {

        RightSettingsW() : SettingsW(){

            // init ui
            ui.setupUi(this);

            // header
            ui.vlHeader->addWidget(&headerW);
            connect(&headerW, &HeaderW::settings_updated_signal, this, &RightSettingsW::settings_updated_signal);            
            connect(headerW.richTextW.textEdit(), &TextEdit::resource_added_signal, this, &RightSettingsW::resource_added_signal);

            // footer
            ui.vlFooter->addWidget(&footerW);
            connect(&footerW, &FooterW::settings_updated_signal, this, &RightSettingsW::settings_updated_signal);
            connect(footerW.richTextW.textEdit(), &TextEdit::resource_added_signal, this, &RightSettingsW::resource_added_signal);

            // all pages
            ui.vlAllPages->addWidget(&globalPageW);
            ui.vlAllPages->setAlignment(Qt::AlignmentFlag::AlignTop);
            delete globalPageW.ui.frameIndividualSettings;
            globalPageW.global = true;

            globalPageW.ui.framePage->setEnabled(true);
            connect(&globalPageW, &PageW::settings_updated_signal, this, &RightSettingsW::settings_updated_signal);

            connect(&globalPageW.pageSetsW, &PageSetsW::reset_pos_signal, this, [&]{
                for(auto &&page : pagesW){
                    page->pageSetsW.read_pos_files();
                }
            });

            // misc all pages
            connect(&globalPageW.miscW, &MiscPageW::all_global_pages_signal, this, [&]{

                for(auto &&page : pagesW){
                    page->set_enabled(false);
                }
                emit settings_updated_signal(true);
            });

            connect(&globalPageW.miscW, &MiscPageW::all_individual_pages_signal, this, [&]{

                for(auto &&page : pagesW){
                    page->set_enabled(true);
                }
                emit settings_updated_signal(true);
            });

            connect(&globalPageW.miscW, &MiscPageW::replace_individual_with_global_signal, this, [&]{

                for(auto &&page : pagesW){
                    PageW::init_ui(*page, globalPageW);
                }

                emit settings_updated_signal(true);
            });
            delete globalPageW.miscW.ui.pbReplaceGlobalWithLocal;

            // misc all sets
            connect(globalSetW.miscW.ui.pbGlobal, &QPushButton::clicked, this, [&]{
                for(auto &&set : setsValidedW){
                    Utility::safe_init_checkboxe_checked_state(set->ui.cbEnableIndividualConsign, false);
                    set->ui.frameSet->setEnabled(false);
                }
                emit settings_updated_signal(true);
            });
            connect(globalSetW.miscW.ui.pbPerPage, &QPushButton::clicked, this, [&]{
                for(auto &&set : setsValidedW){
                    Utility::safe_init_checkboxe_checked_state(set->ui.cbEnableIndividualConsign, true);
                    set->ui.frameSet->setEnabled(true);
                }
                emit settings_updated_signal(true);
            });
            connect(globalSetW.miscW.ui.pbReplaceLocalWIthGlobal, &QPushButton::clicked, this, [&]{

                for(auto &&set : setsLoadedW){
                    SetW::init_ui(*set, globalSetW, true);
                }

                emit settings_updated_signal(true);
            });
            delete globalSetW.miscW.ui.pbReplaceGlobalWithLocal;


            // all sets
            ui.vlAllSets->addWidget(&globalSetW);
            delete globalSetW.ui.frameIndividualSettings;
            globalSetW.ui.frameSet->setEnabled(true);

            // # text
            globalSetW.global = true;
            globalSetW.textW.init_style(RichTextType::globalConsign);
            connect(globalSetW.textW.textEdit(), &TextEdit::resource_added_signal, this, &RightSettingsW::resource_added_signal);
            connect(&globalSetW, &SetW::settings_updated_signal, this, &RightSettingsW::settings_updated_signal);           

            ui.tbRight->setCurrentIndex(0);
        }

        ~RightSettingsW(){

            setsLoadedW.clear();
            setsValidedW.clear();
            pagesW.clear();
        }


        void update_individual_pages(int nbPages){

            int diff = pagesW.size() - nbPages;
            if(diff < 0){ // add -diff pages

                for(int ii = 0; ii < -diff;++ ii){

                    pagesW.push_back(std::make_shared<PageW>());

                    PageW *pageW = pagesW.last().get();
                    delete pageW->miscW.ui.frameApply;
                    delete pageW->miscW.ui.line2;
                    delete pageW->miscW.ui.pbReplaceLocalWIthGlobal;

                    ui.vlSelectedPage->addWidget(pageW);
                    pageW->hide();

                    PageW::init_ui(*pageW, globalPageW);
                    pageW->pageSetsW.ui.leNamePage->setText("Page n°" + QString::number(pagesW.size()));

                    connect(pageW, &PageW::settings_updated_signal, this, &RightSettingsW::settings_updated_signal);

                    connect(&pageW->miscW, &MiscPageW::replace_global_with_individual_signal, this, [=]{
                        PageW::init_ui(globalPageW, *pageW);
                        emit settings_updated_signal(true);
                    });

                    connect(&pageW->pageSetsW, &PageSetsW::reset_pos_signal, this, [=]{

                        for(auto &&page : pagesW){
                            if(pageW->id != page->id){
                                page->pageSetsW.read_pos_files();
                            }
                        }
                    });
                }

            }else if(diff > 0){ // remove diff pages

                for(int ii = 0; ii < -diff;++ ii){
                    pagesW.pop_back();
                }
            }
        }

        void insert_individual_set(int index){

            setsLoadedW.insert(index,std::make_shared<SetW>());
            SetW *setW = setsLoadedW[index].get();
            delete setW->miscW.ui.pbReplaceLocalWIthGlobal;
            delete setW->miscW.ui.line1;
            delete setW->miscW.ui.frameApply;

            ui.vlSelectedSet->addWidget(setW);
            setW->hide();

            connect(setW->textW.textEdit(), &TextEdit::resource_added_signal, this, &RightSettingsW::resource_added_signal);

            //  set style
            connect(setW, &SetW::settings_updated_signal, this, &RightSettingsW::settings_updated_signal);
            connect(&setW->miscW, &MiscSetW::replace_global_with_individual_signal, this, [=]{
                SetW::init_ui(globalSetW, *setW, true);
                emit settings_updated_signal(true);
            });

            // init new individual ui with global ui
            SetW::init_ui(*setW, globalSetW, false);
        }

        void remove_individual_set(int index){
            setsLoadedW.removeAt(index);
        }

        void reset_individual_sets(int nbPhotos){

            qreal currentState  = 750.;
            qreal offset        = 250. / nbPhotos;

            // clean
            setsLoadedW.clear();

            // insert new consigns
            for(int ii = 0; ii < nbPhotos; ++ii){

                emit set_progress_bar_text_signal("Consigne individuelle n°" + QString::number(ii));
                insert_individual_set(ii);
                currentState += offset;
                emit set_progress_bar_state_signal(static_cast<int>(currentState));
            }
        }

        void display_global_page_panel(){
            Utility::safe_init_tool_box_index(ui.tbRight, 0);
        }
        void display_current_page_panel(){
            Utility::safe_init_tool_box_index(ui.tbRight, 1);
        }

        void display_global_set_panel(){
            Utility::safe_init_tool_box_index(ui.tbRight, 2);
        }

        void display_current_set_panel(){
            Utility::safe_init_tool_box_index(ui.tbRight, 3);
        }

        void display_header_panel(){
            Utility::safe_init_tool_box_index(ui.tbRight, 4);
        }

        void display_footer_panel(){
            Utility::safe_init_tool_box_index(ui.tbRight, 5);
        }

        void write_to_xml(QXmlStreamWriter &xml) const{

            headerW.write_to_xml(xml);
            footerW.write_to_xml(xml);

            xml.writeStartElement("Global");
            globalPageW.write_to_xml(xml);
            globalSetW.write_to_xml(xml);
            xml.writeEndElement();

            xml.writeStartElement("Individual");
            xml.writeAttribute("nb_pages", QString::number(pagesW.size()));
            xml.writeAttribute("nb_sets", QString::number(pagesW.size()));
            for(const auto &pageW : pagesW){
                pageW->write_to_xml(xml);
            }
            for(const auto &set : setsLoadedW){
                set->write_to_xml(xml);
            }

            xml.writeEndElement();
        }


        void load_from_xml(QXmlStreamReader &xml){

            QXmlStreamReader::TokenType token = QXmlStreamReader::TokenType::StartElement;
            bool global = false;
            bool individual = false;
            int currentPageId = 0;
            int currentSetId  = 0;
            while(!xml.hasError()) {

                if(token == QXmlStreamReader::TokenType::EndElement && xml.name() == "Document"){                    
                    break;

                }else if(token == QXmlStreamReader::TokenType::StartElement){

                    if(xml.name() == "Header"){
                        headerW.load_from_xml(xml);
                    }else if(xml.name() == "Footer"){
                        footerW.load_from_xml(xml);
                    }else if(xml.name() == "Global"){
                        global = true;
                    }
                    else if(xml.name() == "Individual"){
                        individual = true;
                        global = false;
                    }
                    else if(xml.name() == "Page"){
                        if(global){
                            globalPageW.load_from_xml(xml);
                        }else if(individual){
                            if(pagesW.size() > currentPageId){
                                pagesW[currentPageId++]->load_from_xml(xml);
                            }
                        }
                    }else if(xml.name() == "Set"){
                        if(global){
                            globalSetW.load_from_xml(xml);
                        }else if(individual){
                            setsLoadedW[currentSetId++]->load_from_xml(xml);
                        }
                    }
                }

                token = xml.readNext();
            }
        }


        // ui
        Ui::RightSettingsUI ui;

        // widgets
        HeaderW         headerW;
        FooterW         footerW;

        SetW            globalSetW;
        PageW           globalPageW;

        QList<SPageW>   pagesW;
        QList<SSetW>    setsLoadedW;
        QList<SSetW>    setsValidedW;
    };
}
