
#include "PageSetsW.hpp"



using namespace pc;

bool PageSetsW::posLoaded = false;
QVector<SetsPositionSettings> PageSetsW::predefPositions;

PageSetsW::PageSetsW() : SettingsW(){

    // init ui
    ui.setupUi(this);

    connect(ui.sbPhotosNbWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]{

        int nbMaxPhotos = ui.sbPhotosNbWidth->value() * ui.sbPhotosNbHeight->value();
        if(nbMaxPhotos < ui.sbPhotosNbPerPage->value()){
            Utility::safe_init_spinbox_value(ui.sbPhotosNbPerPage, nbMaxPhotos);
        }

        update_grid_sizes_ui();

        emit settings_updated_signal(true);
    });

    connect(ui.sbPhotosNbHeight, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]{

        int nbMaxPhotos = ui.sbPhotosNbWidth->value() * ui.sbPhotosNbHeight->value();
        if(nbMaxPhotos < ui.sbPhotosNbPerPage->value()){
            Utility::safe_init_spinbox_value(ui.sbPhotosNbPerPage, nbMaxPhotos);
        }

        update_grid_sizes_ui();

        emit settings_updated_signal(true);
    });

    connect(ui.sbPhotosNbPerPage, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]{

        int nbMaxPhotos = ui.sbPhotosNbWidth->value() * ui.sbPhotosNbHeight->value();
        if(nbMaxPhotos < ui.sbPhotosNbPerPage->value()){
            Utility::safe_init_spinbox_value(ui.sbPhotosNbPerPage, nbMaxPhotos);
        }
        emit settings_updated_signal(true);
    });

    ui.vlCustom->addWidget(&customW);
    ui.frameCustom->hide();
    ui.pbValidate->setEnabled(false);
    ui.pbRemoveLast->setEnabled(false);
    customW.init(ui.sbWPerso->value(),ui.sbHPerso->value());
    connect(ui.sbHPerso, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]{
        customW.init(ui.sbWPerso->value(),ui.sbHPerso->value());
        emit settings_updated_signal(true);
    });
    connect(ui.sbWPerso, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]{
        customW.init(ui.sbWPerso->value(),ui.sbHPerso->value());
        emit settings_updated_signal(true);
    });
    connect(ui.rbGrid, &QRadioButton::clicked, this, [=]{
        ui.frameCustom->setEnabled(false);
        ui.framePhotosNb->setEnabled(true);
        ui.frameCustom->hide();
        ui.framePhotosNb->show();
        emit settings_updated_signal(false);
    });
    connect(ui.rbPersonnalised, &QRadioButton::clicked, this, [=]{
        ui.frameCustom->setEnabled(true);
        ui.framePhotosNb->setEnabled(false);
        ui.frameCustom->show();
        ui.framePhotosNb->hide();
        emit settings_updated_signal(false);
    });

    connect(&customW, &CustomPageW::current_state_signal, this, [&](QPoint p1, QPoint p2, int validSets){
        Q_UNUSED(p1);
        ui.pbValidate->setEnabled(p2.x() != -1);
        ui.pbRemoveLast->setEnabled(validSets>0 || p2.x() != -1);
        ui.laNbSets->setText("Nb: " + QString::number(validSets + ((p2.x() != -1)?1:0)));
        emit settings_updated_signal(false);
    });
    connect(ui.pbValidate, &QPushButton::clicked, &customW, &CustomPageW::validate_current_set);
    connect(ui.pbRemoveLast, &QPushButton::clicked, &customW, &CustomPageW::remove_last_set);


    connect(ui.cbColumnsWidth, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&]{

        for(auto && dsb : columnsWidthDsb){
            dsb->hide();
        }
        columnsWidthDsb[ui.cbColumnsWidth->currentIndex()]->show();
    });
    connect(ui.cbLinesHeight, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&]{

        for(auto && dsb : linesHeightDsb){
            dsb->hide();
        }
        linesHeightDsb[ui.cbLinesHeight->currentIndex()]->show();
    });

    connect(ui.pbAddPositions, &QPushButton::clicked, this, [&]{

        QString filePath = QFileDialog::getSaveFileName(nullptr, "Fichier de positions", QCoreApplication::applicationDirPath() + "/data/positions", "Pos (*.pos)");
        if(filePath.size() == 0){
            return;
        }

        QFile file(filePath);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            return;
        }

        QTextStream out(&file);
        if(ui.rbGrid->isChecked()){
            out <<  "grid\n";
            out << ui.sbPhotosNbPerPage->value() << "\n";
            out << ui.sbPhotosNbWidth->value() << "x" << ui.sbPhotosNbHeight->value() << "\n";

            for(int ii = 0; ii < linesHeightDsb.size(); ++ii){
                out << "l " << linesHeightDsb[ii]->value() << "\n";
            }

            for(int ii = 0; ii < columnsWidthDsb.size(); ++ii){
                out << "c " << columnsWidthDsb[ii]->value() << "\n";
            }
        }else{
            out <<  "custom\n";
//            out << (customW.relativePositions.size() + (customW.currentSecondPoint.x() > -1 ? 1 : 0)) << "\n";
            out << customW.relativePositions.size() << "\n";
            out <<  customW.sizeGrid.width() << "x" << customW.sizeGrid.height() << "\n";

            for(int ii = 0; ii < customW.relativePositions.size(); ++ii){

                out << "p " << customW.relativePositions[ii].x() << ":" << customW.relativePositions[ii].y() << " "
                    << customW.relativePositions[ii].width() << ":" << customW.relativePositions[ii].height() << "\n";
            }

        }

        file.close();

        posLoaded = false;
        read_pos_files();
        emit reset_pos_signal();
    });

    update_grid_sizes_ui();
    read_pos_files();
}

void PageSetsW::read_pos_files(){

    qDebug() << "read_pos_files: " << posLoaded;
    if(!posLoaded){
        predefPositions.clear();
        QDirIterator dirIt(QCoreApplication::applicationDirPath() + "/data/positions",QDirIterator::Subdirectories);
        while (dirIt.hasNext()) {
            dirIt.next();
            if (QFileInfo(dirIt.filePath()).isFile()){
                if (QFileInfo(dirIt.filePath()).suffix() == "pos"){

                    QFile file(dirIt.filePath());
                    if(!file.open(QIODevice::ReadOnly)){
                        continue;
                    }

                    SetsPositionSettings pos;

                    bool invalidFile = false;
                    QTextStream in(&file);
                    QString line;
                    int currentLine = 0;
                    while (in.readLineInto(&line)) {

                        if(line.size() == 0){
                            break;
                        }

                        if(currentLine==0){ // mode
                            pos.customMode = (line == "custom");
                        }else if(currentLine==1){ // nb photos
                            pos.nbPhotos = line.toInt();
                        }else if(currentLine==2){ // size grid
                            QStringList size = line.split('x');
                            if(size.size() !=2){
                                invalidFile = true;
                                break;
                            }
                            pos.nbPhotosH = size[0].toInt();
                            pos.nbPhotosV = size[1].toInt();
                        }else if(pos.customMode){ // custom mode -> positions photos : p 1:1 6:4

                            if(pos.relativePosCustom.size() == pos.nbPhotos){
                                break;
                            }

                            QStringList parts = line.split(" ");
                            if(parts.size() != 3 ){
                                invalidFile = true;
                                break;
                            }

                            QStringList topLeft  = parts[1].split(':');
                            QStringList sizeRect = parts[2].split(':');

                            if(topLeft.size() != 2 || sizeRect.size() != 2){
                                invalidFile = true;
                                break;
                            }

                            qreal x = topLeft[0].toDouble();
                            qreal y = topLeft[1].toDouble();
                            qreal w = sizeRect[0].toDouble();
                            qreal h = sizeRect[1].toDouble();

                            if((x+w) > 1. || (y+h) > 1.){
                                invalidFile = true;
                                break;
                            }

                            pos.relativePosCustom.push_back(QRectF(x,y,w,h));

                        }else{ // grid mode -> size columns and lines -> l 0.2 | c 0.2

                            QStringList parts = line.split(" ");
                            if(parts.size() != 2){
                                invalidFile = true;
                                break;
                            }

                            if(parts[0] == "l"){ // line

                                if(pos.linesHeight.size() == pos.nbPhotosV){
                                    break;
                                }

                                pos.linesHeight.push_back(parts[1].toDouble());

                            }else if(parts[0] == "c"){ // column

                                if(pos.columnsWidth.size() == pos.nbPhotosH){
                                    break;
                                }
                                pos.columnsWidth.push_back(parts[1].toDouble());
                            }
                        }

                        ++currentLine;
                    }

                    if(!invalidFile){
                        predefPositions.push_back(std::move(pos));
                    }
                }
            }
        }
    }
    posLoaded = true;

    // reset ui
    for(auto &&button : predefButtons){
        delete button;
    }
    predefButtons.clear();

    // init ui
    int nbColumns = 6;
    for(int ii = 0; ii < predefPositions.size(); ++ii){
        int currentCol = ii/nbColumns;
        int currentLine = ii%nbColumns;

        int currentPhoto = predefButtons.size();
        QPushButton *button = new QPushButton();
        connect(button, &QPushButton::clicked, this, [=]{
            init_ui_with_settings(currentPhoto);
        });

        button->setStyleSheet(
                    "QPushButton {"
                        "border: 1px solid gray;"
                        "border-radius: 3px;"
                        "border-bottom-left-radius: 0px;"
                        "border-bottom-right-radius: 0px;"
                        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #f6f7fa, stop: 1 #dadbde);"
                     "}"
                    "QPushButton::enabled {"
                        "border: 1px solid rgb(0,106,255);"
                     "}"
                    "QPushButton:hover:!pressed{"
                         "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #dadbde, stop: 1 #f6f7fa);"
                    "}"
                    "QPushButton:pressed{"
                       "background-color:  rgb(220,220,220);"
                   "}"
                );


        button->setText("");
        button->setMinimumWidth(60);
        button->setMinimumHeight(60);
        button->setMaximumHeight(60);
        button->setMaximumWidth(60);
        button->setIconSize(QSize(60,60));


        QImage iconImg(60,60, QImage::Format_ARGB32);
        iconImg.fill(qRgba(255,255,255,0));

        QPainter painter(&iconImg);
        if(predefPositions[ii].customMode){
            for(int jj = 0; jj < predefPositions[ii].relativePosCustom.size(); ++jj){

                const QRectF &rect = predefPositions[ii].relativePosCustom[jj];
                QRect setRect(static_cast<int>(rect.x()*iconImg.width()),
                              static_cast<int>(rect.y()*iconImg.height()),
                              static_cast<int>((rect.width())*iconImg.width()),
                              static_cast<int>((rect.height())*iconImg.height()));

                painter.fillRect(setRect, QBrush(m_colors[jj%m_colors.size()]));
                painter.setPen(Qt::black);
                painter.drawRect(setRect);
            }
        }else{

            QSize interSize(50,50);
            int currentPhoto = 0;
            qreal offsetX = 0.;
            qreal offsetY = 0.;
            for(int jj = 0; jj < predefPositions[ii].nbPhotosV; ++jj){

                qreal height= predefPositions[ii].linesHeight[jj];
                offsetX = 0.;
                for(int kk = 0; kk < predefPositions[ii].nbPhotosH; ++kk){
                    if(currentPhoto == predefPositions[ii].nbPhotos){
                        break;
                    }

                    qreal width = predefPositions[ii].columnsWidth[kk];
                    QRect setRect(static_cast<int>(5+offsetX), static_cast<int>(5+offsetY), static_cast<int>(width*interSize.width()), static_cast<int>(height*interSize.height()));
                    painter.fillRect(setRect, QBrush(m_colors[currentPhoto%m_colors.size()]));
                    painter.setPen(Qt::black);
                    painter.drawRect(setRect);

                    ++currentPhoto;
                    offsetX += width*interSize.width();

                }
                offsetY += height*interSize.height();
            }
        }

//        QFont font;
//        font.setPointSizeF(20.);
//        painter.setPen(Qt::white);
//        painter.setFont(font);
//        painter.drawText(20,40, predefPositions[ii].customMode ? "P" : "G");

        QIcon icon(QPixmap::fromImage(iconImg));
        button->setIcon(icon);

        ui.glPreDef->addWidget(button, currentCol, currentLine, Qt::AlignmentFlag::AlignCenter);
        predefButtons.push_back(button);
    }
}

void PageSetsW::init_ui(PageSetsW &p1, const PageSetsW &p2){

    Utility::safe_init_spinbox_value(p1.ui.sbPhotosNbHeight,    p2.ui.sbPhotosNbHeight->value());
    Utility::safe_init_spinbox_value(p1.ui.sbPhotosNbWidth,     p2.ui.sbPhotosNbWidth->value());
    Utility::safe_init_spinbox_value(p1.ui.sbPhotosNbPerPage,   p2.ui.sbPhotosNbPerPage->value());

    Utility::safe_init_spinbox_value(p1.ui.sbHPerso, p2.ui.sbHPerso->value());
    Utility::safe_init_spinbox_value(p1.ui.sbWPerso, p2.ui.sbWPerso->value());
    Utility::safe_init_radio_button_state(p1.ui.rbGrid, p2.ui.rbGrid->isChecked());
    Utility::safe_init_radio_button_state(p1.ui.rbPersonnalised, p2.ui.rbPersonnalised->isChecked());

    if(p2.ui.frameCustom->isEnabled()){
        p1.ui.frameCustom->setEnabled(true);
        p1.ui.framePhotosNb->setEnabled(false);
        p1.ui.frameCustom->show();
        p1.ui.framePhotosNb->hide();
    }else{
        p1.ui.frameCustom->setEnabled(false);
        p1.ui.framePhotosNb->setEnabled(true);
        p1.ui.frameCustom->hide();
        p1.ui.framePhotosNb->show();
    }

    CustomPageW::init_ui(p1.customW, p2.customW);

    p1.update_grid_sizes_ui();

    for(int ii = 0; ii < p1.columnsWidthDsb.size(); ++ii){
        Utility::safe_init_double_spinbox_value(p1.columnsWidthDsb[ii], p2.columnsWidthDsb[ii]->value());
    }
    for(int ii = 0; ii < p1.linesHeightDsb.size(); ++ii){
        Utility::safe_init_double_spinbox_value(p1.linesHeightDsb[ii], p2.linesHeightDsb[ii]->value());
    }
    Utility::safe_init_combo_box_index(p1.ui.cbColumnsWidth, p2.ui.cbColumnsWidth->currentIndex());
    Utility::safe_init_combo_box_index(p1.ui.cbLinesHeight, p2.ui.cbLinesHeight->currentIndex());
}

void PageSetsW::update_settings(SetsPositionSettings &settings) const{

    // grid
    settings.nbPhotosH          = ui.sbPhotosNbWidth->value();
    settings.nbPhotosV          = ui.sbPhotosNbHeight->value();
    settings.nbPhotos           = ui.sbPhotosNbPerPage->value();

    settings.columnsWidth.clear();
    settings.columnsWidth.reserve(columnsWidthDsb.size());
    for(const auto &dsb : columnsWidthDsb){
        settings.columnsWidth.push_back(dsb->value());
    }

    settings.linesHeight.clear();
    settings.linesHeight.reserve(linesHeightDsb.size());
    for(const auto &dsb : linesHeightDsb){
        settings.linesHeight.push_back(dsb->value());
    }

    // custom
    settings.customMode         = ui.rbPersonnalised->isChecked();
    settings.relativePosCustom  = customW.relativePositions;
    if(customW.currentSecondPoint.x() > -1){
        settings.relativePosCustom.push_back(customW.currRelativePos);
    }
}

void PageSetsW::update_grid_sizes_ui(){

    int currentColumnsId = ui.cbColumnsWidth->currentIndex();
    int currentLinesId   = ui.cbLinesHeight->currentIndex();

    if(currentColumnsId == -1){
        currentColumnsId = 0;
    }
    if(currentLinesId == -1){
        currentLinesId = 0;
    }


    int diffColumns = ui.sbPhotosNbWidth->value()  - columnsWidthDsb.size();
    if(diffColumns < 0){ // delete
        for(int ii = 0; ii < -diffColumns; ++ii){
            delete columnsWidthDsb.last();
            columnsWidthDsb.removeLast();
            ui.cbColumnsWidth->removeItem(ui.cbColumnsWidth->count()-1);
        }

        qreal total = 0;
        for(const auto & dsb : columnsWidthDsb){
            total += dsb->value();
        }

        qreal factor = 1./total;
        for(auto && dsb : columnsWidthDsb){
            Utility::safe_init_double_spinbox_value(dsb, dsb->value()*factor);
        }

    }else if(diffColumns > 0){ // add

        for(int ii = 0; ii < diffColumns; ++ii){

            QDoubleSpinBox *newDsb = new QDoubleSpinBox();
            newDsb->setStyleSheet(dsbStyle);
            newDsb->setDecimals(3);
            newDsb->setMinimum(0.);
            newDsb->setMaximum(1.);
            newDsb->setSingleStep(0.01);

            if(columnsWidthDsb.size() > 0){
                newDsb->setValue(1.- (1.*columnsWidthDsb.size()/(columnsWidthDsb.size()+1)));
            }else{
                newDsb->setValue(1.);
            }
            connect(newDsb, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged), this, [&]{

                qreal total = 0;
                for(int ii = 0; ii < columnsWidthDsb.size(); ++ii){
                    total += columnsWidthDsb[ii]->value();
                }

                qreal factor = 1./total;
                for(int ii = 0; ii < columnsWidthDsb.size(); ++ii){
                    if(ii == ui.cbColumnsWidth->currentIndex()){
                        continue;
                    }
                    Utility::safe_init_double_spinbox_value(columnsWidthDsb[ii],columnsWidthDsb[ii]->value()*factor);
                }
                emit settings_updated_signal(true);
            });

            if(columnsWidthDsb.size() == 1){
                columnsWidthDsb[0]->setValue(0.5);
            }else{
                qreal factor = 1. - newDsb->value();
                for(auto && dsb : columnsWidthDsb){
                    Utility::safe_init_double_spinbox_value(dsb,dsb->value()*factor);
                }
            }

            ui.vlColumnsWidth->addWidget(newDsb);
            columnsWidthDsb.push_back(newDsb);
            ui.cbColumnsWidth->addItem("Colonne n°" + QString::number(ui.cbColumnsWidth->count()));
        }
    }

    int diffLines   = ui.sbPhotosNbHeight->value() - linesHeightDsb.size();
    if(diffLines < 0){ // delete

        for(int ii = 0; ii < -diffLines; ++ii){
            delete linesHeightDsb.last();
            linesHeightDsb.removeLast();
            ui.cbLinesHeight->removeItem(ui.cbLinesHeight->count()-1);
        }

        qreal total = 0;
        for(const auto & dsb : linesHeightDsb){
            total += dsb->value();
        }

        qreal factor = 1./total;
        for(auto && dsb : linesHeightDsb){
            Utility::safe_init_double_spinbox_value(dsb, dsb->value()*factor);
        }

    }else if(diffLines > 0){ // add

        for(int ii = 0; ii < diffLines; ++ii){

            QDoubleSpinBox *newDsb = new QDoubleSpinBox();
            newDsb->setStyleSheet(dsbStyle);
            newDsb->setDecimals(3);
            newDsb->setMinimum(0.);
            newDsb->setMaximum(1.);
            newDsb->setSingleStep(0.01);
            if(linesHeightDsb.size() > 0){
                newDsb->setValue(1.- (1.*linesHeightDsb.size()/(linesHeightDsb.size()+1)));
            }else{
                newDsb->setValue(1.);
            }
            connect(newDsb, QOverload<qreal>::of(&QDoubleSpinBox::valueChanged), this, [&]{

                qreal total = 0;
                for(int ii = 0; ii < linesHeightDsb.size(); ++ii){
                    total += linesHeightDsb[ii]->value();
                }

                qreal factor = 1./total;
                for(int ii = 0; ii < linesHeightDsb.size(); ++ii){
                    if(ii == ui.cbLinesHeight->currentIndex()){
                        continue;
                    }
                    Utility::safe_init_double_spinbox_value(linesHeightDsb[ii],linesHeightDsb[ii]->value()*factor);
                }
                emit settings_updated_signal(true);
            });


            if(linesHeightDsb.size() == 1){
                linesHeightDsb[0]->setValue(0.5);
            }else{
                qreal factor = 1. - newDsb->value();
                for(auto && dsb : linesHeightDsb){
                    Utility::safe_init_double_spinbox_value(dsb,dsb->value()*factor);
                }
            }

            ui.vlLinesHeights->addWidget(newDsb);
            linesHeightDsb.push_back(newDsb);
            ui.cbLinesHeight->addItem("Ligne n°" + QString::number(ui.cbLinesHeight->count()));
        }
    }

    if(currentColumnsId > ui.cbColumnsWidth->count()-1){
        currentColumnsId = ui.cbColumnsWidth->count()-1;
    }
    Utility::safe_init_combo_box_index(ui.cbColumnsWidth, currentColumnsId);

    if(currentLinesId > ui.cbLinesHeight->count()-1){
        currentLinesId = ui.cbLinesHeight->count()-1;
    }
    Utility::safe_init_combo_box_index(ui.cbLinesHeight, currentLinesId);

    for(auto && dsb : columnsWidthDsb){
        dsb->hide();
    }

    for(auto && dsb : linesHeightDsb){
        dsb->hide();
    }

    columnsWidthDsb[currentColumnsId]->show();
    linesHeightDsb[currentLinesId]->show();
}

void PageSetsW::init_ui_with_settings(int id){

    SetsPositionSettings &pos = predefPositions[id];
    ui.frameCustom->setEnabled(pos.customMode);
    ui.framePhotosNb->setEnabled(!pos.customMode);

    if(ui.frameCustom->isEnabled()){
        ui.framePhotosNb->setEnabled(false);
        ui.frameCustom->show();
        ui.framePhotosNb->hide();
    }else{
        ui.framePhotosNb->setEnabled(true);
        ui.frameCustom->hide();
        ui.framePhotosNb->show();
    }

    Utility::safe_init_radio_button_state(ui.rbGrid, !pos.customMode);
    Utility::safe_init_radio_button_state(ui.rbPersonnalised, pos.customMode);

    if(!pos.customMode){
        Utility::safe_init_spinbox_value(ui.sbPhotosNbHeight,    pos.nbPhotosV);
        Utility::safe_init_spinbox_value(ui.sbPhotosNbWidth,     pos.nbPhotosH);
        Utility::safe_init_spinbox_value(ui.sbPhotosNbPerPage,   pos.nbPhotos);
        update_grid_sizes_ui();

        for(int ii = 0; ii < columnsWidthDsb.size(); ++ii){
            Utility::safe_init_double_spinbox_value(columnsWidthDsb[ii], pos.columnsWidth[ii]);
        }
        for(int ii = 0; ii < linesHeightDsb.size(); ++ii){
            Utility::safe_init_double_spinbox_value(linesHeightDsb[ii], pos.linesHeight[ii]);
        }

    }else{

        Utility::safe_init_spinbox_value(ui.sbHPerso, pos.nbPhotosV);
        Utility::safe_init_spinbox_value(ui.sbWPerso, pos.nbPhotosH);
        customW.update_ui_with_settings(pos);
        ui.pbRemoveLast->setEnabled(pos.relativePosCustom.size() > 0);
    }

    emit settings_updated_signal(true);
}
